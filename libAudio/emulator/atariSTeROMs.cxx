// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <substrate/span>
#include <substrate/buffer_utils>
#include "atariSTeROMs.hxx"
#include "cpu/m68k.hxx"

atariSTeROMs_t::atariSTeROMs_t(motorola68000_t *cpu) noexcept : peripheral_t<uint32_t>{}, _cpu{cpu} { }

void atariSTeROMs_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	// Extract how wide an access this is
	const auto accessWidth{data.size_bytes()};
	// We only support u16 access to the "ROM"
	if (accessWidth != 2U)
		return;
	// If this is an access to GEMDOS via a `trap` instruction, dispatch that
	if (address == handlerAddressGEMDOS)
	{
		handleGEMDOSAccess();
		// And then load an RTE as the result of this read
		writeBE(uint16_t{0x4e73U}, data);
	}
}

void atariSTeROMs_t::writeAddress(const uint32_t, const substrate::span<uint8_t> &) noexcept
{
	// This function is intentionally a no-op as this is a ROM!
}

void atariSTeROMs_t::handleGEMDOSAccess() const noexcept
{
	//
}
