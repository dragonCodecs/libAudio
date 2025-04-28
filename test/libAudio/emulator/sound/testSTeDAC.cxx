// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <crunch++.h>
#include "emulator/memoryMap.hxx"
#include "emulator/cpu/m68k.hxx"
#include "emulator/timing/mc68901.hxx"
#include "emulator/sound/steDAC.hxx"
#include "emulator/unitsHelpers.hxx"

using m68kMemoryMap_t = memoryMap_t<uint32_t, 0x00ffffffU>;

void writeRegister(peripheral_t<uint32_t> &periph, uint8_t reg, uint8_t value) noexcept
{
	periph.writeAddress(reg, {&value, 1U});
}

void writeRegister(peripheral_t<uint32_t> &periph, uint8_t reg, uint16_t value) noexcept
{
	std::array<uint8_t, 2> data{};
	writeBE(value, data);
	periph.writeAddress(reg, data);
}

template<typename T> T readRegister(peripheral_t<uint32_t> &periph, uint8_t reg) noexcept
{
	std::array<uint8_t, sizeof(T)> result{};
	periph.readAddress(reg, result);
	if constexpr (sizeof(T) == 1)
		return result[0U];
	else
		return readBE<T>(result);
}

class testSTeDAC final : public testsuite, m68kMemoryMap_t
{
	motorola68000_t cpu{*this, 8_MHz};
	mc68901_t mfp{2457600U, cpu, 6U};
	steDAC_t dac{50_kHz + 66U, mfp};

	void testMicrowire()
	{
		writeRegister(dac, 0x24U, uint16_t{0x0ffeU});
		assertEqual(readRegister<uint16_t>(dac, 0x24U), 0x0ffeU);
		writeRegister(dac, 0x22U, uint16_t{0x09d1U});
	}

	void testDMARegisterIO()
	{
		//
	}

public:
	CRUNCH_VIS testSTeDAC() noexcept : testsuite{}, m68kMemoryMap_t{} { }

	void registerTests() final
	{
		CXX_TEST(testMicrowire)
		CXX_TEST(testDMARegisterIO)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testSTeDAC>();
}
