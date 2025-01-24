// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include "atariSTe.hxx"
#include "ram.hxx"
#include "unitsHelpers.hxx"

atariSTe_t::atariSTe_t() noexcept
{
	// Build the system memory map
	addressMap[{0x000000, 0x080000}] = std::make_unique<ram_t<uint32_t, 512_KiB>>();
}
