// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <substrate/span>
#include "ym2149.hxx"

ym2149_t::ym2149_t(const uint32_t clockFreq) noexcept : clockFrequency{clockFreq}
{
}

void ym2149_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
}

void ym2149_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
	// Only admit 8- and 16-bit writes
	if (data.size_bytes() > 2U)
		return;
	// The address to write determines if we're making a register selection or writing a register
	switch (address)
	{
		case 0U: // Register selection
			// Only the bottom 4 bits of the selection are valid/used
			selectedRegister = data[0U] & 0x0fU;
			break;
		case 2U: // Register write
			//
			break;
	}
}
