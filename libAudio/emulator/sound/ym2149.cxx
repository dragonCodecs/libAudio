// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <cmath>
#include <random>
#include <numeric>
#include <substrate/span>
#include <substrate/indexed_iterator>
#include "ym2149.hxx"

ym2149_t::ym2149_t(const uint32_t clockFrequency, const uint32_t sampleFrequency) noexcept :
	clockedPeripheral_t<uint32_t>{clockFrequency}, rng{std::random_device{}()}, rngDistribution{0U, 1U},
	clockManager{clockFrequency, sampleFrequency}
{
	for (auto &channel : channels)
		channel.resetEdgeState(rng, rngDistribution);
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
			const auto channel{static_cast<size_t>(selectedRegister >> 1U)};
			const auto roughAdjustment{(selectedRegister & 1U) != 0U};
			data[0U] = channels[channel].readFrequency(roughAdjustment);
			break;
		}
		// Current noise frequency
		case 6U:
			data[0U] = noisePeriod;
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
			const size_t channel{selectedRegister - 8U};
			data[0U] = channels[channel].level;
			break;
		}
		// Envelope frequency fine adjustment
		case 11U:
			data[0U] = static_cast<uint8_t>(envelopePeriod);
			break;
		// Envelope frequency rough adjustment
		case 12U:
			data[0U] = static_cast<uint8_t>(envelopePeriod >> 8U);
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
					const auto channel{static_cast<size_t>(selectedRegister >> 1U)};
					const auto roughAdjustment{(selectedRegister & 1U) != 0U};
					// Select the channel and write the adjustment
					channels[channel].writeFrequency(data[0U], roughAdjustment);
					break;
				}
				// Noise frequency adjustment
				case 6U:
					// Register is actually only 5 bit, so discard the upper 3
					noisePeriod = data[0U] & 0x1fU;
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
					const size_t channel{selectedRegister - 8U};
					// Adjust that channel's levels (only 5 bits valid, discard the upper 3)
					channels[channel].level = data[0U] & 0x1fU;
					break;
				}
				// Envelope frequency fine adjustment
				case 11U:
					envelopePeriod &= 0xff00U;
					envelopePeriod |= data[0U];
					break;
				// Envelope frequency rough adjustment
				case 12U:
					envelopePeriod &= 0x00ffU;
					envelopePeriod |= data[0U] << 8U;
					break;
				// Envelope shape adjustment
				case 13U:
					envelopeShape = data[0U] & 0x0fU;
					// Reset where we are in the envelope as a result
					envelopePosition = 0U;
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
	// Reset the channel states if ready was true
	if (ready)
	{
		read = false;
		for (auto &state : channelState)
			state = false;
	}

	// See if a sample should be ready this cycle or not
	ready = clockManager.advanceCycle();

	// If we should update the FSM in this cycle, do so
	if (cyclesTillUpdate == 0U)
		updateFSM();
	// The internal state machine runs at 1/8th the device clock speed
	// (presume that ~SEL is low, yielding f(T) = (f(Master) / 2) / (16 * TP)
	// which simplifies to f(T) = f(Master) / (8 * TP))
	cyclesTillUpdate = (cyclesTillUpdate + 1U) & 7U;
	return true;
}

void ym2149_t::updateFSM() noexcept
{
	// Update the channel states to reflect the current chip state
	for (const auto &[idx, channel] : substrate::indexedIterator_t{channels})
	{
		// Check to see the state for tone generation
		const auto channelToneState{channel.state(mixerConfig & (1U << idx))};
		// Followed by noise generation for the channel
		const auto channelNoiseState{noiseState || bool(mixerConfig & (1U << (idx + 3U)))};
		// Combine it all together and update the channel state
		channelState[idx] |= channelToneState && channelNoiseState;
	}

	// Step all the channels forward one internal cycle
	for (auto &channel : channels)
		channel.step();

	// Step the envelope generator forward one internal cycle
	// If the envelope counter exceeds the period of the envelope
	if (++envelopeCounter >= envelopePeriod)
	{
		// Reset the counter
		envelopeCounter = 0U;
		// Update the envelope position (128 possible positions, the last 64 loop)
		if (++envelopePosition > 64U)
			envelopePosition = (envelopePosition & 0x3fU) | 0x40U;
	}

	// Step the noise generator forward one internal cycle, noting it runs
	// at half speed to everything else. If the noise counter exceeds the
	// period of the noise to be generated
	if ((++noiseCounter) >= noisePeriod)
	{
		// Reset the counter
		noiseCounter = 0U;
		// Set the noise state if the 0th and 2nd bits of the noise LFSR are not the same value as each other
		noiseState = (noiseLFSR ^ (noiseLFSR >> 2U)) & 1U;
		// Shift the new noise state in as single bit on the LHS of the noise LFSR
		noiseLFSR >>= 1U;
		noiseLFSR |= noiseState ? (1U << 16U) : 0U;
	}
}

bool ym2149_t::sampleReady() const noexcept { return ready && !read; }

