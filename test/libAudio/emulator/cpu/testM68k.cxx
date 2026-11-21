// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <array>
#include <memory>
#include <substrate/indexed_iterator>
#include <substrate/index_sequence>
#include <crunch++.h>
#include "emulator/cpu/m68k.hxx"
#include "emulator/ram.hxx"
#include "emulator/unitsHelpers.hxx"
#include "testM68kDecodeTable.hxx"

class testM68k final : public testsuite, memoryMap_t<uint32_t, 0x00ffffffU>
{
private:
	motorola68000_t cpu{*this, 8_MHz};

	void testDecode()
	{
		// Run through all 65536 possible instruction values and check they decode properly.
		for (const auto &[insn, decodedOperation] : substrate::indexedIterator_t{instructionMap})
			assertTrue(cpu.decodeInstruction(insn) == decodedOperation);
	}

	void runStep(const bool expectingTrap = false)
	{
		const auto result{cpu.step()};
		assertTrue(result.validInsn);
		if (expectingTrap)
			assertTrue(result.trap);
		else
			assertFalse(result.trap);
		// TODO: Check the cycles taken matches expectations
	}

	void testTST()
	{
		writeAddress(0x000000U, uint16_t{0x4a00U}); // tst.b d0
		writeAddress(0x000002U, uint16_t{0x4a40U}); // tst.w d0
		writeAddress(0x000004U, uint16_t{0x4a80U}); // tst.l d0
		writeAddress(0x000006U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0 to a sensible value
		cpu.writeDataRegister(0U, 0x80000100U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testDisplayRegs()
	{
		// TODO: Actually do something with this.. this just guarantees coverage for now
		cpu.displayRegs();
	}

public:
	testM68k() noexcept : testsuite{}, memoryMap_t<uint32_t, 0x00ffffffU>{}
	{
		// Check all the CPU preconditions before any execution begins
		for (const auto &reg : substrate::indexSequence_t{8U})
			assertEqual(cpu.readDataRegister(reg), 0U);
		for (const auto &reg : substrate::indexSequence_t{8U})
			assertEqual(cpu.readAddrRegister(reg), 0U);
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readStatus(), 0x2000U);

		// Register some memory for the tests to use
		addressMap[{0x000000U, 0x800000U}] = std::make_unique<ram_t<uint32_t, 8_MiB>>();
	}

	void registerTests() final
	{
		CXX_TEST(testDecode)
		CXX_TEST(testTST)
		CXX_TEST(testDisplayRegs)
	}
};

CRUNCHpp_TESTS(testM68k)
