// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <crunch++.h>
#include <substrate/index_sequence>
#include "emulator/memoryMap.hxx"
#include "emulator/unitsHelpers.hxx"

class testClockManager final : public testsuite
{
	void testWholeRatio()
	{
		clockManager_t manager{32_MHz, 2_MHz};
		// Run a good few cycles
		for ([[maybe_unused]] const auto loop : substrate::indexSequence_t{32U})
		{
			// 32:2 gives a ratio of 16
			for (const auto cycle : substrate::indexSequence_t{16U})
			{
				// Expect advanceCycle to return true only on the 16th cycle
				const bool lastCycle{cycle == 15U};
				assertEqual(manager.advanceCycle(), lastCycle);
			}
		}
	}

	void testFractionalRatio()
	{
		clockManager_t manager{32_MHz, 2457600};
		// Run a good few cycles
		for (const auto loop : substrate::indexSequence_t{64U})
		{
			// The 47th cycle should be the correction cycle
			const auto correction{loop == 47U};
			// 32:2.4576 gives a ratio of 13.020833333
			const auto cycles{13U + (correction ? 1U : 0U)};
			for (const auto cycle : substrate::indexSequence_t{cycles})
			{
				// Expect advanceCycle to return true only on the last cycle of the loop
				const bool lastCycle{cycle == cycles - 1U};
				assertEqual(manager.advanceCycle(), lastCycle);
			}
		}
	}

public:
	void registerTests() final
	{
		CXX_TEST(testWholeRatio)
		CXX_TEST(testFractionalRatio)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testClockManager>();
}
