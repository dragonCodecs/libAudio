// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <cmath>
#include <tuple>
#include "memoryMap.hxx"

clockManager_t::clockManager_t(const uint32_t baseClockFrequency, const uint32_t targetClockFrequency) noexcept :
	baseFrequency{baseClockFrequency}, targetFrequency{targetClockFrequency} { }

bool clockManager_t::advanceCycle() noexcept
{
	// Advance the time since the last managed cycle
	cycleCounter += targetFrequency;

	// Check if enough base clock cycles have happened
	if (baseFrequency - cycleCounter < targetFrequency)
	{
		// Issue a cycle and take into account the ratio between the frequencies
		cycleCounter -= baseFrequency;
		return true;
	}
	return false;
}
