// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <tuple>
#include <substrate/span>
#include <substrate/index_sequence>
#include <substrate/buffer_utils>
#include "steDAC.hxx"

/*
 * Implements the Atari STe's Sound DMA, Microwire and DAC interfaces
 *
 * References:
 * - Sound DMA is implemented by the GST Shifter chip, no official documentation exists however
 *   some RE has been done as detailed here: https://info-coach.fr/atari/hardware/STE-HW.php#shifter
 * - The Microwire interface is under-documented, but at heart provides access to the registers
 *   of various blocks on the STe, including the LMC1992 which provides volume control
 * - LMC1992 is a National Semi part through which all audio flows to get out the STe
 *   archived datasheet: https://www.tomek.cedro.info/files/electronics/doc/ic_various/LMC1992.PDF
 * - STe and other schematics: https://docs.dev-docs.org/htm/search.php?find=_s
 */

steDAC_t::steDAC_t(const uint32_t clockFrequency, mc68901_t &mfp) noexcept :
	clockedPeripheral_t<uint32_t>{clockFrequency}, _mfp{mfp} { }

void steDAC_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	// Extract how wide an access this is
	const auto accessWidth{data.size_bytes()};
	// Only admit 8- and 16-bit reads
	if (accessWidth > 2U)
		return;
	// Microwire registers are only accessible as u16's
	if (accessWidth == 2U)
	{
		// u16 accesses must be to even addresses
		if ((address & 1U) != 0U)
			return;

		// Handle microwire address accesses
		switch (address)
		{
			case 0x22U:
				writeBE(microwireData, data);
				return;
			case 0x24U:
				writeBE(microwireCycle(), data);
				return;
			default:
				// Redirect everything else to 8-bit access
				readAddress(address + 1U, data.subspan(1));
				break;
		}
	}
	// The rest of the registers are only accessible as u8's
	else if (accessWidth == 1U)
	{
		// Registers in this peripheral are on the odd byte
		if ((address & 1U) != 1U)
			return;

		// Handle DMA sound system register accesses
		switch (address >> 1U)
		{
			case 0x00U:
				data[0] = control;
				break;
			case 0x01U:
			case 0x02U:
			case 0x03U:
				data[0] = baseAddress.readByte((address >> 1U) - 1U);
				break;
			case 0x04U:
			case 0x05U:
			case 0x06U:
				data[0] = sampleCounter.readByte((address >> 1U) - 4U);
				break;
			case 0x07U:
			case 0x08U:
			case 0x09U:
				data[0] = endAddress.readByte((address >> 1U) - 7U);
				break;
			case 0x10U:
				// Convert the sample channel count and rate divider back into their forms for the peripheral interface
				data[0] = ((sampleMono ? 1U : 0U) << 7U) | (3U - sampleRateDivider);
				break;
		}
	}
}

void steDAC_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
	// Extract how wide an access this is
	const auto accessWidth{data.size_bytes()};
	// Only admit 8- and 16-bit writes
	if (accessWidth > 2U)
		return;
	// Microwire registers are only accessible as u16's
	if (accessWidth == 2U)
	{
		// u16 accesses must be to even addresses
		if ((address & 1U) != 0U)
			return;

		// Handle microwire address accesses
		switch (address)
		{
			case 0x22U:
				microwireData = readBE<uint16_t>(data);
				runMicrowireTransaction();
				break;
			case 0x24U:
				microwireMask = readBE<uint16_t>(data);
				break;
			default:
				// Redirect everything else to 8-bit access
				writeAddress(address + 1U, data.subspan(1));
				break;
		}
	}
	// The rest of the registers are only accessible as u8's
	else if (accessWidth == 1U)
	{
		// Registers in this peripheral are on the odd byte
		if ((address & 1U) != 1U)
			return;

		// Handle DMA sound system register accesses
		switch (address >> 1U)
		{
			case 0x00U:
				// Only the bottom two bits of the control byte are valid
				control = data[0] & 0x03U;
				// If playback is just starting, reset the sample counter
				if (control & (1U << 0U))
					sampleCounter.reset();
				break;
			case 0x01U:
			case 0x02U:
			case 0x03U:
				baseAddress.writeByte((address >> 1U) - 1U, data[0]);
				break;
			case 0x04U:
			case 0x05U:
			case 0x06U:
				sampleCounter.writeByte((address >> 1U) - 4U, data[0]);
				break;
			case 0x07U:
			case 0x08U:
			case 0x09U:
				endAddress.writeByte((address >> 1U) - 7U, data[0]);
				break;
			case 0x10U:
				// Determine whether the new mode should be mono or stereo
				sampleMono = (data[0] & (1U << 7U)) == (1U << 7U);
				// Grab the frequency bits and turn them into a divider (3 == 2^3, 0 == 2^0)
				sampleRateDivider = 3U - (data[0] & 0x03U);
				// Reset the rate counter to adjust for the new sample rate
				sampleRateCounter = 0U;
				break;
		}
	}
}

