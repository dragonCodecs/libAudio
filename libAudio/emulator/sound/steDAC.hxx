// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_SOUND_STEDAC_HXX
#define EMULATOR_SOUND_STEDAC_HXX

#include <cstdint>
#include <substrate/span>
#include "../memoryMap.hxx"

struct steDAC_t final : public clockedPeripheral_t<uint32_t>
{
private:
	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept final;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept final;

	// NB: only reason for these to be marked mutable is so microwireCycle() can be const
	// for use in the readAddress call - not ideal, but not completely terrible
	uint16_t microwireData{0U};
	mutable uint16_t microwireMask{0U};
	mutable uint8_t microwireCycles{0U};
	uint8_t mainVolume{64U};

	void runMicrowireTransaction() noexcept;
	[[nodiscard]] uint16_t microwireCycle() const noexcept;

public:
	steDAC_t(uint32_t clockFrequency) noexcept;
	steDAC_t(const steDAC_t &) noexcept = delete;
	steDAC_t(steDAC_t &&) noexcept = delete;
	steDAC_t &operator =(const steDAC_t &) noexcept = delete;
	steDAC_t &operator =(steDAC_t &&) noexcept = delete;
	~steDAC_t() noexcept final = default;

	[[nodiscard]] bool clockCycle() noexcept final;
	[[nodiscard]] uint8_t outputLevel() const noexcept { return mainVolume; }
};

#endif /*EMULATOR_SOUND_STEDAC_HXX*/
