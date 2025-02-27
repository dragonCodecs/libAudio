// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_GEMDOS_ALLOC_HXX
#define EMULATOR_GEMDOS_ALLOC_HXX

#include <cstdint>
#include <vector>
#include <optional>

struct allocChunk_t final
{
	uint32_t base{0U};
	uint32_t size{0U};
};

struct gemdosAllocator_t final
{
	uint32_t heapBegin;
	uint32_t heapEnd;
	uint32_t heapCurrent;

	std::vector<allocChunk_t> freeList{};
	std::vector<allocChunk_t> allocList{};

	[[nodiscard]] uint32_t changeHeapSize(int32_t amount) noexcept;

public:
	gemdosAllocator_t(uint32_t base, uint32_t size) noexcept;

	[[nodiscard]] std::optional<uint32_t> alloc(size_t size) noexcept;
};

#endif /*EMULATOR_GEMDOS_ALLOC_HXX*/
