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

	void testBRA()
	{
		// Set up 3 branches in each of the 3 instruction encoding forms, checking they jump properly to each other
		// Start with the 8-bit immediate form, then use the 16-bit immediate form, and finally the 32-bit immediate
		// form (6 bytes to encode)
		writeAddress(0x000100U, uint16_t{0x6050U}); // Jump to +0x50 from end of instruction
		writeAddress(0x000152U, uint16_t{0x6000U});
		writeAddress(0x000154U, uint16_t{0xfefcU}); // Jump to -0x104 from end of instruction
		writeAddress(0x000050U, uint16_t{0x60ffU});
		writeAddress(0x000052U, uint32_t{0x000100aeU}); // Jump to +0x100ae from end of instruction
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

	void testBSR()
	{
		// Set up 3 branches in each of the 3 instruction encoding forms, checking they jump properly and then return
		// Start with the 8-bit immediate form, then use the 16-bit immediate form, and finally the 32-bit immediate
		// form (6 bytes to encode)
		writeAddress(0x000200U, uint16_t{0x6150U}); // Jump to +0x50 from end of instruction
		writeAddress(0x000202U, uint16_t{0x6100U});
		writeAddress(0x000204U, uint16_t{0xfefcU}); // Jump to -0x104 from end of instruction u16
		writeAddress(0x000206U, uint16_t{0x61ffU});
		writeAddress(0x000208U, uint32_t{0x000100aeU}); // Jump to +0x100ae from end of instruction u16
		writeAddress(0x00020cU, uint16_t{0x4e75U}); // rts to end the test
		// RTS's for each of the BSRs to hit
		writeAddress(0x000252U, uint16_t{0x4e75U});
		writeAddress(0x000100U, uint16_t{0x4e75U});
		writeAddress(0x0102b6U, uint16_t{0x4e75U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000200U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000200U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000252U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffff8U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the RTS for that and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000202U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000100U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffff8U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the RTS for that and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000206U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x000102b6U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffff8U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the RTS for that and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000020cU);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the fourth and final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
		assertEqual(cpu.readStatus(), 0x0000U);
	}

	void testJMP()
	{
		// NB: We don't test all EA's here as we get through them in other tests.
		writeAddress(0x000000U, uint16_t{0x4ed0U}); // jmp (a0)
		writeAddress(0x000004U, uint16_t{0x4ee8U});
		writeAddress(0x000006U, uint16_t{0x0100U}); // jmp $0100(a0)
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

	void testADD()
	{
		writeAddress(0x000000U, uint16_t{0xd070U});
		writeAddress(0x000002U, uint16_t{0x1204U}); // add.w 4(a0, d1.w*2), d0
		writeAddress(0x000004U, uint16_t{0xd030U});
		writeAddress(0x000006U, uint16_t{0x8800U}); // add.b (a0, a0.l), d0
		writeAddress(0x000008U, uint16_t{0xd088U}); // add.l a0, d0
		writeAddress(0x00000aU, uint16_t{0xd590U}); // add.l d2, (a0)
		writeAddress(0x00000cU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint32_t{0U});
		writeAddress(0x000108U, uint16_t{0x8080U});
		writeAddress(0x000200U, uint8_t{0xc1U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d0, d1 and d2 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0x40000004U);
		cpu.writeDataRegister(1U, 0x00000002U);
		cpu.writeDataRegister(2U, 0x00000000U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0x40008084U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(0U), 0x40008045U);
		assertEqual(cpu.readStatus(), 0x0013U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(cpu.readDataRegister(0U), 0x40008145U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(readAddress<uint32_t>(0x00000100U), 0U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
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

	void testADDI()
	{
		writeAddress(0x000000U, uint16_t{0x0618U});
		writeAddress(0x000002U, uint16_t{0x0004U}); // addi.b #4, (a0)+
		writeAddress(0x000004U, uint16_t{0x0658U});
		writeAddress(0x000006U, uint16_t{0x8001U}); // addi.w #$8001, (a0)+
		writeAddress(0x000008U, uint16_t{0x0698U});
		writeAddress(0x00000aU, uint32_t{0xfeed4ca7U}); // addi.l #$feed4ca7, (a0)+
		writeAddress(0x00000eU, uint16_t{0x0628U});
		writeAddress(0x000010U, uint16_t{0x00bcU});
		writeAddress(0x000012U, uint16_t{0xfff9U}); // addi.b #$bc, -7(a0)
		writeAddress(0x000014U, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint8_t{0x40U});
		writeAddress(0x000101U, uint16_t{0x7ffeU});
		writeAddress(0x000103U, uint32_t{0x80000001U});
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
		assertEqual(readAddress<uint8_t>(0x00000100U), 0x44U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readAddrRegister(0U), 0x00000103U);
		assertEqual(readAddress<uint16_t>(0x00000101U), 0xffffU);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000eU);
		assertEqual(cpu.readAddrRegister(0U), 0x00000107U);
		assertEqual(readAddress<uint32_t>(0x00000103U), 0x7eed4ca8U);
		assertEqual(cpu.readStatus(), 0x0013U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000014U);
		assertEqual(readAddress<uint8_t>(0x00000100U), 0x00U);
		assertEqual(cpu.readStatus(), 0x0015U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testADDQ()
	{
		writeAddress(0x000000U, uint16_t{0x5a04U}); // addq.b #5, d4
		writeAddress(0x000002U, uint16_t{0x5848U}); // addq.w #4, a0
		writeAddress(0x000004U, uint16_t{0x5083U}); // addq.l #8, d3
		writeAddress(0x000006U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d3 and d4 to sensible values
		cpu.writeAddrRegister(0U, 0x000000fcU);
		cpu.writeDataRegister(3U, 0x7ffffff8U);
		cpu.writeDataRegister(4U, 0x000000ffU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(4U), 0x00000004U);
		assertEqual(cpu.readStatus(), 0x0011U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readAddrRegister(0U), 0x00000100U);
		// As this is done to an address register, the status flags should not change
		assertEqual(cpu.readStatus(), 0x0011U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(3U), 0x80000000U);
		assertEqual(cpu.readStatus(), 0x000aU);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testANDI()
	{
		writeAddress(0x000000U, uint16_t{0x0238U});
		writeAddress(0x000002U, uint16_t{0x0024U});
		writeAddress(0x000004U, uint16_t{0x0100U}); // andi.b #$24, $0100.w
		writeAddress(0x000006U, uint16_t{0x0258U});
		writeAddress(0x000008U, uint16_t{0x0000U}); // andi.w #0, (a0)+
		writeAddress(0x00000aU, uint16_t{0x0281U});
		writeAddress(0x00000cU, uint16_t{0x8421U});
		writeAddress(0x00000eU, uint16_t{0x1248U}); // andi.l #$84211248, d1
		writeAddress(0x000010U, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint8_t{0x29U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0 and d1 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(1U, 0xfeedaca7U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(readAddress<uint8_t>(0x000100U), uint8_t{0x20U});
		assertEqual(cpu.readStatus(), 0x0010U);
		// Reset the status register to a different improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(cpu.readAddrRegister(0U), 0x00000102U);
		assertEqual(readAddress<uint16_t>(0x000100U), uint16_t{0U});
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000010U);
		assertEqual(cpu.readDataRegister(1U), 0x84210000U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testANDISpecial()
	{
		writeAddress(0x000000U, uint16_t{0x023cU});
		writeAddress(0x000002U, uint16_t{0x00ffU}); // andi #$ff, ccr
		writeAddress(0x000004U, uint16_t{0x023cU});
		writeAddress(0x000006U, uint16_t{0x0005U}); // andi #$05, ccr
		writeAddress(0x000008U, uint16_t{0x027cU});
		writeAddress(0x00000aU, uint16_t{0xf00fU}); // andi #$f00f, sr
		writeAddress(0x00000cU, uint16_t{0x027cU});
		writeAddress(0x00000eU, uint16_t{0x070aU}); // andi #$070a, sr
		writeAddress(0x000010U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Set the status register to some improbable value that makes it easy to how it changes
		cpu.writeStatus(0x271fU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readStatus(), 0x271fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readStatus(), 0x2705U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readStatus(), 0x2005U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000010U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);

		// This sequence should trap
		writeAddress(0x000000U, uint16_t{0x027cU});
		writeAddress(0x000002U, uint16_t{0xffffU}); // andi #$ffff, sr
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep(true);
	}

	void testASL()
	{
		writeAddress(0x000000U, uint16_t{0xe501U}); // asl.b #2, d1
		writeAddress(0x000002U, uint16_t{0xe161U}); // asl.w d0, d1
		writeAddress(0x000004U, uint16_t{0xe521U}); // asl.b d2, d1
		writeAddress(0x000006U, uint16_t{0xe181U}); // asl.l #8, d1
		writeAddress(0x000008U, uint16_t{0xe1d0U}); // asl (a0)
		writeAddress(0x00000aU, uint16_t{0xe903U}); // asl.b #4, d3
		writeAddress(0x00000cU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint16_t{0x55aaU});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d0, d1, d2 and d3 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0x00000004U);
		cpu.writeDataRegister(1U, 0x00000060U);
		cpu.writeDataRegister(2U, 0x00000000U);
		cpu.writeDataRegister(3U, 0x00000045U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x0006U);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(1U), 0x00000080U);
		assertEqual(cpu.readStatus(), 0x001bU);
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
		assertEqual(cpu.readStatus(), 0x0004U); // XXX: X bit should actually be unaffacted by this.
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(1U), 0x00080000U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(readAddress<uint16_t>(0x000100U), uint16_t{0xab54U});
		assertEqual(cpu.readStatus(), 0x000aU);
		// Step the sixth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readDataRegister(3U), 0x00000050U);
		assertEqual(cpu.readStatus(), 0x0002U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testASR()
	{
		writeAddress(0x000000U, uint16_t{0xe401U}); // asr.b #2, d1
		writeAddress(0x000002U, uint16_t{0xe061U}); // asr.w d0, d1
		writeAddress(0x000004U, uint16_t{0xe421U}); // asr.b d2, d1
		writeAddress(0x000006U, uint16_t{0xe081U}); // asr.l #8, d1
		writeAddress(0x000008U, uint16_t{0xe0d0U}); // asr (a0)
		writeAddress(0x00000aU, uint16_t{0xe021U}); // asr.b d0, d1
		writeAddress(0x00000cU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint16_t{0x55aaU});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d0, d1 and d2 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0x00000004U);
		cpu.writeDataRegister(1U, 0x80004806U);
		cpu.writeDataRegister(2U, 0x00000000U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000eU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(1U), 0x80004801U);
		assertEqual(cpu.readStatus(), 0x0011U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(1U), 0x80000480U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x0017U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(1U), 0x80000480U);
		assertEqual(cpu.readStatus(), 0x0018U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(1U), 0xff800004U);
		assertEqual(cpu.readStatus(), 0x0019U);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(readAddress<uint16_t>(0x000100U), uint16_t{0x2ad5U});
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the sixth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readDataRegister(1U), 0xff800000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testBCLR()
	{
		writeAddress(0x000000U, uint16_t{0x0880U});
		writeAddress(0x000002U, uint16_t{0x0002U}); // bclr #2, d0
		writeAddress(0x000004U, uint16_t{0x0380U}); // bclr d1, d0
		writeAddress(0x000006U, uint16_t{0x0390U}); // bclr d1, (a0)
		writeAddress(0x000008U, uint16_t{0x0890U});
		writeAddress(0x00000aU, uint16_t{0x0007U}); // bclr #7, (a0)
		writeAddress(0x00000cU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint8_t{0xa5U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d0 and d1 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0xcafef00fU);
		cpu.writeDataRegister(1U, 0x00000004U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0xcafef00bU);
		assertEqual(cpu.readStatus(), 0x001bU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(0U), 0xcafef00bU);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Reset the status register to a different value that makes it easy to see if it changes
		cpu.writeStatus(0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(readAddress<uint8_t>(0x000100U), uint8_t{0xa5U});
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(readAddress<uint8_t>(0x000100U), uint8_t{0x25U});
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testBSET()
	{
		writeAddress(0x000000U, uint16_t{0x08c0U});
		writeAddress(0x000002U, uint16_t{0x0002U}); // bset #2, d0
		writeAddress(0x000004U, uint16_t{0x03c0U}); // bset d1, d0
		writeAddress(0x000006U, uint16_t{0x03d0U}); // bset d1, (a0)
		writeAddress(0x000008U, uint16_t{0x08d0U});
		writeAddress(0x00000aU, uint16_t{0x0007U}); // bset #7, (a0)
		writeAddress(0x00000cU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint8_t{0xa5U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d0 and d1 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0xcafef00dU);
		cpu.writeDataRegister(1U, 0x00000004U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0xcafef00dU);
		assertEqual(cpu.readStatus(), 0x001bU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(0U), 0xcafef01dU);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Reset the status register to a different value that makes it easy to see if it changes
		cpu.writeStatus(0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(readAddress<uint8_t>(0x000100U), uint8_t{0xb5U});
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(readAddress<uint8_t>(0x000100U), uint8_t{0xb5U});
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testBTST()
	{
		writeAddress(0x000000U, uint16_t{0x0800U});
		writeAddress(0x000002U, uint16_t{0x0002U}); // btst #2, d0
		writeAddress(0x000004U, uint16_t{0x0300U}); // btst d1, d0
		writeAddress(0x000006U, uint16_t{0x0310U}); // btst d1, (a0)
		writeAddress(0x000008U, uint16_t{0x0810U});
		writeAddress(0x00000aU, uint16_t{0x0007U}); // btst #7, (a0)
		writeAddress(0x00000cU, uint16_t{0x4e75U}); // rts to end the test
		writeAddress(0x000100U, uint8_t{0xa5U});
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up a0, d0 and d1 to sensible values
		cpu.writeAddrRegister(0U, 0x00000100U);
		cpu.writeDataRegister(0U, 0xcafef00dU);
		cpu.writeDataRegister(1U, 0x00000004U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0xcafef00dU);
		assertEqual(cpu.readStatus(), 0x001bU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(0U), 0xcafef00dU);
		assertEqual(cpu.readStatus(), 0x001fU);
		// Reset the status register to a different value that makes it easy to see if it changes
		cpu.writeStatus(0x0000U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(readAddress<uint8_t>(0x000100U), uint8_t{0xa5U});
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(readAddress<uint8_t>(0x000100U), uint8_t{0xa5U});
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testCLR()
	{
		writeAddress(0x000000U, uint16_t{0x4200U}); // clr.b d0
		writeAddress(0x000002U, uint16_t{0x4240U}); // clr.w d0
		writeAddress(0x000004U, uint16_t{0x4280U}); // clr.l d0
		writeAddress(0x000006U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0 to a sensible value
		cpu.writeDataRegister(0U, 0xfeedaca7U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001bU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(0U), 0xfeedac00U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Reset the status register to a different improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0xfeed0000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testCMP()
	{
		writeAddress(0x000000U, uint16_t{0xb0bcU});
		writeAddress(0x000002U, uint32_t{0x0badf00dU}); // cmp.l #$0badf00d, d0
		writeAddress(0x000006U, uint16_t{0xb03cU});
		writeAddress(0x000008U, uint16_t{0x007fU}); // cmp.b #$7f, d0
		writeAddress(0x00000aU, uint16_t{0xb07cU});
		writeAddress(0x00000cU, uint16_t{0xaca7U}); // cmp.w #$aca7, d0
		writeAddress(0x00000eU, uint16_t{0xb401U}); // cmp.b d1, d2
		writeAddress(0x000010U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0, d1 and d2 to sensible values
		cpu.writeDataRegister(0U, 0x0badf00dU);
		cpu.writeDataRegister(1U, 0x00000001U);
		cpu.writeDataRegister(2U, 0x00000080U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001bU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(cpu.readStatus(), 0x0009U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000eU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000010U);
		assertEqual(cpu.readStatus(), 0x0002U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testCMPA()
	{
		writeAddress(0x000000U, uint16_t{0xb1fcU});
		writeAddress(0x000002U, uint32_t{0x0badf00dU}); // cmpa.l #$0badf00d, a0
		writeAddress(0x000006U, uint16_t{0xb2fcU});
		writeAddress(0x000008U, uint16_t{0x007fU}); // cmpa.w #$7f, a1
		writeAddress(0x00000aU, uint16_t{0xb6fcU});
		writeAddress(0x00000cU, uint16_t{0xaca7U}); // cmpa.w #$aca7, a3
		writeAddress(0x00000eU, uint16_t{0xb4c1U}); // cmpa.w d1, a2
		writeAddress(0x000010U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d1, a0, a1, a2 and a3 to sensible values
		cpu.writeDataRegister(1U, 0x00000001U);
		cpu.writeAddrRegister(0U, 0x0badf00dU);
		cpu.writeAddrRegister(1U, 0x0000000dU);
		cpu.writeAddrRegister(2U, 0x00008000U);
		cpu.writeAddrRegister(3U, 0xfffff00dU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001bU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(cpu.readStatus(), 0x0009U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000eU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000010U);
		assertEqual(cpu.readStatus(), 0x0002U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testCMPI()
	{
		writeAddress(0x000000U, uint16_t{0x0c80U});
		writeAddress(0x000002U, uint32_t{0x0badf00dU}); // cmpi.l #$0badf00d, d0
		writeAddress(0x000006U, uint16_t{0x0c00U});
		writeAddress(0x000008U, uint16_t{0x007fU}); // cmpi.b #$7f, d0
		writeAddress(0x00000aU, uint16_t{0x0c40U});
		writeAddress(0x00000cU, uint16_t{0xaca7U}); // cmpi.w #$aca7, d0
		writeAddress(0x00000eU, uint16_t{0x0c02U});
		writeAddress(0x000010U, uint16_t{0x0001U}); // cmpi.b #1, d2
		writeAddress(0x000012U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0 and d2 to sensible values
		cpu.writeDataRegister(0U, 0x0badf00dU);
		cpu.writeDataRegister(2U, 0x00000080U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001bU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(cpu.readStatus(), 0x0009U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000eU);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000012U);
		assertEqual(cpu.readStatus(), 0x0002U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testDIVS()
	{
		writeAddress(0x000000U, uint16_t{0x81fcU});
		writeAddress(0x000002U, uint16_t{0x0004U}); // divs.w #4, d0
		writeAddress(0x000004U, uint16_t{0x83fcU});
		writeAddress(0x000006U, uint16_t{0x0001U}); // divs.w #1, d1
		writeAddress(0x000008U, uint16_t{0x83fcU});
		writeAddress(0x00000aU, uint16_t{0x0000U}); // divs.w #0, d1
		writeAddress(0x00000cU, uint16_t{0x85fcU});
		writeAddress(0x00000eU, uint16_t{0x8000U}); // divs.w #$8000, d2
		writeAddress(0x000010U, uint16_t{0x85fcU});
		writeAddress(0x000012U, uint16_t{0xffffU}); // divs.w #-1, d2
		writeAddress(0x000014U, uint16_t{0x87fcU});
		writeAddress(0x000016U, uint16_t{0x0002U}); // divs.w #2, d3
		writeAddress(0x000018U, uint16_t{0x89fcU});
		writeAddress(0x00001aU, uint16_t{0x0002U}); // divs.w #2, d4
		writeAddress(0x00001cU, uint16_t{0x85fcU});
		writeAddress(0x00001eU, uint16_t{0x0001U}); // divs.w #1, d2
		writeAddress(0x000020U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0, d1, d2, d3 and d4 to sensible values
		cpu.writeDataRegister(0U, 0x00010041U);
		cpu.writeDataRegister(1U, 0x00068000U);
		cpu.writeDataRegister(2U, 0x80000000U);
		cpu.writeDataRegister(3U, 0x00010041U);
		cpu.writeDataRegister(4U, 0xfffff00dU);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0x00014010U);
		assertEqual(cpu.readStatus(), 0x0010U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(1U), 0x00068000U);
		assertEqual(cpu.readStatus(), 0x000aU);
		// Step the third instruction and validate
		runStep(true); // Expect this one to trap because of DBZ
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readDataRegister(1U), 0x00068000U);
		assertEqual(cpu.readStatus(), 0x000aU);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000010U);
		assertEqual(cpu.readDataRegister(2U), 0x80000000U);
		assertEqual(cpu.readStatus(), 0x0006U);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000014U);
		assertEqual(cpu.readDataRegister(2U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the sixth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000018U);
		assertEqual(cpu.readDataRegister(3U), 0x00010041U);
		assertEqual(cpu.readStatus(), 0x000aU);
		// Step the seventh instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000001cU);
		assertEqual(cpu.readDataRegister(4U), 0xfffff807U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the eighth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000020U);
		assertEqual(cpu.readDataRegister(2U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testDIVU()
	{
		writeAddress(0x000000U, uint16_t{0x80fcU});
		writeAddress(0x000002U, uint16_t{0x0004U}); // divu.w #4, d0
		writeAddress(0x000004U, uint16_t{0x82fcU});
		writeAddress(0x000006U, uint16_t{0x0001U}); // divu.w #1, d1
		writeAddress(0x000008U, uint16_t{0x82fcU});
		writeAddress(0x00000aU, uint16_t{0x0000U}); // divu.w #0, d1
		writeAddress(0x00000cU, uint16_t{0x84fcU});
		writeAddress(0x00000eU, uint16_t{0x8000U}); // divu.w #$8000, d2
		writeAddress(0x000010U, uint16_t{0x84fcU});
		writeAddress(0x000012U, uint16_t{0xffffU}); // divu.w #$ffff, d2
		writeAddress(0x000014U, uint16_t{0x86fcU});
		writeAddress(0x000016U, uint16_t{0x0002U}); // divu.w #2, d3
		writeAddress(0x000018U, uint16_t{0x88fcU});
		writeAddress(0x00001aU, uint16_t{0x0002U}); // divu.w #2, d4
		writeAddress(0x00001cU, uint16_t{0x8afcU});
		writeAddress(0x00001eU, uint16_t{0x0001U}); // divu.w #1, d5
		writeAddress(0x000020U, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0, d1, d2 and d3 to sensible values
		cpu.writeDataRegister(0U, 0x00010041U);
		cpu.writeDataRegister(1U, 0x00068000U);
		cpu.writeDataRegister(2U, 0x80000000U);
		cpu.writeDataRegister(3U, 0x00010041U);
		cpu.writeDataRegister(4U, 0x0000f00dU);
		cpu.writeDataRegister(5U, 0x00000000U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001fU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(0U), 0x00014010U);
		assertEqual(cpu.readStatus(), 0x0010U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x000fU);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(1U), 0x00068000U);
		assertEqual(cpu.readStatus(), 0x000aU);
		// Step the third instruction and validate
		runStep(true); // Expect this one to trap because of DBZ
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readDataRegister(1U), 0x00068000U);
		assertEqual(cpu.readStatus(), 0x000aU);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000010U);
		assertEqual(cpu.readDataRegister(2U), 0x80000000U);
		assertEqual(cpu.readStatus(), 0x0002U);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000014U);
		assertEqual(cpu.readDataRegister(2U), 0x80008000U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the sixth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000018U);
		assertEqual(cpu.readDataRegister(3U), 0x00018020U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the seventh instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000001cU);
		assertEqual(cpu.readDataRegister(4U), 0x00017806U);
		assertEqual(cpu.readStatus(), 0x0000U);
		// Step the eighth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000020U);
		assertEqual(cpu.readDataRegister(5U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the final instruction to complete the test
		runStep();
		assertEqual(cpu.readProgramCounter(), 0xffffffffU);
		assertEqual(cpu.readAddrRegister(7U), 0x00800000U);
	}

	void testEXT()
	{
		writeAddress(0x000000U, uint16_t{0x4880U}); // ext.w d0
		writeAddress(0x000002U, uint16_t{0x4881U}); // ext.w d1
		writeAddress(0x000004U, uint16_t{0x48c0U}); // ext.l d0
		writeAddress(0x000006U, uint16_t{0x48c1U}); // ext.l d1
		writeAddress(0x000008U, uint16_t{0x49c0U}); // extb.l d0
		writeAddress(0x00000aU, uint16_t{0x49c2U}); // extb.l d2
		writeAddress(0x00000cU, uint16_t{0x4e75U}); // rts to end the test
		// Set the CPU to execute this sequence
		cpu.executeFrom(0x00000000U, 0x00800000U);
		// Set up d0, d1 and d2 to sensible values
		cpu.writeDataRegister(0U, 0x00000000U);
		cpu.writeDataRegister(1U, 0x000000c3U);
		cpu.writeDataRegister(2U, 0x000000f0U);
		// Set the status register to some improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x001bU);
		// Validate starting conditions
		assertEqual(cpu.readProgramCounter(), 0x00000000U);
		assertEqual(cpu.readAddrRegister(7U), 0x007ffffcU);
		// Step the first instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000002U);
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0014U);
		// Set the status register to some other improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x0007U);
		// Step the second instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000004U);
		assertEqual(cpu.readDataRegister(1U), 0x0000ffc3U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the third instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000006U);
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Step the fourth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x00000008U);
		assertEqual(cpu.readDataRegister(1U), 0xffffffc3U);
		assertEqual(cpu.readStatus(), 0x0008U);
		// Step the fifth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000aU);
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);
		assertEqual(cpu.readStatus(), 0x0004U);
		// Set the status register to a third improbable value that makes it easy to see if it changes
		cpu.writeStatus(0x0017U);
		// Step the sixth instruction and validate
		runStep();
		assertEqual(cpu.readProgramCounter(), 0x0000000cU);
		assertEqual(cpu.readDataRegister(2U), 0xfffffff0U);
		assertEqual(cpu.readStatus(), 0x0018U);
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
	CRUNCH_VIS testM68k() noexcept : testsuite{}, memoryMap_t<uint32_t, 0x00ffffffU>{}
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
		CXX_TEST(testBRA)
		CXX_TEST(testBSR)
		CXX_TEST(testJMP)
		CXX_TEST(testRTS)
		CXX_TEST(testRTE)
		CXX_TEST(testADD)
		CXX_TEST(testADDA)
		CXX_TEST(testADDI)
		CXX_TEST(testADDQ)
		CXX_TEST(testANDI)
		CXX_TEST(testANDISpecial)
		CXX_TEST(testASL)
		CXX_TEST(testASR)
		CXX_TEST(testBCLR)
		CXX_TEST(testBSET)
		CXX_TEST(testBTST)
		CXX_TEST(testCLR)
		CXX_TEST(testCMP)
		CXX_TEST(testCMPA)
		CXX_TEST(testCMPI)
		CXX_TEST(testDIVS)
		CXX_TEST(testDIVU)
		CXX_TEST(testEXT)
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
		CXX_TEST(testOR)
		CXX_TEST(testORI)
		CXX_TEST(testScc)
		CXX_TEST(testSUB)
		CXX_TEST(testSUBQ)
		CXX_TEST(testSWAP)
		CXX_TEST(testTRAP)
		CXX_TEST(testTST)
		CXX_TEST(testDisplayRegs)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testM68k>();
}
