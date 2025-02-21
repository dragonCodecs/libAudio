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
}
