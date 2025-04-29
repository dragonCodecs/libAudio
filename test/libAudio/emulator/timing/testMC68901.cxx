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

void writeRegister(peripheral_t<uint32_t> &periph, const uint8_t reg, uint8_t value) noexcept
{
	periph.writeAddress(reg, {&value, 1U});
}

template<typename T> void writeRegister(peripheral_t<uint32_t> &periph, const uint8_t reg, const T value) noexcept
{
	std::array<uint8_t, sizeof(T)> data{};
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

	void testRegisterIO()
	{
		// Verify the function of the GPIO registers
		writeRegister(mfp, 0x00U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x00U), 0x00ffU);
		assertEqual(readRegister<uint8_t>(mfp, 0x01U), 0xffU);
		writeRegister(mfp, 0x01U, uint8_t{0x00U});
		assertEqual(readRegister<uint8_t>(mfp, 0x01U), 0x00U);
		writeRegister(mfp, 0x02U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x02U), 0x00ffU);
		writeRegister(mfp, 0x04U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x04U), 0x00ffU);
		// Verify the function of the interrupt control registers
		writeRegister(mfp, 0x06U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x06U), 0x00ffU);
		writeRegister(mfp, 0x08U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x08U), 0x00ffU);
		writeRegister(mfp, 0x0aU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x0aU), 0x00ffU);
		writeRegister(mfp, 0x0cU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x0cU), 0x00ffU);
		writeRegister(mfp, 0x0eU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x0eU), 0x00ffU);
		writeRegister(mfp, 0x10U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x10U), 0x00ffU);
		writeRegister(mfp, 0x12U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x12U), 0x00ffU);
		writeRegister(mfp, 0x14U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x14U), 0x00ffU);
		// Verify the function of the vector register
		writeRegister(mfp, 0x16U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x16U), 0x00f8U);
		// Verify the function of the timer registers
		writeRegister(mfp, 0x18U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x18U), 0x001fU);
		writeRegister(mfp, 0x1eU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x1eU), 0x0000U);
		writeRegister(mfp, 0x19U, uint8_t{0x00U});
		writeRegister(mfp, 0x1aU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x1aU), 0x001fU);
		writeRegister(mfp, 0x20U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x20U), 0x0000U);
		writeRegister(mfp, 0x1bU, uint8_t{0x00U});
		writeRegister(mfp, 0x1cU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x1cU), 0x0077U);
		writeRegister(mfp, 0x22U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x22U), 0x0000U);
		writeRegister(mfp, 0x24U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x24U), 0x0000U);
		writeRegister(mfp, 0x1dU, uint8_t{0x00U});
		writeRegister(mfp, 0x1eU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x1eU), 0x00ffU);
		writeRegister(mfp, 0x20U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x20U), 0x00ffU);
		writeRegister(mfp, 0x22U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x22U), 0x00ffU);
		writeRegister(mfp, 0x24U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x24U), 0x00ffU);
		// Verify the function of the UART registers
		writeRegister(mfp, 0x26U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x26U), 0x00ffU);
		writeRegister(mfp, 0x28U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x28U), 0x00ffU);
		writeRegister(mfp, 0x2aU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x2aU), 0x00ffU);
		writeRegister(mfp, 0x2cU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x2cU), 0x00ffU);
		writeRegister(mfp, 0x2eU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x2eU), 0x00ffU);
	}

	void testBadRegisterIO()
	{
		// Should be discarded as writing to an even address for an odd byte register
		writeRegister(mfp, 0x00U, uint8_t{0xffU});
		assertEqual(readRegister<uint8_t>(mfp, 0x00U), 0x00U);
		assertEqual(readRegister<uint16_t>(mfp, 0x00U), 0x0000U);
		// Should be discarded as writing 32 bits in a go is inadmissable
		writeRegister(mfp, 0x00U, uint32_t{0xffffffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x00U), 0x0000U);
		assertEqual(readRegister<uint16_t>(mfp, 0x02U), 0x00ffU);
		// Should be a no-op as reading 32 bits in a go is inadmissable
		assertEqual(readRegister<uint32_t>(mfp, 0x00U), 0x00000000U);
	}

	void testConfigureTimer()
	{
		// Check that we can configure timer A for ~50Hz operation
		mfp.configureTimer(0U, 245U, 7U);
		assertEqual(readRegister<uint8_t>(mfp, 0x19U), 0x07U);
		assertEqual(readRegister<uint8_t>(mfp, 0x1fU), 245U);

		// Check that we can configure timer C for 200Hz operation
		mfp.configureTimer(2U, 192U, 5U);
		assertEqual(readRegister<uint8_t>(mfp, 0x1dU), 0x50U);
		assertEqual(readRegister<uint8_t>(mfp, 0x23U), 192U);

		// This call will be a no-op
		mfp.configureTimer(4U, 192U, 5U);
	}

	void testIRQGeneration()
	{
		// Reconfigure the interrupt registers so just timers A and C can generate events
		// Enable just timer A and C IRQs
		writeRegister(mfp, 0x07U, uint8_t{0x20U});
		writeRegister(mfp, 0x09U, uint8_t{0x20U});
		// Reset the pending and in-service status for both
		writeRegister(mfp, 0x0bU, uint8_t{0x00U});
		writeRegister(mfp, 0x0dU, uint8_t{0x00U});
		writeRegister(mfp, 0x0fU, uint8_t{0x00U});
		writeRegister(mfp, 0x11U, uint8_t{0x00U});
		// Make sure IRQs will be delivered at the correct vector base address
		writeRegister(mfp, 0x17U, uint8_t{0x40U});
		// Make sure the mask registers allow IRQ generation so we can
		// mark pending interrupts for the timers
		writeRegister(mfp, 0x13U, uint8_t{0x20U});
		writeRegister(mfp, 0x15U, uint8_t{0x20U});
		assertFalse(cpu.hasPendingInterrupts());

		// Configure and enable timer B but not its IRQs generation
		mfp.configureTimer(1U, 2U, 1U);

		// Cycle the clock until timer C fires and we generate an interrupt
		for (const auto _ : substrate::indexSequence_t{12288U})
		{
			// Make sure now interrupts are set yet
			assertEqual(readRegister<uint8_t>(mfp, 0x0bU), 0x00U);
			assertEqual(readRegister<uint8_t>(mfp, 0x0dU), 0x00U);
			// Now wiggle the clock
			assertTrue(mfp.clockCycle());
		}
		// Now verify that it's TC's IRQ that's actually pending
		assertEqual(readRegister<uint8_t>(mfp, 0x0bU), 0x00U);
		assertEqual(readRegister<uint8_t>(mfp, 0x0dU), 0x20U);
		assertTrue(cpu.hasPendingInterrupts());
		// Assert that the TC interrupt creates the proper cause number
		assertEqual(static_cast<m68k::irqRequester_t &>(mfp).irqCause(), 0x45U);
		// And that that reset the pending bit
		assertEqual(readRegister<uint8_t>(mfp, 0x0dU), 0x00U);

		// Reconfigure timer A to be sensitive to external events
		writeRegister(mfp, 0x19U, uint8_t{0x00U});
		mfp.configureTimer(0U, 1U, 0x08U);

		// Generate some more clock cycles so we're 50% of the way to timer C firing again
		for (const auto _ : substrate::indexSequence_t{6144U})
		{
			assertEqual(readRegister<uint8_t>(mfp, 0x0bU), 0x00U);
			assertEqual(readRegister<uint8_t>(mfp, 0x0dU), 0x00U);
			assertTrue(mfp.clockCycle());
		}
		// Mark timer A with an external event
		mfp.fireDMAEvent();
		assertTrue(mfp.clockCycle());
		// Generate some more clock cycles so we're so timer C fires again
		for (const auto _ : substrate::indexSequence_t{6143U})
		{
			assertEqual(readRegister<uint8_t>(mfp, 0x0bU), 0x20U);
			assertEqual(readRegister<uint8_t>(mfp, 0x0dU), 0x00U);
			assertTrue(mfp.clockCycle());
		}

		// Check we now have two pending interrupts
		assertEqual(readRegister<uint8_t>(mfp, 0x0bU), 0x20U);
		assertEqual(readRegister<uint8_t>(mfp, 0x0dU), 0x20U);
		// Verify that A's pops before C's
		assertEqual(static_cast<m68k::irqRequester_t &>(mfp).irqCause(), 0x4dU);
		assertEqual(static_cast<m68k::irqRequester_t &>(mfp).irqCause(), 0x45U);
	}

	void testGPIO7Events()
	{
		// Enable IRQs for just GPIO7
		writeRegister(mfp, 0x07U, uint8_t{0x80U});
		writeRegister(mfp, 0x09U, uint8_t{0x00U});
		// Reset the pending and in-service status for both
		writeRegister(mfp, 0x0bU, uint8_t{0x00U});
		writeRegister(mfp, 0x0dU, uint8_t{0x00U});
		writeRegister(mfp, 0x0fU, uint8_t{0x00U});
		writeRegister(mfp, 0x11U, uint8_t{0x00U});
		// Make sure IRQs will be delivered at the correct vector base address
		writeRegister(mfp, 0x17U, uint8_t{0x40U});
		// Make sure the mask registers do not allow any IRQ generation just yet
		writeRegister(mfp, 0x13U, uint8_t{0x00U});
		writeRegister(mfp, 0x15U, uint8_t{0x00U});
		// Disable timer A as an external event source
		mfp.configureTimer(0U, 0U, 0U);

		// Check that running the peripheral does nothing now
		assertTrue(mfp.clockCycle());
		assertEqual(readRegister<uint8_t>(mfp, 0x0bU), 0x00U);
		assertEqual(readRegister<uint8_t>(mfp, 0x0dU), 0x00U);

		// Generate an interrupt on GPIO7
		mfp.fireDMAEvent();
		assertTrue(mfp.clockCycle());
		assertEqual(readRegister<uint8_t>(mfp, 0x0bU), 0x80U);
		assertEqual(readRegister<uint8_t>(mfp, 0x0dU), 0x00U);
		// The interrupt is presently masked, so.. irqCause should report a spurious error
		assertEqual(static_cast<m68k::irqRequester_t &>(mfp).irqCause(), 0x18U);
		// Unmask and do that again..
		writeRegister(mfp, 0x13U, uint8_t{0x80U});
		assertEqual(static_cast<m68k::irqRequester_t &>(mfp).irqCause(), 0x4fU);
	}

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
	CRUNCH_VIS testMC68901() noexcept : testsuite{}, m68kMemoryMap_t{} { }

	void registerTests() final
	{
		CXX_TEST(testRegisterIO)
		CXX_TEST(testBadRegisterIO)
		CXX_TEST(testConfigureTimer)
		CXX_TEST(testIRQGeneration)
		CXX_TEST(testGPIO7Events)
		CXX_TEST(testTimerAEventCounting)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMC68901>();
}
