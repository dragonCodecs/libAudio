// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <substrate/span>
#include "ym2149.hxx"

ym2149_t::ym2149_t(const uint32_t clockFreq) noexcept : clockedPeripheral_t<uint32_t>{clockFreq}
{
}

void ym2149_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	// Only admit 8- and 16-bit reads
	if (data.size_bytes() > 2U)
		return;
	// We only respond to reads on address 0
	if (address != 0U)
		return;
	// Where to read from depends on the register selection, so..
	switch (selectedRegister)
	{
		// Current channel frequency rough/fine
		case 0U:
		case 1U:
		case 2U:
		case 3U:
		case 4U:
		case 5U:
		{
			// Turn the register selection into a channel number and rough adjustment indicator
			const auto channelNumber{static_cast<size_t>(selectedRegister >> 1U)};
			const auto roughAdjustment{(selectedRegister & 1U) != 0U};
			data[0U] = channel[channelNumber].readFrequency(roughAdjustment);
			break;
		}
		// Current noise frequency
		case 6U:
			data[0U] = noiseFrequency;
			break;
		// Current mixer configuration
		case 7U:
			data[0U] = mixerConfig;
			break;
		// Current channel level
		case 8U:
		case 9U:
		case 10U:
		{
			// Turn the register selection into a channel number
			const size_t channelNumber{selectedRegister - 8U};
			data[0U] = channel[channelNumber].level;
			break;
		}
		// Envelope frequency fine adjustment
		case 11U:
			data[0U] = static_cast<uint8_t>(envelopeFrequency);
			break;
		// Envelope frequency rough adjustment
		case 12U:
			data[0U] = static_cast<uint8_t>(envelopeFrequency >> 8U);
			break;
		// Envelope shape adjustment
		case 13U:
			data[0U] = envelopeShape;
			break;
		// I/O port data
		case 14U:
		case 15U:
		{
			// Turn the register selection into a port number (repesenting A vs B)
			const auto port{selectedRegister & 1U};
			data[0U] = ioPort[port];
			break;
		}
	}
}

void ym2149_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
	// Only admit 8- and 16-bit writes
	if (data.size_bytes() > 2U)
		return;
	// The address to write determines if we're making a register selection or writing a register
	switch (address)
	{
		case 0U: // Register selection
			// Only the bottom 4 bits of the selection are valid/used
			selectedRegister = data[0U] & 0x0fU;
			break;
		case 2U: // Register write
			// Where we write depends on the register selection, so..
			switch (selectedRegister)
			{
				// Channel frequency rough/fine adjustment
				case 0U:
				case 1U:
				case 2U:
				case 3U:
				case 4U:
				case 5U:
				{
					// Turn the register selection into a channel number and rough adjustment indicator
					const auto channelNumber{static_cast<size_t>(selectedRegister >> 1U)};
					const auto roughAdjustment{(selectedRegister & 1U) != 0U};
					// Select the channel and write the adjustment
					channel[channelNumber].writeFrequency(data[0U], roughAdjustment);
					break;
				}
				// Noise frequency adjustment
				case 6U:
					// Register is actually only 5 bit, so discard the upper 3
					noiseFrequency = data[0U] & 0x1fU;
					break;
				// Mixer configuration adjustment
				case 7U:
					mixerConfig = data[0U];
					break;
				// Channel level adjustment
				case 8U:
				case 9U:
				case 10U:
				{
					// Turn the register selection into a channel number
					const size_t channelNumber{selectedRegister - 8U};
					// Adjust that channel's levels (only 5 bits valid, discard the upper 3)
					channel[channelNumber].level = data[0U] & 0x1fU;
					break;
				}
				// Envelope frequency fine adjustment
				case 11U:
					envelopeFrequency &= 0xff00U;
					envelopeFrequency |= data[0U];
					break;
				// Envelope frequency rough adjustment
				case 12U:
					envelopeFrequency &= 0x00ffU;
					envelopeFrequency |= data[0U] << 8U;
					break;
				// Envelope shape adjustment
				case 13U:
					envelopeShape = data[0U] & 0x0fU;
					break;
				// I/O port data
				case 14U:
				case 15U:
				{
					// Turn the register selection into a port number (repesenting A vs B)
					const auto port{selectedRegister & 1U};
					ioPort[port] = data[0U];
					break;
				}
			}
			break;
	}
}

bool ym2149_t::clockCycle() noexcept
{
	// If we should update the FSM in this cycle, do so
	if (cyclesToUpdate == 0U)
		updateFSM();
	// The internal state machine runs at 1/8th the device clock speed
	// (presume that ~SEL is low, yielding f(T) = (f(Master) / 2) / (16 * TP)
	// which simplifies to f(T) = f(Master) / (8 * TP))
	cyclesToUpdate = (cyclesToUpdate + 1U) & 7U;
	return true;
}

void ym2149_t::updateFSM() noexcept
{
}

bool ym2149_t::sampleReady() const noexcept
{
	return true;
}

namespace ym2149
{
	void channel_t::writeFrequency(const uint8_t value, const bool roughAdjust) noexcept
	{
		if (roughAdjust)
		{
			frequency &= 0x00ff0U;
			// Only 4 bits valid, throw away the upper 4
			frequency |= (value & 0x0fU) << 8U;
		}
		else
		{
			frequency &= 0xff00U;
			frequency |= value;
		}
	}

	uint8_t channel_t::readFrequency(const bool roughAdjust) const noexcept
	{
		if (roughAdjust)
			return static_cast<uint8_t>(frequency >> 8U);
		else
			return static_cast<uint8_t>(frequency);
	}
} // namespace ym2149
