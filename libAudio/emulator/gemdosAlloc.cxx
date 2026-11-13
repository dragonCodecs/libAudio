// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include "gemdosAlloc.hxx"

extern "C"
{
	GemdosAllocator *gemdos_allocator_new(uint32_t, uint32_t);
	void gemdos_allocator_drop(GemdosAllocator *);

	uint32_t gemdos_allocator_alloc(GemdosAllocator *, size_t);
	bool gemdos_allocator_free(GemdosAllocator *, uint32_t);
	uint32_t gemdos_allocator_largest_free_block(const GemdosAllocator *);
}

gemdosAllocator_t::gemdosAllocator_t(const uint32_t base, const uint32_t size) noexcept :
	self{gemdos_allocator_new(base, size)} { }

gemdosAllocator_t::~gemdosAllocator_t() noexcept
	{ gemdos_allocator_drop(self); }

std::optional<uint32_t> gemdosAllocator_t::alloc(const size_t size) noexcept
{
	auto ptr = gemdos_allocator_alloc(self, size);
	if (ptr == UINT32_MAX)
		return std::nullopt;
	return ptr;
}

bool gemdosAllocator_t::free(const uint32_t ptr) noexcept
{
	return gemdos_allocator_free(self, ptr);
}

uint32_t gemdosAllocator_t::largestFreeBlock() const noexcept
{
	return gemdos_allocator_largest_free_block(self);
}
