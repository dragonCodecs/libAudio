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
		CXX_TEST(testDisplayRegs)
	}
};

CRUNCHpp_TESTS(testM68k)
