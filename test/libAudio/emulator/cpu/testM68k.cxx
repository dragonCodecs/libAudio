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
		writeAddress(0x010100U, uint16_t{0x4e75U}); // rts to end the test
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
		writeAddress(0x000000U, uint16_t{0x4ed0U}); // jmp (a0)
		writeAddress(0x000004U, uint16_t{0x4ee8U});
		writeAddress(0x000006U, uint16_t{0x0100U}); // jmp 0x0100(a0)
		writeAddress(0x000104U, uint16_t{0x4ef8U});
		writeAddress(0x000106U, uint16_t{0x0110U}); // jmp $0110.w
		writeAddress(0x000110U, uint16_t{0x4ef9U});
		writeAddress(0x000112U, uint16_t{0x0001U});
		writeAddress(0x000114U, uint16_t{0x0000U}); // jmp $00010000.l
		writeAddress(0x010000U, uint16_t{0x4e75U}); // rts to end the test
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

	void testRTS()
	{
		writeAddress(0x000000U, uint16_t{0x4e75U}); // rts
		// Run the solitary rts
		assertTrue(cpu.executeToReturn(0x00000000U, 0x00800000U, false));
		// Then make sure the CPU state matches up
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
		assertEqual(cpu.readStatus(), 0x2000U);
	}

	void testRTE()
	{
		writeAddress(0x000100U, uint16_t{0x4e73U}); // rte
		// Set up to execute an exception vector
		cpu.executeFromException(0x00000100U, 0x00800000U, 1U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000100U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffff8U);
		assertEqual(cpu.readStatus(), 0x2000U);
		// Check that the stacked exception frame is okay
		assertEqual(readAddress<uint16_t>(0x007ffffeU), uint16_t{0x1004U}); // Frame type and vector info
		assertEqual(readAddress<uint32_t>(0x007ffffaU), uint32_t{0xffffffffU}); // Return program counter
		assertEqual(readAddress<uint16_t>(0x007ffff8U), uint16_t{0x0000U}); // Return status register
		// Step the CPU and validate the results line up
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
		assertEqual(cpu.readStatus(), 0x0000U);
	}

	void testADDA()
	{
		writeAddress(0x000000U, uint16_t{0xd0e1U}); // adda.w -(a1), a0
		writeAddress(0x000002U, uint16_t{0xd3c0U}); // adda.l d0, a1
		writeAddress(0x000004U, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint16_t{0xc100U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, a1 and d0 to sensible values
		cpu.writeAddrRegister(0U, 0x00000010U);
		cpu.writeAddrRegister(1U, 0x00000102U);
		cpu.writeDataRegister(0U, 0x40000004U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readAddrRegister(0U), 0xffffc110U);
		assertEqual(cpu.readAddrRegister(1U), 0x00000100U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readAddrRegister(1U), 0x40000104U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testOR()
	{
		writeAddress(0x000000U, uint16_t{0x8080U}); // or.l d0, d0
		writeAddress(0x000002U, uint16_t{0x8050U}); // or.w (a0), d0
		writeAddress(0x000004U, uint16_t{0x803cU});
		writeAddress(0x000006U, uint16_t{0x0040U}); // or.b #$40, d0
		writeAddress(0x000008U, uint16_t{0x8199U}); // or.l d0, (a1)+
		writeAddress(0x00000aU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint32_t{0x84000000U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Re-use the 3rd instruction as the data for the 2nd, and set a reasonable destination for the 4th
		cpu.writeAddrRegister(0U, 0x00000004U);
		cpu.writeAddrRegister(1U, 0x00000100U);
		// Make sure d0 starts out as 0 for this sequence
		cpu.writeDataRegister(0U, 0U);
		// Set the carry and overflow bits in the status register so we can observe them being cleared
		// And the extend bit to make sure that's left alone
		cpu.writeStatus(0x0013U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);
		assertEqual(cpu.readAddrRegister(0U), 0x00000004U);
		assertEqual(cpu.readAddrRegister(1U), 0x00000100U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0013U);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0x0000803cU);
		assertEqual(cpu.readStatus(), 0x0018U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(0U), 0x0000807cU);
		assertEqual(cpu.readStatus(), 0x0010U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(cpu.readDataRegister(0U), 0x0000807cU);
		assertEqual(cpu.readAddrRegister(1U), 0x00000104U);
		assertEqual(readAddress<uint32_t>(0x000100U), uint32_t{0x8400807cU});
		assertEqual(cpu.readStatus(), 0x0018U);
		// Step the fifth and final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testORI()
	{
		writeAddress(0x000000U, uint16_t{0x0038U});
		writeAddress(0x000002U, uint16_t{0x0024U});
		writeAddress(0x000004U, uint16_t{0x0100U}); // ori.b #$24, $0100.w
		writeAddress(0x000006U, uint16_t{0x0040U});
		writeAddress(0x000008U, uint16_t{0x0000U}); // ori.w #0, d0
		writeAddress(0x00000aU, uint16_t{0x0080U});
		writeAddress(0x00000cU, uint16_t{0x8000U});
		writeAddress(0x00000eU, uint16_t{0x0001U}); // ori.l #$80000001, d0
		writeAddress(0x000010U, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint8_t{0x01U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0 so we get zero = true from the second instruction, but get something interesting for the 3rd
		cpu.writeDataRegister(0U, 0x05a00000U);
		// Set the carry and overflow bits in the status register so we can observe them being cleared
		// And the extend bit to make sure that's left alone
		cpu.writeStatus(0x0013U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0013U);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(readAddress<uint8_t>(0x000100U), uint8_t{0x25U});
		assertEqual(cpu.readStatus(), 0x0010U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(cpu.readDataRegister(0U), 0x05a00000U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000010U);
		assertEqual(cpu.readDataRegister(0U), 0x85a00001U);
		assertEqual(cpu.readStatus(), 0x0018U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
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
		CXX_TEST(testRTS)
		CXX_TEST(testRTE)
		CXX_TEST(testADDA)
		CXX_TEST(testOR)
		CXX_TEST(testORI)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testM68k>();
}
