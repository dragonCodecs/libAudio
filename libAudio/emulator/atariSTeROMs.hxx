// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_ATARI_STE_ROMS_HXX
#define EMULATOR_ATARI_STE_ROMS_HXX

#include <cstdint>
#include <substrate/span>
#include "memoryMap.hxx"

struct atariSTeROMs_t final : peripheral_t<uint32_t>
{
private:
	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept override;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept override;

	void handleGEMDOSAccess() const noexcept;

public:
	atariSTeROMs_t() noexcept = default;
	atariSTeROMs_t(const atariSTeROMs_t &) = default;
	atariSTeROMs_t(atariSTeROMs_t &&) = default;
	atariSTeROMs_t &operator =(const atariSTeROMs_t &) = default;
	atariSTeROMs_t &operator =(atariSTeROMs_t &&) = default;
	~atariSTeROMs_t() noexcept override = default;

	constexpr static uint32_t handlerAddressGEMDOS{0x000000U};
};

#endif /*EMULATOR_ATARI_STE_ROMS_HXX*/
