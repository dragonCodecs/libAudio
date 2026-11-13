// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_ATARI_STE_ROMS_HXX
#define EMULATOR_ATARI_STE_ROMS_HXX

#include <cstdint>
#include <substrate/span>
#include "memoryMap.hxx"
#include "cpu/m68k.hxx"

struct AtariSTeROMs;

struct atariSTeROMs_t final : peripheral_t<uint32_t>
{
private:
	AtariSTeROMs *self{nullptr};

	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept final;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept final;

public:
	atariSTeROMs_t(motorola68000_t &cpu, memoryMap_t<uint32_t, 0x00ffffffU> &peripherals,
		uint32_t heapBase, uint32_t heapSize) noexcept;
	atariSTeROMs_t(const atariSTeROMs_t &) noexcept = delete;
	atariSTeROMs_t(atariSTeROMs_t &&) noexcept = delete;
	atariSTeROMs_t &operator =(const atariSTeROMs_t &) noexcept = delete;
	atariSTeROMs_t &operator =(atariSTeROMs_t &&) noexcept = delete;
	~atariSTeROMs_t() noexcept final;

	constexpr static uint32_t handlerAddressGEMDOS{0x000000U};
};

#endif /*EMULATOR_ATARI_STE_ROMS_HXX*/
