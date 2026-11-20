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

	void testNEG()
	{
		writeAddress(0x000000U, uint16_t{0x4400U}); // neg.b d0
		writeAddress(0x000002U, uint16_t{0x4440U}); // neg.w d0
		writeAddress(0x000004U, uint16_t{0x4480U}); // neg.l d0
		writeAddress(0x000006U, uint16_t{0x4458U}); // neg.w (a0)+
		writeAddress(0x000008U, uint16_t{0x4450U}); // neg.w (a0)
		writeAddress(0x00000aU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint16_t{0x0000U});
		writeAddress(0x000102U, uint16_t{0x8000U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0 and d0 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0xffff00ffU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000eU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(0U), 0xffff0001U);
		assertEqual(cpu.readStatus(), 0x0011U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0xffffffffU);
		assertEqual(cpu.readStatus(), 0x0019U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(0U), 0x00000001U);
		assertEqual(cpu.readStatus(), 0x0011U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(readAddress<uint16_t>(0x000100U), uint16_t{0x0000U});
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(readAddress<uint16_t>(0x000102U), uint16_t{0x8000U});
		assertEqual(cpu.readStatus(), 0x001bU);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testNOP()
	{
		writeAddress(0x000000U, uint16_t{0x4e71U}); // nop
		writeAddress(0x000002U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
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
		writeAddress(0x00000cU, uint32_t{0x80000001U}); // ori.l #$80000001, d0
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

	void testORISpecial()
	{
		writeAddress(0x000000U, uint16_t{0x003cU});
		writeAddress(0x000002U, uint16_t{0x000aU}); // ori #$0a, ccr
		writeAddress(0x000004U, uint16_t{0x003cU});
		writeAddress(0x000006U, uint16_t{0xff15U}); // ori #$15, ccr w/ intentional value misencode
		writeAddress(0x000008U, uint16_t{0x007cU});
		writeAddress(0x00000aU, uint16_t{0x070eU}); // ori #$070e, sr
		writeAddress(0x00000cU, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Set the status register to some improbable value that makes it easy to how it changes
		cpu.writeStatus(0x0000U);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readStatus(), 0x000aU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Switch the processor into supervisor mode
		cpu.writeStatus(0x2000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readStatus(), 0x270eU);
		// Switch the processor back out into user mode
		cpu.writeStatus(0x0000U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);

		// This sequence should trap
		writeAddress(0x000000U, uint16_t{0x007cU});
		writeAddress(0x000002U, uint16_t{0x2705U}); // ori #$2705, sr
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep(true);
	}

	void testScc()
	{
		writeAddress(0x000000U, uint16_t{0x50c0U}); // st d0
		writeAddress(0x000002U, uint16_t{0x51c0U}); // sf d0
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0 so we can observe which bits change with the condition tests
		cpu.writeDataRegister(0U, 0x00000000U);
		// Set up the status bits in a way where it's trivial to observe if they change
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(0U), 0x000000ffU);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Switch up the status bits in a way where it's trivial to observe if they change
		cpu.writeStatus(0x0010U);
	}

	void testSUB()
	{
		writeAddress(0x000000U, uint16_t{0x9590U}); // sub.l d2, (a0)
		writeAddress(0x000002U, uint16_t{0x9088U}); // sub.l a0, d0
		writeAddress(0x000004U, uint16_t{0x9030U});
		writeAddress(0x000006U, uint16_t{0x8800U}); // sub.b (a0, a0.l), d0
		writeAddress(0x000008U, uint16_t{0x9070U});
		writeAddress(0x00000aU, uint16_t{0x1204U}); // sub.w 4(a0, d1.w*2), d0
		writeAddress(0x00000cU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint32_t{0U});
		writeAddress(0x000108U, uint16_t{0x8080U});
		writeAddress(0x000200U, uint8_t{0xc1U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d0, d1 and d2 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0x40008145U);
		cpu.writeDataRegister(1U, 0x00000002U);
		cpu.writeDataRegister(2U, 0x00000000U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(readAddress<uint32_t>(0x00000100U), 0U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0x40008045U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(0U), 0x40008084U);
		assertEqual(cpu.readStatus(), 0x001bU);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readDataRegister(0U), 0x40000004U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testSUBA()
	{
		writeAddress(0x000000U, uint16_t{0x90e1U}); // suba.w -(a1), a0
		writeAddress(0x000002U, uint16_t{0x93c0U}); // suba.l d0, a1
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
		assertEqual(cpu.readAddrRegister(0U), 0x00003f10U);
		assertEqual(cpu.readAddrRegister(1U), 0x00000100U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readAddrRegister(1U), 0xc00000fcU);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testSUBI()
	{
		writeAddress(0x000000U, uint16_t{0x0418U});
		writeAddress(0x000002U, uint16_t{0x0004U}); // subi.b #4, (a0)+
		writeAddress(0x000004U, uint16_t{0x0458U});
		writeAddress(0x000006U, uint16_t{0x8001U}); // subi.w #$8001, (a0)+
		writeAddress(0x000008U, uint16_t{0x0498U});
		writeAddress(0x00000aU, uint32_t{0x7fffffffU}); // subi.l #$7fffffff, (a0)+
		writeAddress(0x00000eU, uint16_t{0x0428U});
		writeAddress(0x000010U, uint16_t{0x003cU});
		writeAddress(0x000012U, uint16_t{0xfff9U}); // subi.b #$3c, -7(a0)
		writeAddress(0x000014U, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint8_t{0x40U});
		writeAddress(0x000101U, uint16_t{0x7ffeU});
		writeAddress(0x000103U, uint32_t{0xfeed4ca7U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0 to point to the data pool at +0x0100U
		cpu.writeAddrRegister(0U, 0x00000100U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readAddrRegister(0U), 0x00000101U);
		assertEqual(readAddress<uint8_t>(0x00000100U), 0x3cU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readAddrRegister(0U), 0x00000103U);
		assertEqual(readAddress<uint16_t>(0x00000101U), 0xfffdU);
		assertEqual(cpu.readStatus(), 0x001bU);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000eU);
		assertEqual(cpu.readAddrRegister(0U), 0x00000107U);
		assertEqual(readAddress<uint32_t>(0x00000103U), 0x7eed4ca8U);
		assertEqual(cpu.readStatus(), 0x0002U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000014U);
		assertEqual(readAddress<uint8_t>(0x00000100U), 0x00U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testSUBQ()
	{
		writeAddress(0x000000U, uint16_t{0x5183U}); // subq.l #8, d3
		writeAddress(0x000002U, uint16_t{0x5948U}); // subq.w #4, a0
		writeAddress(0x000004U, uint16_t{0x5b04U}); // subq.b #5, d4
		writeAddress(0x000006U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d3 and d4 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(3U, 0x80000000U);
		cpu.writeDataRegister(4U, 0x00000004U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(3U), 0x7ffffff8U);
		assertEqual(cpu.readStatus(), 0x0002U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readAddrRegister(0U), 0x000000fcU);
		// As this is done to an address register, the status flags should not change
		assertEqual(cpu.readStatus(), 0x0002U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(4U), 0x000000ffU);
		assertEqual(cpu.readStatus(), 0x0019U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testSWAP()
	{
		writeAddress(0x000000U, uint16_t{0x4840U}); // swap d0
		writeAddress(0x000002U, uint16_t{0x4841U}); // swap d1
		writeAddress(0x000004U, uint16_t{0x4842U}); // swap d2
		writeAddress(0x000006U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0, d1 and d2 to sensible values
		cpu.writeDataRegister(0U, 0x00000000U);
		cpu.writeDataRegister(1U, 0x12345678U);
		cpu.writeDataRegister(2U, 0x0ca1f00dU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(1U), 0x56781234U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(2U), 0xf00d0ca1U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testTRAP()
	{
		writeAddress(0x000000U, uint16_t{0x4e40U}); // trap #0
		writeAddress(0x000002U, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000004U, uint16_t{0x4e44U}); // trap #4
		writeAddress(0x000006U, uint16_t{0x4e73U}); // rte
		// TRAP handler vector pointers
		writeAddress(0x000080U, uint32_t{0x00000004U});
		writeAddress(0x000090U, uint32_t{0x00000006U});
		// Set up the supervisor-mode stack pointer
		cpu.writeStatus(0x2000U);
		cpu.writeAddrRegister(7U, 0x00700000U);
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readAddrRegister(7U), 0x006ffff8U);
		assertEqual(cpu.readStatus(), 0x2000U);
		assertEqual(readAddress<uint16_t>(0x006ffffeU), uint16_t{0x0080U}); // Frame type and vector info
		assertEqual(readAddress<uint32_t>(0x006ffffaU), uint32_t{0x00000002U}); // Return program counter
		assertEqual(readAddress<uint16_t>(0x006ffff8U), uint16_t{0x0000U}); // Return status register
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readAddrRegister(7U), 0x006ffff0U);
		assertEqual(cpu.readStatus(), 0x2000U);
		assertEqual(readAddress<uint16_t>(0x006ffff6U), uint16_t{0x0090U}); // Frame type and vector info
		assertEqual(readAddress<uint32_t>(0x006ffff2U), uint32_t{0x00000006U}); // Return program counter
		assertEqual(readAddress<uint16_t>(0x006ffff0U), uint16_t{0x2000U}); // Return status register
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readAddrRegister(7U), 0x006ffff8U);
		assertEqual(cpu.readStatus(), 0x2000U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
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
		CXX_TEST(testNEG)
		CXX_TEST(testNOP)
		CXX_TEST(testOR)
		CXX_TEST(testORI)
		CXX_TEST(testORISpecial)
		CXX_TEST(testScc)
		CXX_TEST(testSUB)
		CXX_TEST(testSUBA)
		CXX_TEST(testSUBI)
		CXX_TEST(testSUBQ)
		CXX_TEST(testSWAP)
		CXX_TEST(testTRAP)
		CXX_TEST(testTST)
		CXX_TEST(testDisplayRegs)
	}
};

CRUNCHpp_TESTS(testM68k)
