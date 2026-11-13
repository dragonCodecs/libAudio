// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_GEMDOS_ALLOC_HXX
#define EMULATOR_GEMDOS_ALLOC_HXX

#include <cstdint>
#include <vector>
#include <optional>

struct GemdosAllocator;

struct gemdosAllocator_t final
{
	GemdosAllocator *self{nullptr};

public:
	gemdosAllocator_t(uint32_t base, uint32_t size) noexcept;
	~gemdosAllocator_t() noexcept;

	[[nodiscard]] std::optional<uint32_t> alloc(size_t size) noexcept;
	[[nodiscard]] bool free(uint32_t ptr) noexcept;
	[[nodiscard]] uint32_t largestFreeBlock() const noexcept;
};

#endif /*EMULATOR_GEMDOS_ALLOC_HXX*/
