// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <array>
#include <memory>
#include <substrate/indexed_iterator>
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

	void testBranch()
	{
		// Set up 3 branches in each of the 3 instruction encooding forms, checking they jump properly to each other
		// Start with the 8-bit immediate form, then use the 16-bit immediate form, and finally the 32-bit immediate
		// form (6 bytes to encode)
		writeAddress(0x000100U, uint16_t{0x6050U}); // Jump to +0x50 from end of instruction
		writeAddress(0x000152U, uint16_t{0x6000U});
		writeAddress(0x000154U, uint16_t{0xfefcU}); // Jump to -0x104 from end of instruction
		writeAddress(0x000050U, uint16_t{0x60ffU});
		writeAddress(0x000052U, uint16_t{0x0001U}); // Jump to +0x100ae from end of instruction
		writeAddress(0x000054U, uint16_t{0x00aeU});
		writeAddress(0x010100U, uint16_t{0x4e75U}); // RTS to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000100U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000100U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000152U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000050U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00010100U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the fourth and final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
		assertEqual(cpu.readStatus(), 0x0000U);
	}

	void testJump()
	{
		// NB: We don't test all EA's here as we get through them in other tests.
		writeAddress(0x000000U, uint16_t{0x4ed0U}); // JMP (a0)
		writeAddress(0x000004U, uint16_t{0x4ee8U});
		writeAddress(0x000006U, uint16_t{0x0100U}); // JMP 0x0100(a0)
		writeAddress(0x000104U, uint16_t{0x4ef8U});
		writeAddress(0x000106U, uint16_t{0x0110U}); // JMP (0x0110).W
		writeAddress(0x000110U, uint16_t{0x4ef9U});
		writeAddress(0x000112U, uint16_t{0x0001U});
		writeAddress(0x000114U, uint16_t{0x0000U}); // JMP (0x00010000).L
		writeAddress(0x010000U, uint16_t{0x4e75U}); // RTS to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		cpu.writeAddrRegister(0U, 0x00000004U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(0U), 0x00000004U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000104U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000110U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00010000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the fifth and final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
		assertEqual(cpu.readStatus(), 0x0000U);
	}

public:
	CRUNCH_VIS testM68k() noexcept : testsuite{}, memoryMap_t<uint32_t, 0x00ffffffU>{}
	{
		addressMap[{0x000000U, 0x800000U}] = std::make_unique<ram_t<uint32_t, 8_MiB>>();
	}

	void registerTests() final
	{
		CXX_TEST(testDecode)
		CXX_TEST(testBranch)
		CXX_TEST(testJump)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testM68k>();
}
