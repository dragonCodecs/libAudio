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

	void testJSR()
	{
		writeAddress(0x000000U, uint16_t{0x4e90U}); // jsr (a0)
		writeAddress(0x000002U, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint16_t{0x4e75U}); // rts to return from JSR
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0 to point to the "subroutine"
		cpu.writeAddrRegister(0U, 0x00000100U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000100U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffff8U);
		assertEqual(cpu.readStatus(), 0x001fU);
		assertEqual(readAddress<uint32_t>(0x007ffff8U), 0x00000002U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testLEA()
	{
		writeAddress(0x000000U, uint16_t{0x43d0U}); // lea (a0), a1
		writeAddress(0x000002U, uint16_t{0x43e8U});
		writeAddress(0x000004U, uint16_t{0xfeedU}); // lea $feed(a0), a1
		writeAddress(0x000006U, uint16_t{0x43f0U});
		writeAddress(0x000008U, uint16_t{0x0c10U}); // lea $10(a0, d0.l*4), a1
		writeAddress(0x00000aU, uint16_t{0x43f8U});
		writeAddress(0x00000cU, uint16_t{0xca15U}); // lea ($ca15).w, a1
		writeAddress(0x00000eU, uint16_t{0x43f9U});
		writeAddress(0x000010U, uint32_t{0x0badf00dU}); // lea ($0badf00d).l, a1
		writeAddress(0x000014U, uint16_t{0x43faU});
		writeAddress(0x000016U, uint16_t{0xfffaU}); // lea -6(pc), a1
		writeAddress(0x000018U, uint16_t{0x43fbU});
		writeAddress(0x00001aU, uint16_t{0x0727U});
		writeAddress(0x00001cU, uint16_t{0x0026U});
		writeAddress(0x00001eU, uint32_t{0xf00daca1U}); // lea $f00daca1($26[pc], d0.w*8), a1
		writeAddress(0x000022U, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000040U, uint32_t{0xdeadbeefU});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0 and d0 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0x00000200U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readAddrRegister(1U), 0x00000100U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readAddrRegister(1U), 0xffffffedU);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(cpu.readAddrRegister(1U), 0x00000910U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000eU);
		assertEqual(cpu.readAddrRegister(1U), 0xffffca15U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000014U);
		assertEqual(cpu.readAddrRegister(1U), 0x0badf00dU);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the sixth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000018U);
		assertEqual(cpu.readAddrRegister(1U), 0x00000010U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the seventh instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000022U);
		// 0xdeadbeefU + 0x00000200U * 8U + 0xf00daca1U
		assertEqual(cpu.readAddrRegister(1U), 0xcebb7b90U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testLSL()
	{
		writeAddress(0x000000U, uint16_t{0xe509U}); // lsl.b #2, d1
		writeAddress(0x000002U, uint16_t{0xe169U}); // lsl.w d0, d1
		writeAddress(0x000004U, uint16_t{0xe529U}); // lsl.b d2, d1
		writeAddress(0x000006U, uint16_t{0xe189U}); // lsl.l #8, d1
		writeAddress(0x000008U, uint16_t{0xe3d0U}); // lsl (a0)
		writeAddress(0x00000aU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint16_t{0x55aaU});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d0, d1 and d2 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0x00000004U);
		cpu.writeDataRegister(1U, 0x00000060U);
		cpu.writeDataRegister(2U, 0x00000000U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x0006U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(1U), 0x00000080U);
		assertEqual(cpu.readStatus(), 0x0019U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(1U), 0x00000800U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001bU);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(1U), 0x00000800U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(1U), 0x00080000U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(readAddress<uint16_t>(0x000100U), uint16_t{0xab54U});
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testLSR()
	{
		writeAddress(0x000000U, uint16_t{0xe409U}); // lsr.b #2, d1
		writeAddress(0x000002U, uint16_t{0xe069U}); // lsr.w d0, d1
		writeAddress(0x000004U, uint16_t{0xe429U}); // lsr.b d2, d1
		writeAddress(0x000006U, uint16_t{0xe089U}); // lsr.l #8, d1
		writeAddress(0x000008U, uint16_t{0xe2d0U}); // lsr (a0)
		writeAddress(0x00000aU, uint16_t{0xe029U}); // lsr.b d0, d1
		writeAddress(0x00000cU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint16_t{0x55aaU});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d0, d1 and d2 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0x00000004U);
		cpu.writeDataRegister(1U, 0x01008806U);
		cpu.writeDataRegister(2U, 0x00000000U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000eU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(1U), 0x01008801U);
		assertEqual(cpu.readStatus(), 0x0011U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(1U), 0x01000880U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x0017U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(1U), 0x01000880U);
		assertEqual(cpu.readStatus(), 0x0018U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(1U), 0x00010008U);
		assertEqual(cpu.readStatus(), 0x0011U);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(readAddress<uint16_t>(0x000100U), uint16_t{0x2ad5U});
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the sixth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readDataRegister(1U), 0x00010000U);
		assertEqual(cpu.readStatus(), 0x0015U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testMOVE()
	{
		writeAddress(0x000000U, uint16_t{0x103cU});
		writeAddress(0x000002U, uint16_t{0x0048U}); // move.b #$48, d0
		writeAddress(0x000004U, uint16_t{0x303cU});
		writeAddress(0x000006U, uint16_t{0x0000U}); // move.w #0, d0
		writeAddress(0x000008U, uint16_t{0x203cU});
		writeAddress(0x00000aU, uint32_t{0xff000000U}); // move.w #$ff000000, d0
		writeAddress(0x00000eU, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0x00000048U);
		assertEqual(cpu.readStatus(), 0x0010U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Set the status register to some other value that makes it easy to see if it changes
		cpu.writeStatus(0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000eU);
		assertEqual(cpu.readDataRegister(0U), 0xff000000U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testMOVESpecialCCR()
	{
		writeAddress(0x000000U, uint16_t{0x44c0U}); // move d0, ccr
		writeAddress(0x000002U, uint16_t{0x42c2U}); // move ccr, d2
		writeAddress(0x000004U, uint16_t{0x44c1U}); // move d1, ccr
		writeAddress(0x000006U, uint16_t{0x42c2U}); // move ccr, d2
		writeAddress(0x000008U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence in user mode
		cpu.writeStatus(0x0000U);
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Set up d0 and d1 to sensible values
		cpu.writeDataRegister(0U, 0x0000270fU);
		cpu.writeDataRegister(1U, 0x00000015U);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readStatus(), 0x000fU);
		// Step the second instruction and validate,
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readStatus(), 0x000fU);
		assertEqual(cpu.readDataRegister(2U), 0x0000000fU);
		// Switch into supervisor mode
		cpu.writeStatus(0x2000);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readStatus(), 0x2015U);
		// Step the fourth instruction and validate,
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readStatus(), 0x2015U);
		assertEqual(cpu.readDataRegister(2U), 0x00000015U);
		// Switch back into user mode
		cpu.writeStatus(0x0000);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testMOVESpecialSR()
	{
		writeAddress(0x000000U, uint16_t{0x46c0U}); // move d0, sr
		writeAddress(0x000002U, uint16_t{0x40c2U}); // move sr, d2
		writeAddress(0x000004U, uint16_t{0x46c1U}); // move d1, sr
		writeAddress(0x000006U, uint16_t{0x40c2U}); // move sr, d2
		writeAddress(0x000008U, uint16_t{0x46c0U}); // move d0, sr
		// Set the CPU to execute this sequence in supervisor mode
		cpu.writeStatus(0x0000U);
		cpu.executeFrom(0x00000000U, 0x00800000U, false);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x2000U);
		// Set up d0 and d1 to sensible values
		cpu.writeDataRegister(0U, 0x0000270fU);
		cpu.writeDataRegister(1U, 0x00000015U);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readStatus(), 0x270fU);
		// Step the second instruction and validate,
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readStatus(), 0x270fU);
		assertEqual(cpu.readDataRegister(2U), 0x0000270fU);
		// Step the third instruction and validate
		// this transitions us out of supervisor mode
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readStatus(), 0x0015U);
		// Step the fourth instruction and validate,
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readStatus(), 0x0015U);
		assertEqual(cpu.readDataRegister(2U), 0x0015U);
		// Step the fifth instruction and validate (this should trap)
		runStep(true);
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(cpu.readStatus(), 0x0015U);
	}

	void testMOVESpecialUSP()
	{
		writeAddress(0x000000U, uint16_t{0x4e60U}); // move a0, usp
		writeAddress(0x000002U, uint16_t{0x4e69U}); // move usp, a1
		writeAddress(0x000004U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence in supervisor mode
		cpu.writeStatus(0x0000U);
		cpu.executeFrom(0x00000000U, 0x00800000U, false);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x2000U);
		// Set up a0 to a sensible value
		cpu.writeAddrRegister(0U, 0x1234abcdU);
		// Step the first instruction and validate
		runStep();
		// Switch into user mode
		cpu.writeStatus(0x0000);
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readAddrRegister(7U), 0x1234abcdU);
		// Update the USP with a new value then switch back into supervisor mode
		cpu.writeAddrRegister(7U, 0xdeadbeefU);
		cpu.writeStatus(0x2000);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readAddrRegister(1U), 0xdeadbeefU);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);

		// Set the CPU Uback up to execute the same sequence in user mode
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the first instruction and validate that it traps
		runStep(true);
	}

	void testMOVEA()
	{
		writeAddress(0x000000U, uint16_t{0x3048U}); // movea.w a0, a0
		writeAddress(0x000002U, uint16_t{0x2040U}); // movea.l d0, a0
		writeAddress(0x000004U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0 and d0 to sensible values
		cpu.writeAddrRegister(0U, 0xfeedaca7U);
		cpu.writeDataRegister(0U, 0x01248421U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readAddrRegister(0U), 0xffffaca7U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Set the status register to some other value that makes it easy to see if it changes
		cpu.writeStatus(0x0000U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readAddrRegister(0U), 0x01248421U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testMOVEM()
	{
		writeAddress(0x000000U, uint16_t{0x48f8U});
		writeAddress(0x000002U, uint16_t{0x00ffU});
		writeAddress(0x000004U, uint16_t{0x0100U}); // movem.l d0-d7, ($0100).w
		writeAddress(0x000006U, uint16_t{0x48f8U});
		writeAddress(0x000008U, uint16_t{0xff00U});
		writeAddress(0x00000aU, uint16_t{0x0100U}); // movem.l a0-a7, ($0100).w
		writeAddress(0x00000cU, uint16_t{0x48a7U});
		writeAddress(0x00000eU, uint16_t{0xc0c0U}); // movem.w d0-d1/a0-a1, -(sp)
		writeAddress(0x000010U, uint16_t{0x4c9fU});
		writeAddress(0x000012U, uint16_t{0x001eU}); // movem.w +(sp), d1-d4
		writeAddress(0x000014U, uint16_t{0x3043U}); // movea.w d3, a0
		writeAddress(0x000016U, uint16_t{0x4cd8U});
		writeAddress(0x000018U, uint16_t{0x2418U}); // movem.l +(a0), d3-d4/a2,a5
		writeAddress(0x00001aU, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up all the data registers so it's easy to tell which one got put where
		cpu.writeDataRegister(0U, 0x00010001U);
		cpu.writeDataRegister(1U, 0x00020002U);
		cpu.writeDataRegister(2U, 0x00030003U);
		cpu.writeDataRegister(3U, 0x00040004U);
		cpu.writeDataRegister(4U, 0x00050005U);
		cpu.writeDataRegister(5U, 0x00060006U);
		cpu.writeDataRegister(6U, 0x00070007U);
		cpu.writeDataRegister(7U, 0x00080008U);
		// Set up all the address registers so it's easy to tell which one got put where
		cpu.writeAddrRegister(0U, 0x01000100U);
		cpu.writeAddrRegister(1U, 0x02000200U);
		cpu.writeAddrRegister(2U, 0x03000300U);
		cpu.writeAddrRegister(3U, 0x04000400U);
		cpu.writeAddrRegister(4U, 0x05000500U);
		cpu.writeAddrRegister(5U, 0x06000600U);
		cpu.writeAddrRegister(6U, 0x07000700U);
		// NB, we skip a7 as that's the stack pointer and we know what value that has already.
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(readAddress<uint32_t>(0x000100U), 0x00010001U);
		assertEqual(readAddress<uint32_t>(0x000104U), 0x00020002U);
		assertEqual(readAddress<uint32_t>(0x000108U), 0x00030003U);
		assertEqual(readAddress<uint32_t>(0x00010cU), 0x00040004U);
		assertEqual(readAddress<uint32_t>(0x000110U), 0x00050005U);
		assertEqual(readAddress<uint32_t>(0x000114U), 0x00060006U);
		assertEqual(readAddress<uint32_t>(0x000118U), 0x00070007U);
		assertEqual(readAddress<uint32_t>(0x00011cU), 0x00080008U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(readAddress<uint32_t>(0x000100U), 0x01000100U);
		assertEqual(readAddress<uint32_t>(0x000104U), 0x02000200U);
		assertEqual(readAddress<uint32_t>(0x000108U), 0x03000300U);
		assertEqual(readAddress<uint32_t>(0x00010cU), 0x04000400U);
		assertEqual(readAddress<uint32_t>(0x000110U), 0x05000500U);
		assertEqual(readAddress<uint32_t>(0x000114U), 0x06000600U);
		assertEqual(readAddress<uint32_t>(0x000118U), 0x07000700U);
		assertEqual(readAddress<uint32_t>(0x00011cU), 0x007ffffcU);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000010U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffff4U);
		assertEqual(readAddress<uint16_t>(0x7ffffaU), 0x0200U);
		assertEqual(readAddress<uint16_t>(0x7ffff8U), 0x0100U);
		assertEqual(readAddress<uint16_t>(0x7ffff6U), 0x0002U);
		assertEqual(readAddress<uint16_t>(0x7ffff4U), 0x0001U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000014U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readDataRegister(1U), 0x00000001U);
		assertEqual(cpu.readDataRegister(2U), 0x00000002U);
		assertEqual(cpu.readDataRegister(3U), 0x00000100U);
		assertEqual(cpu.readDataRegister(4U), 0x00000200U);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000016U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readAddrRegister(0U), 0x00000100U);
		// Step the sixth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000001aU);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readAddrRegister(0U), 0x00000110U);
		assertEqual(cpu.readDataRegister(3U), 0x01000100U);
		assertEqual(cpu.readDataRegister(4U), 0x02000200U);
		assertEqual(cpu.readAddrRegister(2U), 0x03000300U);
		assertEqual(cpu.readAddrRegister(5U), 0x04000400U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testMOVEP()
	{
		writeAddress(0x000000U, uint16_t{0x0188U});
		writeAddress(0x000002U, uint16_t{0x0000U}); // movep.w d0, $0(a0)
		writeAddress(0x000004U, uint16_t{0x01c8U});
		writeAddress(0x000006U, uint16_t{0x0011U}); // movep.l d0, $11(a0)
		writeAddress(0x000008U, uint16_t{0x0308U});
		writeAddress(0x00000aU, uint16_t{0x0000U}); // movep.w $0(a0), d1
		writeAddress(0x00000cU, uint16_t{0x0548U});
		writeAddress(0x00000eU, uint16_t{0x0010U}); // movep.l $10(a0), d2
		writeAddress(0x000010U, uint16_t{0x0548U});
		writeAddress(0x000012U, uint16_t{0x0011U}); // movep.l $11(a0), d2
		writeAddress(0x000014U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0 and d0 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0xfeedaca7U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(readAddress<uint16_t>(0x000100U), 0xac00U);
		assertEqual(readAddress<uint16_t>(0x000102U), 0xa700U);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Set the status register to some other value that makes it easy to see if it changes
		cpu.writeStatus(0x0000U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(readAddress<uint16_t>(0x000110U), 0x00feU);
		assertEqual(readAddress<uint16_t>(0x000112U), 0x00edU);
		assertEqual(readAddress<uint16_t>(0x000114U), 0x00acU);
		assertEqual(readAddress<uint16_t>(0x000116U), 0x00a7U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readDataRegister(1U), 0x0000aca7U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000010U);
		assertEqual(cpu.readDataRegister(2U), 0x00000000U);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000014U);
		assertEqual(cpu.readDataRegister(2U), 0xfeedaca7U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testMOVEQ()
	{
		writeAddress(0x000000U, uint16_t{0x7080U}); // moveq #$80, d0
		writeAddress(0x000002U, uint16_t{0x7000U}); // moveq #$00, d0
		writeAddress(0x000004U, uint16_t{0x705aU}); // moveq #$5a, d0
		writeAddress(0x000006U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Zero d0 so it's easy to see if the first move works properly
		cpu.writeDataRegister(0U, 0x00000000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x0017U);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(0U), 0xffffff80U);
		assertEqual(cpu.readStatus(), 0x0018U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000bU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(0U), 0x0000005aU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testMULS()
	{
		writeAddress(0x000000U, uint16_t{0xc1fcU});
		writeAddress(0x000002U, uint16_t{0x0003U}); // muls.w #3, d0
		writeAddress(0x000004U, uint16_t{0xc3c0U}); // muls.w d0, d1
		writeAddress(0x000006U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0 and d1 to sensible values
		cpu.writeDataRegister(0U, 0x0ca78132U);
		cpu.writeDataRegister(1U, 0x00010000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0xfffe8396U);
		assertEqual(cpu.readStatus(), 0x0018U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x0001U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(1U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testMULU()
	{
		writeAddress(0x000000U, uint16_t{0xc0fcU});
		writeAddress(0x000002U, uint16_t{0x0003U}); // mulu.w #3, d0
		writeAddress(0x000004U, uint16_t{0xc2c0U}); // mulu.w d0, d1
		writeAddress(0x000006U, uint16_t{0xc4c2U}); // mulu.w d2, d2
		writeAddress(0x000008U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0, d1, and d2 to sensible values
		cpu.writeDataRegister(0U, 0x0ca78132U);
		cpu.writeDataRegister(1U, 0x00010000U);
		cpu.writeDataRegister(2U, 0x0000ffffU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0x00018396U);
		assertEqual(cpu.readStatus(), 0x0010U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x0001U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(1U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(2U), 0xfffe0001U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
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
		CXX_TEST(testJSR)
		CXX_TEST(testLEA)
		CXX_TEST(testLSL)
		CXX_TEST(testLSR)
		CXX_TEST(testMOVE)
		CXX_TEST(testMOVESpecialCCR)
		CXX_TEST(testMOVESpecialSR)
		CXX_TEST(testMOVESpecialUSP)
		CXX_TEST(testMOVEA)
		CXX_TEST(testMOVEM)
		CXX_TEST(testMOVEP)
		CXX_TEST(testMOVEQ)
		CXX_TEST(testMULS)
		CXX_TEST(testMULU)
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
