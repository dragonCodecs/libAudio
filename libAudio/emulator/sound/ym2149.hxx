// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_SOUND_YM2149_HXX
#define EMULATOR_SOUND_YM2149_HXX

#include <cstdint>
#include <array>
#include <substrate/span>
#include "../memoryMap.hxx"

namespace ym2149
{
	struct channel_t final
	{
		uint16_t frequency{0U};
		uint8_t level{0U};

		void writeFrequency(uint8_t value, bool roughAdjust) noexcept;
		uint8_t readFrequency(bool roughAdjust) const noexcept;
	};
} // namespace ym2149

struct ym2149_t final : public clockedPeripheral_t<uint32_t>
{
private:
	uint8_t selectedRegister{0U};
	uint8_t cyclesToUpdate{0U};

	std::array<ym2149::channel_t, 3U> channel{};
	uint8_t noiseFrequency{0U};
	uint8_t mixerConfig{0U};
	uint16_t envelopeFrequency{0U};
	uint8_t envelopeShape{0U};
	std::array<uint8_t, 2U> ioPort{};

	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept override;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept override;

	void updateFSM() noexcept;

public:
	ym2149_t(uint32_t clockFrequency) noexcept;
	ym2149_t(const ym2149_t &) = default;
	ym2149_t(ym2149_t &&) = default;
	ym2149_t &operator =(const ym2149_t &) = default;
	ym2149_t &operator =(ym2149_t &&) = default;
	~ym2149_t() noexcept override = default;

	[[nodiscard]] bool clockCycle() noexcept override;
	[[nodiscard]] bool sampleReady() const noexcept;
};

#endif /*EMULATOR_SOUND_YM2149_HXX*/
