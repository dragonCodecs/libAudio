// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_SOUND_YM2149_HXX
#define EMULATOR_SOUND_YM2149_HXX

#include <cstdint>
#include <array>
#include <random>
#include <substrate/span>
#include "../memoryMap.hxx"

struct Ym2149;
struct Ym2149Channel;

struct ym2149_t final : public clockedPeripheral_t<uint32_t>
{
private:
	std::minstd_rand rng;
	std::uniform_int_distribution<uint16_t> rngDistribution{};

	Ym2149 *self{nullptr};

	std::array<Ym2149Channel *, 3U> channels{};

	clockManager_t clockManager;

	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept final;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept final;

public:
	ym2149_t(uint32_t clockFrequency, uint32_t sampleFrequency) noexcept;
	ym2149_t(const ym2149_t &) noexcept = delete;
	ym2149_t(ym2149_t &&) noexcept = delete;
	ym2149_t &operator =(const ym2149_t &) noexcept = delete;
	ym2149_t &operator =(ym2149_t &&) noexcept = delete;
	~ym2149_t() noexcept final;

	[[nodiscard]] bool clockCycle() noexcept final;
	[[nodiscard]] bool sampleReady() const noexcept;
	[[nodiscard]] int16_t sample() noexcept;

	[[nodiscard]] bool advanceCycle() noexcept;

	// NB, only use for testing
	void forceChannelStates(bool edgeState) noexcept;
};

#endif /*EMULATOR_SOUND_YM2149_HXX*/
