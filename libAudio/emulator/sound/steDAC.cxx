// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include "steDAC.hxx"

steDAC_t::steDAC_t(const uint32_t clockFrequency) noexcept : clockedPeripheral_t<uint32_t>{clockFrequency}
	{ }

void steDAC_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	// Extract how wide an access this is
	const auto accessWidth{data.size_bytes()};
	// Microwire registers are only accessible as u16's
	if (accessWidth == 2U)
	{
		// u16 accesses must be to even addresses
		if ((address & 1U) != 0U)
			return;

		// Handle microwire address accesses
		switch (address)
		{
			case 0x22U:
				writeBE(microwireData, data);
				return;
			case 0x24U:
				writeBE(microwireCycle(), data);
				return;
		}
	}
}

void steDAC_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
}

bool steDAC_t::clockCycle() noexcept
{
	return true;
}

uint16_t steDAC_t::microwireCycle() const noexcept
{
	// If there are still cycles to do
	if (microwireCycles)
	{
		--microwireCycles;
		// Rotate the mask one more bit left
		microwireMask = (microwireMask << 1U) | (microwireMask >> 15U);
	}
	return microwireMask;
}
