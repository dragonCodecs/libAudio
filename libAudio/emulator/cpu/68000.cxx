// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include "68000.hxx"

motorola68000_t::motorola68000_t(memoryMap_t<uint32_t> &peripherals) noexcept : _peripherals{peripherals}
{
}
