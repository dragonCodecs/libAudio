// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_GEMDOS_ALLOC_HXX
#define EMULATOR_GEMDOS_ALLOC_HXX

#include <cstdint>

struct gemdosAllocator_t final
{
	uint32_t heapStart;
	uint32_t heapEnd;
	uint32_t heapCurrent;

	[[nodiscard]] uint32_t changeHeapSize(int32_t amount) noexcept;

public:
	gemdosAllocator_t(uint32_t base, uint32_t size) noexcept;
};

#endif /*EMULATOR_GEMDOS_ALLOC_HXX*/
