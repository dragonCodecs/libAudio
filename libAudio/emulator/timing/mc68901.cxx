// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <substrate/span>
#include "mc68901.hxx"
#include "../unitsHelpers.hxx"

mc68901_t::mc68901_t(const uint32_t clockFrequency) noexcept : clockedPeripheral_t<uint32_t>{clockFrequency},
	timers{{{clockFrequency}, {clockFrequency}, {clockFrequency}, {clockFrequency}}} { }

void mc68901_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
}

void mc68901_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
}

bool mc68901_t::clockCycle() noexcept
{
	return true;
}
