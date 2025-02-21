// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <cmath>
#include <tuple>
#include "memoryMap.hxx"

clockManager_t::clockManager_t(const uint32_t baseClockFrequency, const uint32_t targetClockFrequency) noexcept :
	clockRatio{static_cast<float>(static_cast<double>(baseClockFrequency) / static_cast<double>(targetClockFrequency))} { }

bool clockManager_t::advanceCycle() noexcept
{
	// Advance the time since the last managed cycle
	++timeSinceLastCycle;

	// Decompose the clock ratio into its integer and fractional components
	const auto [clockRatioInt, clockRatioFrac]
	{
		[this]() -> std::tuple<uint32_t, float>
		{
			float intPart;
			const auto floatPart{std::modf(clockRatio, &intPart)};
			return {static_cast<uint32_t>(intPart), floatPart};
		}()
	};

	// Grab the integral part of the clock ratio and correction factor to compute the number
	// of base cycles to the next managed cycle
	const auto clocksToNextCycle{clockRatioInt + static_cast<uint32_t>(std::trunc(correction))};

	// Check if enough base clock cycles have happened
	if (timeSinceLastCycle == clocksToNextCycle)
	{
		// If the correction factor went above 1 (a correction cycle was necessary),
		// then adjust it back down so we maintain the range [0, 1)
		if (correction > 1.0f)
			correction -= 1.0f;

		// Add the fractional part of the ratio to the correction factor
		correction += clockRatioFrac;

		// Reset the time since the last cycle as we're going to issue one now
		timeSinceLastCycle = 0U;
		return true;
	}
	return false;
}