bool steDAC_t::clockCycle() noexcept
{
	// Check if there's anything playing or not, and if not then exit early
	if ((control & (1U << 0U)) == 0x00U)
		return true;

	// Something's playing, great.. step the rate counter to see if we should do something
	// in this cycle, and check if the counter overflowed
	if (++sampleRateCounter == (1U << sampleRateDivider))
		// Yes, so reset it
		sampleRateCounter = 0U;
	else
		// No, okay - we're done.. exit early
		return true;

	// Step the sample counter based on whether we're playing mono or stereo
	sampleCounter += sampleMono ? 1U : 2U;

	// If the sample address equals the end address
	if (baseAddress + sampleCounter == endAddress)
	{
		// Reset the counter back to the start
		sampleCounter.reset();
		// If we're not looping playback, disable DMA
		if ((control & (1U << 1U)) == 0x00U)
			control &= 0xfeU;
		// Ping the MFP with the completion event
		_mfp.fireDMAEvent();
	}

	return true;
}

void steDAC_t::runMicrowireTransaction() noexcept
{
	// Every microwire transaction is 16 cycles
	microwireCycles = 16U;
	// Cycle through the bits and count the number actually used by the mask,
	// building a condensed value to work with
	const auto [value, bitsUsed]
	{
		[this]() -> std::tuple<uint16_t, size_t>
		{
			uint16_t data{0U};
			size_t bits{0U};
			// Loop through all the bits
			for (const auto bit : substrate::indexSequence_t{microwireCycles})
			{
				// If this bit is used per the mask
				if (microwireMask & (1U << bit))
				{
					// Then copy the value of the data into our local and count it
					data |= ((microwireData >> bit) & 1U) << bits;
					++bits;
				}
			}
			// The result is a neat tuple
			return {data, bits};
		}()
	};
	// Now check for a specific write of a specific width representing an access
	// to the main volume control register for the DAC
	if (bitsUsed == 11U && (value >> 9U) == 2U)
	{
		// Extract out the write data and the write address
		const auto data{value & 0x3fU};
		const auto address{(value >> 6U) & 0x07U};
		// And see where the write is targeted
		switch (address)
		{
			case 3U: // DAC main volume register
				// If the value of the write is more than the max allowed volume, cap it
				if (data > 40U)
					mainVolume = 64U;
				// Otherwise rescale it to our internal 0-64 levels
				else
					mainVolume = (data * 64U) / 40U;
				break;
		}
	}
}

uint16_t steDAC_t::microwireCycle() const noexcept
{
	// If there are still cycles to do
	if (microwireCycles)
	{
		--microwireCycles;
		// Rotate the mask one more bit left
		microwireMask = (microwireMask << 1U) | (microwireMask >> 15U);
	}
	return microwireMask;
}

int16_t steDAC_t::sample(const memoryMap_t<uint32_t, 0x00ffffffU> &memoryMap) const noexcept
{
	// If the DMA engine is currently active, grab a sample back and mix down to mono, otherwise return an idle value
	if (control & 0x01U)
	{
		const auto sampleAddress{baseAddress + sampleCounter};
		// If this is mono, just grab one sample and call it good
		if (sampleMono)
			return memoryMap.readAddress<int8_t>(sampleAddress) * 64;
		// Otherwise, grab two and sum
		const auto left{memoryMap.readAddress<int8_t>(sampleAddress)};
		const auto right{memoryMap.readAddress<int8_t>(sampleAddress + 1U)};
		return (left + right) * 32;
	}
	return 0;
}

namespace steDAC
{
	void register24b_t::writeByte(const uint8_t position, const uint8_t byte) noexcept
	{
		const size_t shift{8U * (2U - position)};
		value &= ~(0xffU << shift);
		value |= uint32_t{byte} << shift;
		// The bottom most bit in these registers must remain 0, as must the top 8
		value &= 0x00fffffeU;
	}

	uint8_t register24b_t::readByte(const uint8_t position) const noexcept
	{
		const size_t shift{8U * (2U - position)};
		return static_cast<uint8_t>(value >> shift);
	}

	void register24b_t::reset() noexcept
		{ value = 0U; }

	register24b_t &register24b_t::operator +=(const uint32_t amount) noexcept
	{
		value += amount;
		return *this;
	}
} // namespace steDAC
