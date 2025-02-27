// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include "gemdosAlloc.hxx"

gemdosAllocator_t::gemdosAllocator_t(const uint32_t base, const uint32_t size) noexcept :
	heapStart{base}, heapEnd{base + size}, heapCurrent{base} { }

uint32_t gemdosAllocator_t::changeHeapSize(const int32_t amount) noexcept
{
	// Check if this allocation would exhaust the heap
	if (heapCurrent + amount > heapEnd)
		return UINT32_MAX;

	// Everything okay? okay.. make a copy of the heap pointer to return and
	// then add the allocation to the heap pointer
	const auto result{heapCurrent};
	heapCurrent += amount;
	return result;
}
