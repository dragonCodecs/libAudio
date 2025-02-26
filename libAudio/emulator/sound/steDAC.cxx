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

steDAC_t::steDAC_t(const uint32_t clockFrequency) noexcept : clockedPeripheral_t<uint32_t>{clockFrequency}
	{ }

void steDAC_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	// Extract how wide an access this is
	const auto accessWidth{data.size_bytes()};
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
		}
	}
}

void steDAC_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
	// Extract how wide an access this is
	const auto accessWidth{data.size_bytes()};
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
		}
	}
}

bool steDAC_t::clockCycle() noexcept
{
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
