// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_SOUND_YM2149_HXX
#define EMULATOR_SOUND_YM2149_HXX

#include <cstdint>
#include <array>
#include <random>
#include <substrate/span>
#include "../memoryMap.hxx"

namespace ym2149
{
	struct channel_t final
	{
	private:
		uint16_t period{0U};
		uint16_t counter{0U};
		bool edgeState;

	public:
		uint8_t level{0U};

		void writeFrequency(uint8_t value, bool roughAdjust) noexcept;
		uint8_t readFrequency(bool roughAdjust) const noexcept;

		void resetEdgeState(std::minstd_rand &rng, std::uniform_int_distribution<uint16_t> &dist) noexcept;
		void step() noexcept;
		[[nodiscard]] bool state(bool toneInhibit) const noexcept;
		[[nodiscard]] bool shiftRequired() const noexcept;

		void forceEdgeState(bool state) noexcept;
	};
} // namespace ym2149

struct ym2149_t final : public clockedPeripheral_t<uint32_t>
{
private:
	std::minstd_rand rng;
	std::uniform_int_distribution<uint16_t> rngDistribution{};

	uint8_t selectedRegister{0U};
	uint8_t cyclesTillUpdate{0U};

	std::array<ym2149::channel_t, 3U> channels{};
	std::array<bool, 3U> channelState{};
	uint8_t noisePeriod{0U};
	uint8_t noiseCounter{0U};
	// Default to no output from any channel of any kind
	uint8_t mixerConfig{0x3fU};
	uint16_t envelopePeriod{0U};
	uint16_t envelopeCounter{0U};
	uint8_t envelopeShape{0U};
	uint8_t envelopePosition{64U};
	std::array<uint8_t, 2U> ioPort{};

	bool noiseState{0U};
	uint32_t noiseLFSR{1U};

	clockManager_t clockManager;
	bool ready{false};
	bool read{false};

	// Have enough space for 2048 samples of adjustment history
	constexpr static size_t dcAdjustmentLengthLog2{11U};
	std::array<uint16_t, 1U << dcAdjustmentLengthLog2> dcAdjustmentBuffer{};
	size_t dcAdjustmentPosition{0U};
	uint32_t dcAdjustmentSum{0U};

	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept override;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept override;

	void updateFSM() noexcept;
	[[nodiscard]] uint8_t computeEnvelopeLevel() const noexcept;
	[[nodiscard]] int16_t dcAdjust(uint16_t sample) noexcept;

public:
	ym2149_t(uint32_t clockFrequency, uint32_t sampleFrequency) noexcept;
	ym2149_t(const ym2149_t &) = default;
	ym2149_t(ym2149_t &&) = default;
	ym2149_t &operator =(const ym2149_t &) = default;
	ym2149_t &operator =(ym2149_t &&) = default;
	~ym2149_t() noexcept override = default;

	[[nodiscard]] bool clockCycle() noexcept override;
	[[nodiscard]] bool sampleReady() const noexcept;
	[[nodiscard]] int16_t sample() noexcept;

	// NB, only use for testing
	void forceChannelStates(bool edgeState) noexcept;
};

#endif /*EMULATOR_SOUND_YM2149_HXX*/