int16_t ym2149_t::sample() noexcept
{
	read = true;
	// Grab the current envelope level
	const auto envelopeLevel{computeEnvelopeLevel()};

	std::array<uint16_t, 3U> levels;
	// Go through each channel and compute the volume level associated with it
	for (const auto &[channel, state] : substrate::indexedIterator_t{channelState})
	{
		const auto level
		{
			[&](const size_t index, const bool channelEnabled) -> uint8_t
			{
				if (channelEnabled)
				{
					// Extract the level information from the channel
					const auto rawLevel{channels[index].level};
					// If it indicates the level is controlled by the envelope, use that
					if (rawLevel & 0x10U)
						return envelopeLevel;
					// Otherwise, use the raw level for the channel, scaled appropriately
					else
						return rawLevel << 1U;
				}
				return 0U;
			}(channel, state)
		};
		// Check to see what shift should be applied (1 if there is one needed, 0 otherwise)
		const auto shift{static_cast<size_t>(channels[channel].shiftRequired())};
		// Compute and store the final level using the logarithm table for the chip
		levels[channel] = ym2149::logLevel(level) >> shift;
	}

	// Sum the levels and do DC adjustment
	return dcAdjust(std::reduce(levels.begin(), levels.end()));
}

uint8_t ym2149_t::computeEnvelopeLevel() const noexcept
{
	// Extract out the actual position and phase data (32 unique positions, 4 phases)
	const auto position{envelopePosition & 0x1fU};
	const auto phase{envelopePosition >> 5U};
	switch (phase)
	{
		// Initial envelope phase, behaviour determined by attack bit
		case 0U:
			// If the attack bit is set, the envelope level is the position value, otherwise
			// the ramp runs backwards, so invert the position to make the level.
			if (envelopeShape & 0x4U)
				return position;
			return 0x1fU - position;
		// The 2nd and 4th phase is determined identically by the rest of the shape bits
		case 1U:
		case 3U:
			// If this is not a continuous envelope, the level is low permanently
			if (envelopeShape & 0x8U)
				return 0U;
			// Otherwise, if the envelope should hold its value
			if (envelopeShape & 0x1U)
			{
				// Low or high determined by attack and alternate bits xor'd together
				const auto state{(envelopeShape & 0x2U) ^ ((envelopeShape & 0x4U) >> 1U)};
				return state ? 0x1fU : 0x00U;
			}
			// If the alternate bit is set, then ramp the opposite direction to before
			if (envelopeShape & 0x2U)
			{
				if (envelopeShape & 0x4U)
					return 0x1fU - position;
				return position;
			}
			// Otherwise, ramp the same as before
			if (envelopeShape & 0x4U)
				return position;
			return 0x1fU - position;
		// The 3rd phase is also determined by the rest of the shape bits, but has the opposite
		// logic for the alternate-controlled ramping when alternate set
		case 2U:
			// If this is not a continuous envelope, the level is low permanently
			if (envelopeShape & 0x8U)
				return 0U;
			// Otherwise, if the envelope should hold its value
			if (envelopeShape & 0x1U)
			{
				// Low or high determined by attack and alternate bits xor'd together
				const auto state{(envelopeShape & 0x2U) ^ ((envelopeShape & 0x4U) >> 1U)};
				return state ? 0x1fU : 0x00U;
			}
			// Ramp in the original direction regardless of alternate bit
			if (envelopeShape & 0x4U)
				return position;
			return 0x1fU - position;
	}
	// Should not be possible, but just in case..
	return 0U;
}

int16_t ym2149_t::dcAdjust(uint16_t sample) noexcept
{
	// First adjust the DC ballancing adjustment sum
	dcAdjustmentSum -= dcAdjustmentBuffer[dcAdjustmentPosition];
	dcAdjustmentSum += sample;
	// Then store this sample in the buffer
	dcAdjustmentBuffer[dcAdjustmentPosition] = sample;
	// Update the adjustment position to make this a circular buffer
	dcAdjustmentPosition = (dcAdjustmentPosition + 1U) & (dcAdjustmentBuffer.size() - 1U);
	// Now scale the sum and apply it to the sample to get the final sample
	return static_cast<int16_t>(int32_t{sample} - int32_t(dcAdjustmentSum >> dcAdjustmentLengthLog2));
}

namespace ym2149
{
	void channel_t::resetEdgeState(std::minstd_rand &rng, std::uniform_int_distribution<uint16_t> &dist) noexcept
		{ edgeState = static_cast<bool>(dist(rng)); }

	void channel_t::writeFrequency(const uint8_t value, const bool roughAdjust) noexcept
	{
		if (roughAdjust)
		{
			period &= 0x00ff0U;
			// Only 4 bits valid, throw away the upper 4
			period |= (value & 0x0fU) << 8U;
		}
		else
		{
			period &= 0xff00U;
			period |= value;
		}
	}

	uint8_t channel_t::readFrequency(const bool roughAdjust) const noexcept
	{
		if (roughAdjust)
			return static_cast<uint8_t>(period >> 8U);
		else
			return static_cast<uint8_t>(period);
	}

	void channel_t::step() noexcept
	{
		// If our channel's period counter exceeds the period of the current tone frequency
		if (++counter >= period)
		{
			// Reset the counter and update the edge states
			counter = 0U;
			edgeState = !edgeState;
		}
	}

	bool channel_t::state(const bool toneInhibit) const noexcept
		{ return edgeState || toneInhibit; }

	bool channel_t::shiftRequired() const noexcept
		{ return period <= 1U; }

	// Logarithmic volume levels from 1 / (sqrt(2) ^ (level / 2))
	// The sequence is reversed though to get 0 to be the lowest volume
	uint16_t logLevel(const uint8_t level) noexcept
	{
		// Do the main computation
		double result{1.0 / std::pow(std::sqrt(2.0), (31U - level) / 2.0)};
		// Scale the result to make space for 3 channels of max volume, and convert
		// to the range [0, 32767] to make the result
		return static_cast<uint16_t>((result / 3.0) * 32767U);
	}
} // namespace ym2149
