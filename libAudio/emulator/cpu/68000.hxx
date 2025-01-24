// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_CPU_68000_HXX
#define EMULATOR_CPU_68000_HXX

#include "../memoryMap.hxx"

struct motorola68000_t
{
private:
	memoryMap_t<uint32_t> &_peripherals;

public:
	motorola68000_t(memoryMap_t<uint32_t> &peripherals) noexcept;
};

#endif /*EMULATOR_CPU_68000_HXX*/
