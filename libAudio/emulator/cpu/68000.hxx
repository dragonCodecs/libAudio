// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_CPU_68000_HXX
#define EMULATOR_CPU_68000_HXX

#include <cstdint>
#include <array>
#include <substrate/flags>
#include "../memoryMap.hxx"

enum class m68kStatusBits_t
{
	// User byte bit meanings
	carry = 0U,
	overflow = 1U,
	zero = 2U,
	negative = 3U,
	extend = 4U,

	// System byte bit meanings
	// 8-10 are interrupt bits
	supervisor = 13U,
	trace = 15U,
};

struct motorola68000_t
{
private:
	memoryMap_t<uint32_t> &_peripherals;

	// There are 8 data registers, 7 address registers, 2 stack pointers,
	// a program counter and a status register in a m68k
	std::array<uint32_t, 8> d;
	std::array<uint32_t, 7> a;
	uint32_t systemStackPointer;
	uint32_t userStackPointer;
	uint32_t programCounter;
	// By default the system starts up in supervisor (system) mode
	substrate::bitFlags_t<uint16_t, m68kStatusBits_t> status{m68kStatusBits_t::supervisor};

public:
	motorola68000_t(memoryMap_t<uint32_t> &peripherals) noexcept;
};

#endif /*EMULATOR_CPU_68000_HXX*/
