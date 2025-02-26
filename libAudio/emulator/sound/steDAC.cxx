// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include "steDAC.hxx"

steDAC_t::steDAC_t(const uint32_t clockFrequency) noexcept : clockedPeripheral_t<uint32_t>{clockFrequency}
	{ }

void steDAC_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
}

void steDAC_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
}

bool steDAC_t::clockCycle() noexcept
{
	return true;
}
