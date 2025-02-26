// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_SOUND_STEDAC_HXX
#define EMULATOR_SOUND_STEDAC_HXX

#include <cstdint>
#include "../memoryMap.hxx"

struct steDAC_t final : public clockedPeripheral_t<uint32_t>
{
private:
	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept override;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept override;

public:
	steDAC_t(uint32_t clockFrequency) noexcept;
	steDAC_t(const steDAC_t &) = default;
	steDAC_t(steDAC_t &&) = default;
	steDAC_t &operator =(const steDAC_t &) = default;
	steDAC_t &operator =(steDAC_t &&) = default;
	~steDAC_t() noexcept override = default;

	[[nodiscard]] bool clockCycle() noexcept override;
};

#endif /*EMULATOR_SOUND_STEDAC_HXX*/
