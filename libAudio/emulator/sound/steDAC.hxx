// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_SOUND_STEDAC_HXX
#define EMULATOR_SOUND_STEDAC_HXX

#include <cstdint>
#include <substrate/span>
#include "../memoryMap.hxx"
#include "../timing/mc68901.hxx"

struct STeDac;

struct steDAC_t final : public clockedPeripheral_t<uint32_t>
{
private:
	STeDac *self{nullptr};

	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept final;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept final;

public:
	steDAC_t(uint32_t clockFrequency, mc68901_t &mfp) noexcept;
	steDAC_t(const steDAC_t &) noexcept = delete;
	steDAC_t(steDAC_t &&) noexcept = delete;
	steDAC_t &operator =(const steDAC_t &) noexcept = delete;
	steDAC_t &operator =(steDAC_t &&) noexcept = delete;
	~steDAC_t() noexcept final;

	[[nodiscard]] bool clockCycle() noexcept final;
	[[nodiscard]] uint8_t outputLevel() const noexcept;
	[[nodiscard]] int16_t sample(const memoryMap_t<uint32_t, 0x00ffffffU> &memoryMap) const noexcept;
};

#endif /*EMULATOR_SOUND_STEDAC_HXX*/
