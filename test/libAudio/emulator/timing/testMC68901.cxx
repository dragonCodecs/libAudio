// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <crunch++.h>
#include <substrate/index_sequence>
#include "emulator/memoryMap.hxx"
#include "emulator/cpu/m68k.hxx"
#include "emulator/timing/mc68901.hxx"
#include "emulator/unitsHelpers.hxx"

using m68kMemoryMap_t = memoryMap_t<uint32_t, 0x00ffffffU>;

template<typename T> void writeRegister(peripheral_t<uint32_t> &periph, const uint8_t reg, const T value) noexcept
{
	std::array<uint8_t, sizeof(T)> data{};
	if constexpr (sizeof(T) == 1)
		data[0] = value;
	else
		writeBE(value, data);
	periph.writeAddress(reg, data);
}

template<typename T> T readRegister(peripheral_t<uint32_t> &periph, const uint8_t reg) noexcept
{
	std::array<uint8_t, sizeof(T)> result{};
	periph.readAddress(reg, result);
	if constexpr (sizeof(T) == 1)
		return result[0U];
	else
		return readBE<T>(result);
}

class testMC68901 final : public testsuite, m68kMemoryMap_t
{
	motorola68000_t cpu{*this, 8_MHz};
	mc68901_t mfp{2457600U, cpu, 6U};

	void testTimerAEventCounting()
	{
		// Enable timer A to set a pending interrupt
		writeRegister(mfp, 0x07U, uint8_t{0x20U});
		// Enable timer A to count two external events occuring before triggering
		mfp.configureTimer(0U, 2U, 0x08U);

		// Run a sequence to generate two events, checking between each one what the counter is doing
		assertEqual(readRegister<uint8_t>(mfp, 0x1fU), 0x02U);
		assertTrue(mfp.clockCycle());
		assertEqual(readRegister<uint8_t>(mfp, 0x1fU), 0x02U);
		mfp.fireDMAEvent();
		assertEqual(readRegister<uint8_t>(mfp, 0x1fU), 0x02U);
		assertTrue(mfp.clockCycle());
		assertEqual(readRegister<uint8_t>(mfp, 0x1fU), 0x01U);
		mfp.fireDMAEvent();
		// Check there is no pending interrupt as the event's not yet processed
		assertEqual(readRegister<uint8_t>(mfp, 0x0bU), 0x00U);
		assertTrue(mfp.clockCycle());
		// Check the timer reloads and generates a pending IRQ now it is
		assertEqual(readRegister<uint8_t>(mfp, 0x1fU), 0x02U);
		assertEqual(readRegister<uint8_t>(mfp, 0x0bU), 0x20U);
	}

public:
	testMC68901() noexcept : testsuite{}, m68kMemoryMap_t{} { }

	void registerTests() final
	{
		CXX_TEST(testTimerAEventCounting)
	}
};

CRUNCHpp_TESTS(testMC68901)
