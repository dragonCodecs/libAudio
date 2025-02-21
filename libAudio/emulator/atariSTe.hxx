// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_ATARI_STE_HXX
#define EMULATOR_ATARI_STE_HXX

#include "memoryMap.hxx"
#include "cpu/m68k.hxx"
#include "unitsHelpers.hxx"
#include "sndh/iceDecrunch.hxx"

// M68k has a 24-bit address bus, but we can't directly represent that, so use a 32-bit address value instead.
struct atariSTe_t : protected memoryMap_t<uint32_t, 0x00ffffffU>
{
private:
	motorola68000_t cpu{*this, 8_MHz};

public:
	atariSTe_t() noexcept;

	[[nodiscard]] bool copyToRAM(sndhDecruncher_t &data) noexcept;
	[[nodiscard]] bool init(uint16_t subtune) noexcept;
};

#endif /*EMULATOR_ATARI_STE_HXX*/
