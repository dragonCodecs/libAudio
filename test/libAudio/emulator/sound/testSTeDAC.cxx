// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <crunch++.h>
#include "emulator/memoryMap.hxx"
#include "emulator/cpu/m68k.hxx"
#include "emulator/timing/mc68901.hxx"
#include "emulator/sound/steDAC.hxx"
#include "emulator/unitsHelpers.hxx"

using m68kMemoryMap_t = memoryMap_t<uint32_t, 0x00ffffffU>;

class testSTeDAC final : public testsuite, m68kMemoryMap_t
{
	motorola68000_t cpu{*this, 8_MHz};
	mc68901_t mfp{2457600U, cpu, 6U};
	steDAC_t dac{50_kHz + 66U, mfp};

	void testRegisterIO()
	{
		//
	}

public:
	CRUNCH_VIS testSTeDAC() noexcept : testsuite{}, m68kMemoryMap_t{} { }

	void registerTests() final
	{
		CXX_TEST(testRegisterIO)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testSTeDAC>();
}
