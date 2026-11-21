// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use super::{Motorola68k, StepResult};
use crate::emulator::MemoryMap;
use crate::emulator::ram::Ram;

const STE_RAM_SIZE: usize = 8 * 1024 * 1024;

fn new_m68k<'a>() -> (Motorola68k<'a>, MemoryMap<'a, u32>)
{
	let mut memory_map = MemoryMap::<u32>::new(0x00ffffff);
	let cpu = Motorola68k::new(memory_map.clone(), 8_000_000);

	// Check all the CPU preconditions before any execution begins
	for reg in 0..8 {
		assert_eq!(cpu.read_data_register(reg), 0);
	}
	for reg in 0..8 {
		assert_eq!(cpu.read_addr_register(reg), 0);
	}
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_status(), 0x2000);

	// Register some memory for the tests to use
	memory_map.add_peripheral_at(0x000000..0x800000, Ram::<STE_RAM_SIZE>::new().into());

	(cpu, memory_map)
}

fn run_step(cpu: &mut Motorola68k<'_>, expecting_trap: bool)
{
	let result = cpu.step();
	if expecting_trap {
		assert_eq!(result, StepResult::Trap);
	} else {
		assert_eq!(result, StepResult::Valid);
	}
}

#[test]
fn run_bra()
{
	let (mut cpu, mut memory_map) = new_m68k();
	// Set up 3 branches in each of the 3 instruction encoding forms, checking they jump properly to each other
	// Start with the 8-bit immediate form, then use the 16-bit immediate form, and finally the 32-bit immediate
	// form (6 bytes to encode)
	memory_map.write_address(0x000100, 0x6050u16); // bra $50.b ; Jump to +0x50 from end of instruction
	memory_map.write_address(0x000152, 0x6000u16);
	memory_map.write_address(0x000154, 0xfefcu16); // bra -$0104.w ; Jump to -0x104 from end of instruction
	memory_map.write_address(0x000050, 0x60ffu16);
	memory_map.write_address(0x000052, 0x000100aeu32); // bra $0100ae.l ; Jump to +0x100ae from end of instruction
	memory_map.write_address(0x010100, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000100, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000100);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000152);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000050);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00010100);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the fourth and final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
	assert_eq!(cpu.read_status(), 0x0000);
}

#[test]
fn run_bsr()
{
	let (mut cpu, mut memory_map) = new_m68k();
	// Set up 3 branches in each of the 3 instruction encoding forms, checking they jump properly and then return
	// Start with the 8-bit immediate form, then use the 16-bit immediate form, and finally the 32-bit immediate
	// form (6 bytes to encode)
	memory_map.write_address(0x000200, 0x6150u16); // bsr $50.b ; Jump to +0x50 from end of instruction
	memory_map.write_address(0x000202, 0x6100u16);
	memory_map.write_address(0x000204, 0xfefcu16); // bsr -$104.w ; Jump to -0x104 from end of instruction u16
	memory_map.write_address(0x000206, 0x61ffu16);
	memory_map.write_address(0x000208, 0x000100aeu32); // bsr $100ae.l ; Jump to +0x100ae from end of instruction u16
	memory_map.write_address(0x00020c, 0x4e75u16); // rts to end the test
	// RTS's for each of the BSRs to hit
	memory_map.write_address(0x000252, 0x4e75u16);
	memory_map.write_address(0x000100, 0x4e75u16);
	memory_map.write_address(0x0102b6, 0x4e75u16);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000200, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000200);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000252);
	assert_eq!(cpu.read_addr_register(7), 0x007ffff8);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the RTS for that and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000202);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000100);
	assert_eq!(cpu.read_addr_register(7), 0x007ffff8);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the RTS for that and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000206);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x000102b6);
	assert_eq!(cpu.read_addr_register(7), 0x007ffff8);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the RTS for that and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000020c);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the fourth and final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
	assert_eq!(cpu.read_status(), 0x0000);
}

#[test]
fn run_jmp()
{
	let (mut cpu, mut memory_map) = new_m68k();
	// NB: We don't test all EA's here as we get through them in other tests.
	memory_map.write_address(0x000000, 0x4ed0u16); // jmp (a0)
	memory_map.write_address(0x000004, 0x4ee8u16);
	memory_map.write_address(0x000006, 0x0100u16); // jmp $0100(a0)
	memory_map.write_address(0x000104, 0x4ef8u16);
	memory_map.write_address(0x000106, 0x0110u16); // jmp $0110.w
	memory_map.write_address(0x000110, 0x4ef9u16);
	memory_map.write_address(0x000112, 0x0001u16);
	memory_map.write_address(0x000114, 0x0000u16); // jmp $00010000.l
	memory_map.write_address(0x010000, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	cpu.write_addr_register(0, 0x00000004);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(0), 0x00000004);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000104);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000110);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00010000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the fifth and final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
	assert_eq!(cpu.read_status(), 0x0000);
}

#[test]
fn run_rts()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x4e75u16); // rts
	// Run the solitary rts
	assert!(cpu.execute_to_return(0x00000000, 0x00800000, false));
	// Then make sure the CPU state matches up
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
	assert_eq!(cpu.read_status(), 0x2000);
}

#[test]
fn run_rte()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000100, 0x4e73u16); // rte
	// Set up to execute an exception vector
	cpu.execute_from_exception(0x00000100, 0x00800000, 1);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000100);
	assert_eq!(cpu.read_addr_register(7), 0x007ffff8);
	assert_eq!(cpu.read_status(), 0x2000);
	// Check that the stacked exception frame is okay
	assert_eq!(memory_map.read_address::<u16>(0x007ffffe), 0x1004); // Frame type and vector info
	assert_eq!(memory_map.read_address::<u32>(0x007ffffa), u32::MAX); // Return program counter
	assert_eq!(memory_map.read_address::<u16>(0x007ffff8), 0x0000); // Return status register
	// Step the CPU and validate the results line up
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
	assert_eq!(cpu.read_status(), 0x0000);
}

#[test]
fn run_add()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xd070u16);
	memory_map.write_address(0x000002, 0x1204u16); // add.w 4(a0, d1.w*2), d0
	memory_map.write_address(0x000004, 0xd030u16);
	memory_map.write_address(0x000006, 0x8800u16); // add.b (a0, a0.l), d0
	memory_map.write_address(0x000008, 0xd088u16); // add.l a0, d0
	memory_map.write_address(0x00000a, 0xd590u16); // add.l d2, (a0)
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0u32);
	memory_map.write_address(0x000108, 0x8080u16);
	memory_map.write_address(0x000200, 0xc1u8);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, d0, d1 and d2 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0x40000004);
	cpu.write_data_register(1, 0x00000002);
	cpu.write_data_register(2, 0x00000000);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x40008084);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(0), 0x40008045);
	assert_eq!(cpu.read_status(), 0x0013);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_data_register(0), 0x40008145);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(memory_map.read_address::<u32>(0x00000100), 0);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_adda()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xd0e1u16); // adda.w -(a1), a0
	memory_map.write_address(0x000002, 0xd3c0u16); // adda.l d0, a1
	memory_map.write_address(0x000004, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0xc100u16);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, a1 and d0 to sensible values
	cpu.write_addr_register(0, 0x00000010);
	cpu.write_addr_register(1, 0x00000102);
	cpu.write_data_register(0, 0x40000004);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_addr_register(0), 0xffffc110);
	assert_eq!(cpu.read_addr_register(1), 0x00000100);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_addr_register(1), 0x40000104);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_addi()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x0618u16);
	memory_map.write_address(0x000002, 0x0004u16); // addi.b #4, (a0)+
	memory_map.write_address(0x000004, 0x0658u16);
	memory_map.write_address(0x000006, 0x8001u16); // addi.w #$8001, (a0)+
	memory_map.write_address(0x000008, 0x0698u16);
	memory_map.write_address(0x00000a, 0xfeed4ca7u32); // addi.l #$feed4ca7, (a0)+
	memory_map.write_address(0x00000e, 0x0628u16);
	memory_map.write_address(0x000010, 0x00bcu16);
	memory_map.write_address(0x000012, 0xfff9u16); // addi.b #$bc, -7(a0)
	memory_map.write_address(0x000014, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x40u8);
	memory_map.write_address(0x000101, 0x7ffeu16);
	memory_map.write_address(0x000103, 0x80000001u32);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0 to point to the data pool at +0x0100U
	cpu.write_addr_register(0, 0x00000100);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_addr_register(0), 0x00000101);
	assert_eq!(memory_map.read_address::<u8>(0x00000100), 0x44);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_addr_register(0), 0x00000103);
	assert_eq!(memory_map.read_address::<u16>(0x00000101), 0xffff);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000e);
	assert_eq!(cpu.read_addr_register(0), 0x00000107);
	assert_eq!(memory_map.read_address::<u32>(0x00000103), 0x7eed4ca8);
	assert_eq!(cpu.read_status(), 0x0013);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000014);
	assert_eq!(memory_map.read_address::<u8>(0x00000100), 0x00);
	assert_eq!(cpu.read_status(), 0x0015);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_addq()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x5a04u16); // addq.b #5, d4
	memory_map.write_address(0x000002, 0x5848u16); // addq.w #4, a0
	memory_map.write_address(0x000004, 0x5083u16); // addq.l #8, d3
	memory_map.write_address(0x000006, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, d3 and d4 to sensible values
	cpu.write_addr_register(0, 0x000000fc);
	cpu.write_data_register(3, 0x7ffffff8);
	cpu.write_data_register(4, 0x000000ff);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(4), 0x00000004);
	assert_eq!(cpu.read_status(), 0x0011);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_addr_register(0), 0x00000100);
	// As this is done to an address register, the status flags should not change
	assert_eq!(cpu.read_status(), 0x0011);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(3), 0x80000000);
	assert_eq!(cpu.read_status(), 0x000a);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_and()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xc080u16); // and.l d0, d0
	memory_map.write_address(0x000002, 0xc050u16); // and.w (a0), d0
	memory_map.write_address(0x000004, 0xc03cu16);
	memory_map.write_address(0x000006, 0x0060u16); // and.b #$60, d0
	memory_map.write_address(0x000008, 0xc199u16); // and.l d0, (a1)+
	memory_map.write_address(0x00000a, 0xc07cu16);
	memory_map.write_address(0x00000c, 0x0000u16); // and.w #$00, d0
	memory_map.write_address(0x00000e, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x4400ffffu32);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Re-use the 3rd instruction as the data for the 2nd, and set a reasonable destination for the 4th
	cpu.write_addr_register(0, 0x00000004);
	cpu.write_addr_register(1, 0x00000100);
	// Make sure d0 starts out with a sensible value for this sequence
	cpu.write_data_register(0, 0x7f00ffff);
	// Set the carry and overflow bits in the status register so we can observe them being cleared
	// And the extend bit to make sure that's left alone
	cpu.write_status(0x0013);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0013);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(0), 0x7f00ffff);
	assert_eq!(cpu.read_status(), 0x0010);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x7f00c03c);
	assert_eq!(cpu.read_status(), 0x0018);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(0), 0x7f00c020);
	assert_eq!(cpu.read_status(), 0x0010);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_data_register(0), 0x7f00c020);
	assert_eq!(cpu.read_addr_register(1), 0x00000104);
	assert_eq!(memory_map.read_address::<u32>(0x000100), 0x4400c020);
	assert_eq!(cpu.read_status(), 0x0010);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000e);
	assert_eq!(cpu.read_status(), 0x0014);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_andi()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x0238u16);
	memory_map.write_address(0x000002, 0x0024u16);
	memory_map.write_address(0x000004, 0x0100u16); // andi.b #$24, $0100.w
	memory_map.write_address(0x000006, 0x0258u16);
	memory_map.write_address(0x000008, 0x0000u16); // andi.w #0, (a0)+
	memory_map.write_address(0x00000a, 0x0281u16);
	memory_map.write_address(0x00000c, 0x8421u16);
	memory_map.write_address(0x00000e, 0x1248u16); // andi.l #$84211248, d1
	memory_map.write_address(0x000010, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x29u8);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0 and d1 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(1, 0xfeedaca7);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0x20);
	assert_eq!(cpu.read_status(), 0x0010);
	// Reset the status register to a different improbable value that makes it easy to see if it changes
	cpu.write_status(0x000f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_addr_register(0), 0x00000102);
	assert_eq!(memory_map.read_address::<u16>(0x000100), 0);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000010);
	assert_eq!(cpu.read_data_register(1), 0x84210000);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_andi_special()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x023cu16);
	memory_map.write_address(0x000002, 0x00ffu16); // andi #$ff, ccr
	memory_map.write_address(0x000004, 0x023cu16);
	memory_map.write_address(0x000006, 0x0005u16); // andi #$05, ccr
	memory_map.write_address(0x000008, 0x027cu16);
	memory_map.write_address(0x00000a, 0xf00fu16); // andi #$f00f, sr
	memory_map.write_address(0x00000c, 0x027cu16);
	memory_map.write_address(0x00000e, 0x070au16); // andi #$070a, sr
	memory_map.write_address(0x000010, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set the status register to some improbable value that makes it easy to how it changes
	cpu.write_status(0x271f);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_status(), 0x271f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_status(), 0x2705);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_status(), 0x2005);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000010);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);

	// This sequence should trap
	memory_map.write_address(0x000000, 0x027cu16);
	memory_map.write_address(0x000002, 0xffffu16); // andi #$ffff, sr
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, true);
}

#[test]
fn run_asl()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xe501u16); // asl.b #2, d1
	memory_map.write_address(0x000002, 0xe161u16); // asl.w d0, d1
	memory_map.write_address(0x000004, 0xe521u16); // asl.b d2, d1
	memory_map.write_address(0x000006, 0xe181u16); // asl.l #8, d1
	memory_map.write_address(0x000008, 0xe1d0u16); // asl (a0)
	memory_map.write_address(0x00000a, 0xe903u16); // asl.b #4, d3
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x55aau16);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, d0, d1, d2 and d3 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0x00000004);
	cpu.write_data_register(1, 0x00000060);
	cpu.write_data_register(2, 0x00000000);
	cpu.write_data_register(3, 0x00000045);
	// Set the status register to some improbable value that makes it easy to how it changes
	cpu.write_status(0x0006);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(1), 0x00000080);
	assert_eq!(cpu.read_status(), 0x001b);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(1), 0x00000800);
	assert_eq!(cpu.read_status(), 0x0000);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001b);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(1), 0x00000800);
	assert_eq!(cpu.read_status(), 0x0004); // XXX: X bit should actually be unaffacted by this.
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(1), 0x00080000);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(memory_map.read_address::<u16>(0x000100), 0xab54);
	assert_eq!(cpu.read_status(), 0x000a);
	// Step the sixth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_data_register(3), 0x00000050);
	assert_eq!(cpu.read_status(), 0x0002);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_asr()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xe401u16); // asr.b #2, d1
	memory_map.write_address(0x000002, 0xe061u16); // asr.w d0, d1
	memory_map.write_address(0x000004, 0xe421u16); // asr.b d2, d1
	memory_map.write_address(0x000006, 0xe081u16); // asr.l #8, d1
	memory_map.write_address(0x000008, 0xe0d0u16); // asr (a0)
	memory_map.write_address(0x00000a, 0xe021u16); // asr.b d0, d1
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x55aau16);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, d0, d1 and d2 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0x00000004);
	cpu.write_data_register(1, 0x80004806);
	cpu.write_data_register(2, 0x00000000);
	// Set the status register to some improbable value that makes it easy to how it changes
	cpu.write_status(0x000e);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(1), 0x80004801);
	assert_eq!(cpu.read_status(), 0x0011);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(1), 0x80000480);
	assert_eq!(cpu.read_status(), 0x0000);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x0017);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(1), 0x80000480);
	assert_eq!(cpu.read_status(), 0x0018);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(1), 0xff800004);
	assert_eq!(cpu.read_status(), 0x0019);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(memory_map.read_address::<u16>(0x000100), 0x2ad5);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the sixth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_data_register(1), 0xff800000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_bcc()
{
	let (mut cpu, mut memory_map) = new_m68k();
	// This jump sled requires the status register be changed in each step to a suitable value
	// to variously trigger and not trigger the different jumps.
	memory_map.write_address(0x000000, 0x6240u16); // bhi +$40 (0x000042)
	memory_map.write_address(0x000002, 0x6202u16); // bhi +$02 (0x000006)
	memory_map.write_address(0x000006, 0x6340u16); // bls +$40 (0x000048)
	memory_map.write_address(0x000008, 0x6302u16); // bls +$02 (0x00000c)
	memory_map.write_address(0x00000c, 0x6440u16); // bcc +$40 (0x00004e)
	memory_map.write_address(0x00000e, 0x6402u16); // bcc +$02 (0x000012)
	memory_map.write_address(0x000012, 0x6540u16); // bcs +$40 (0x000054)
	memory_map.write_address(0x000014, 0x6502u16); // bcs +$02 (0x000018)
	memory_map.write_address(0x000018, 0x6640u16); // bne +$40 (0x00005a)
	memory_map.write_address(0x00001a, 0x6602u16); // bne +$02 (0x00001e)
	memory_map.write_address(0x00001e, 0x6740u16); // beq +$40 (0x000060)
	memory_map.write_address(0x000020, 0x6702u16); // beq +$02 (0x000024)
	memory_map.write_address(0x000024, 0x6840u16); // bvc +$40 (0x000066)
	memory_map.write_address(0x000026, 0x6802u16); // bvc +$02 (0x00002a)
	memory_map.write_address(0x00002a, 0x6940u16); // bvs +$40 (0x00006c)
	memory_map.write_address(0x00002c, 0x6902u16); // bvs +$02 (0x000030)
	memory_map.write_address(0x000030, 0x6a40u16); // bpl +$40 (0x000072)
	memory_map.write_address(0x000032, 0x6a02u16); // bpl +$02 (0x000036)
	memory_map.write_address(0x000036, 0x6b40u16); // bmi +$40 (0x000078)
	memory_map.write_address(0x000038, 0x6b02u16); // bmi +$02 (0x00003c)
	memory_map.write_address(0x00003c, 0x6c40u16); // bge +$40 (0x00007e)
	memory_map.write_address(0x00003e, 0x6c02u16); // bge +$02 (0x000042)
	memory_map.write_address(0x000042, 0x6d40u16); // blt +$40 (0x000084)
	memory_map.write_address(0x000044, 0x6d02u16); // blt +$02 (0x000048)
	memory_map.write_address(0x000048, 0x6e40u16); // bgt +$40 (0x00008a)
	memory_map.write_address(0x00004a, 0x6e02u16); // bgt +$02 (0x00004e)
	memory_map.write_address(0x00004e, 0x6f00u16);
	memory_map.write_address(0x000050, 0x0100u16); // ble +$0100 (0x000152)
	memory_map.write_address(0x000052, 0x6f00u16);
	memory_map.write_address(0x000054, 0x00acu16); // ble +$00aa (0x000100)
	memory_map.write_address(0x000100, 0x4e75u16); // rts to end the test

	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set up the status register to skip the first jump
	cpu.write_status(0x001f);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	// Set up the status register to not skip the second jump
	cpu.write_status(0x001a);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001f);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000e);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001e);
	// Step the sixth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000012);
	// Step the seventh instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000014);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001f);
	// Step the eighth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000018);
	// Step the nineth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000001a);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001b);
	// Step the tenth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000001e);
	// Step the eleventh instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000020);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001f);
	// Step the twelveth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000024);
	// Step the thirteenth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000026);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001d);
	// Step the fourteenth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000002a);
	// Step the fiftheenth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000002c);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001f);
	// Step the sixtheenth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000030);
	// Step the seventheenth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000032);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x0017);
	// Step the eigtheenth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000036);
	// Step the ninteenth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000038);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001f);
	// Step the twenthieth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000003c);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x0017);
	// Step the twentyfirst instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000003e);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001f);
	// Step the twentysecond instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000042);
	// Step the twentythird instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000044);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x0017);
	// Step the twentyfourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000048);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001f);
	// Step the twentyfifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000004a);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x001b);
	// Step the twentysixth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000004e);
	// Step the twentyseventh instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000052);
	// Set up the status register to not skip the next jump
	cpu.write_status(0x0013);
	// Step the twentyeigth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000100);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_bclr()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x0880u16);
	memory_map.write_address(0x000002, 0x0002u16); // bclr #2, d0
	memory_map.write_address(0x000004, 0x0380u16); // bclr d1, d0
	memory_map.write_address(0x000006, 0x0390u16); // bclr d1, (a0)
	memory_map.write_address(0x000008, 0x0890u16);
	memory_map.write_address(0x00000a, 0x0007u16); // bclr #7, (a0)
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0xa5u8);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, d0 and d1 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0xcafef00f);
	cpu.write_data_register(1, 0x00000004);
	// Set the status register to some improbable value that makes it easy to how it changes
	cpu.write_status(0x001f);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0xcafef00b);
	assert_eq!(cpu.read_status(), 0x001b);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(0), 0xcafef00b);
	assert_eq!(cpu.read_status(), 0x001f);
	// Reset the status register to a different value that makes it easy to see if it changes
	cpu.write_status(0x0000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0xa5);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0x25);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_bset()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x08c0u16);
	memory_map.write_address(0x000002, 0x0002u16); // bset #2, d0
	memory_map.write_address(0x000004, 0x03c0u16); // bset d1, d0
	memory_map.write_address(0x000006, 0x03d0u16); // bset d1, (a0)
	memory_map.write_address(0x000008, 0x08d0u16);
	memory_map.write_address(0x00000a, 0x0007u16); // bset #7, (a0)
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0xa5u8);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, d0 and d1 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0xcafef00d);
	cpu.write_data_register(1, 0x00000004);
	// Set the status register to some improbable value that makes it easy to how it changes
	cpu.write_status(0x001f);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0xcafef00d);
	assert_eq!(cpu.read_status(), 0x001b);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(0), 0xcafef01d);
	assert_eq!(cpu.read_status(), 0x001f);
	// Reset the status register to a different value that makes it easy to see if it changes
	cpu.write_status(0x0000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0xb5);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0xb5);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_btst()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x0800u16);
	memory_map.write_address(0x000002, 0x0002u16); // btst #2, d0
	memory_map.write_address(0x000004, 0x0300u16); // btst d1, d0
	memory_map.write_address(0x000006, 0x0310u16); // btst d1, (a0)
	memory_map.write_address(0x000008, 0x0810u16);
	memory_map.write_address(0x00000a, 0x0007u16); // btst #7, (a0)
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0xa5u8);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, d0 and d1 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0xcafef00d);
	cpu.write_data_register(1, 0x00000004);
	// Set the status register to some improbable value that makes it easy to how it changes
	cpu.write_status(0x001f);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0xcafef00d);
	assert_eq!(cpu.read_status(), 0x001b);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(0), 0xcafef00d);
	assert_eq!(cpu.read_status(), 0x001f);
	// Reset the status register to a different value that makes it easy to see if it changes
	cpu.write_status(0x0000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0xa5);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0xa5);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_clr()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x4200u16); // clr.b d0
	memory_map.write_address(0x000002, 0x4240u16); // clr.w d0
	memory_map.write_address(0x000004, 0x4280u16); // clr.l d0
	memory_map.write_address(0x000006, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0 to a sensible value
	cpu.write_data_register(0, 0xfeedaca7);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001b);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(0), 0xfeedac00);
	assert_eq!(cpu.read_status(), 0x0014);
	// Reset the status register to a different improbable value that makes it easy to see if it changes
	cpu.write_status(0x000f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0xfeed0000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(0), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_cmp()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xb0bcu16);
	memory_map.write_address(0x000002, 0x0badf00du32); // cmp.l #$0badf00d, d0
	memory_map.write_address(0x000006, 0xb03cu16);
	memory_map.write_address(0x000008, 0x007fu16); // cmp.b #$7f, d0
	memory_map.write_address(0x00000a, 0xb07cu16);
	memory_map.write_address(0x00000c, 0xaca7u16); // cmp.w #$aca7, d0
	memory_map.write_address(0x00000e, 0xb401u16); // cmp.b d1, d2
	memory_map.write_address(0x000010, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0, d1 and d2 to sensible values
	cpu.write_data_register(0, 0x0badf00d);
	cpu.write_data_register(1, 0x00000001);
	cpu.write_data_register(2, 0x00000080);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001b);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_status(), 0x0014);
	// Set the status register to some other improbable value that makes it easy to see if it changes
	cpu.write_status(0x000f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_status(), 0x0009);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000e);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000010);
	assert_eq!(cpu.read_status(), 0x0002);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_cmpa()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xb1fcu16);
	memory_map.write_address(0x000002, 0x0badf00du32); // cmpa.l #$0badf00d, a0
	memory_map.write_address(0x000006, 0xb2fcu16);
	memory_map.write_address(0x000008, 0x007fu16); // cmpa.w #$7f, a1
	memory_map.write_address(0x00000a, 0xb6fcu16);
	memory_map.write_address(0x00000c, 0xaca7u16); // cmpa.w #$aca7, a3
	memory_map.write_address(0x00000e, 0xb4c1u16); // cmpa.w d1, a2
	memory_map.write_address(0x000010, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d1, a0, a1, a2 and a3 to sensible values
	cpu.write_data_register(1, 0x00000001);
	cpu.write_addr_register(0, 0x0badf00d);
	cpu.write_addr_register(1, 0x0000000d);
	cpu.write_addr_register(2, 0x00008000);
	cpu.write_addr_register(3, 0xfffff00d);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001b);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_status(), 0x0014);
	// Set the status register to some other improbable value that makes it easy to see if it changes
	cpu.write_status(0x000f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_status(), 0x0009);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000e);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000010);
	assert_eq!(cpu.read_status(), 0x0002);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_cmpi()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x0c80u16);
	memory_map.write_address(0x000002, 0x0badf00du32); // cmpi.l #$0badf00d, d0
	memory_map.write_address(0x000006, 0x0c00u16);
	memory_map.write_address(0x000008, 0x007fu16); // cmpi.b #$7f, d0
	memory_map.write_address(0x00000a, 0x0c40u16);
	memory_map.write_address(0x00000c, 0xaca7u16); // cmpi.w #$aca7, d0
	memory_map.write_address(0x00000e, 0x0c02u16);
	memory_map.write_address(0x000010, 0x0001u16); // cmpi.b #1, d2
	memory_map.write_address(0x000012, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0 and d2 to sensible values
	cpu.write_data_register(0, 0x0badf00d);
	cpu.write_data_register(2, 0x00000080);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001b);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_status(), 0x0014);
	// Set the status register to some other improbable value that makes it easy to see if it changes
	cpu.write_status(0x000f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_status(), 0x0009);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000e);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000012);
	assert_eq!(cpu.read_status(), 0x0002);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_dbcc()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x51c8u16);
	memory_map.write_address(0x000002, 0xfffeu16); // dbf d0, #-2
	memory_map.write_address(0x000004, 0x5dc9u16);
	memory_map.write_address(0x000006, 0xfffcu16); // dblt d1, #-4
	memory_map.write_address(0x000008, 0x50c9u16);
	memory_map.write_address(0x00000a, 0xfff8u16); // dbt d1, #-8
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0, and d1 to sensible values
	cpu.write_data_register(0, 0x08080001);
	cpu.write_data_register(1, 0x00068000);
	// Set the status register to the a valid less-than condition configuration
	cpu.write_status(0x0008);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_data_register(0), 0x08080000);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x0808ffff);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(1), 0x00068000);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_data_register(1), 0x00068000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_divs()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x81fcu16);
	memory_map.write_address(0x000002, 0x0004u16); // divs.w #4, d0
	memory_map.write_address(0x000004, 0x83fcu16);
	memory_map.write_address(0x000006, 0x0001u16); // divs.w #1, d1
	memory_map.write_address(0x000008, 0x83fcu16);
	memory_map.write_address(0x00000a, 0x0000u16); // divs.w #0, d1
	memory_map.write_address(0x00000c, 0x85fcu16);
	memory_map.write_address(0x00000e, 0x8000u16); // divs.w #$8000, d2
	memory_map.write_address(0x000010, 0x85fcu16);
	memory_map.write_address(0x000012, 0xffffu16); // divs.w #-1, d2
	memory_map.write_address(0x000014, 0x87fcu16);
	memory_map.write_address(0x000016, 0x0002u16); // divs.w #2, d3
	memory_map.write_address(0x000018, 0x89fcu16);
	memory_map.write_address(0x00001a, 0x0002u16); // divs.w #2, d4
	memory_map.write_address(0x00001c, 0x85fcu16);
	memory_map.write_address(0x00001e, 0x0001u16); // divs.w #1, d2
	memory_map.write_address(0x000020, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0, d1, d2, d3 and d4 to sensible values
	cpu.write_data_register(0, 0x00010041);
	cpu.write_data_register(1, 0x00068000);
	cpu.write_data_register(2, 0x80000000);
	cpu.write_data_register(3, 0x00010041);
	cpu.write_data_register(4, 0xfffff00d);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x00014010);
	assert_eq!(cpu.read_status(), 0x0010);
	// Set the status register to some other improbable value that makes it easy to see if it changes
	cpu.write_status(0x000f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(1), 0x00068000);
	assert_eq!(cpu.read_status(), 0x000a);
	// Step the third instruction and validate
	run_step(&mut cpu, true); // Expect this one to trap because of DBZ
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_data_register(1), 0x00068000);
	assert_eq!(cpu.read_status(), 0x000a);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000010);
	assert_eq!(cpu.read_data_register(2), 0x80000000);
	assert_eq!(cpu.read_status(), 0x0006);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000014);
	assert_eq!(cpu.read_data_register(2), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the sixth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000018);
	assert_eq!(cpu.read_data_register(3), 0x00010041);
	assert_eq!(cpu.read_status(), 0x000a);
	// Step the seventh instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000001c);
	assert_eq!(cpu.read_data_register(4), 0xfffff807);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the eighth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000020);
	assert_eq!(cpu.read_data_register(2), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_divu()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x80fcu16);
	memory_map.write_address(0x000002, 0x0004u16); // divu.w #4, d0
	memory_map.write_address(0x000004, 0x82fcu16);
	memory_map.write_address(0x000006, 0x0001u16); // divu.w #1, d1
	memory_map.write_address(0x000008, 0x82fcu16);
	memory_map.write_address(0x00000a, 0x0000u16); // divu.w #0, d1
	memory_map.write_address(0x00000c, 0x84fcu16);
	memory_map.write_address(0x00000e, 0x8000u16); // divu.w #$8000, d2
	memory_map.write_address(0x000010, 0x84fcu16);
	memory_map.write_address(0x000012, 0xffffu16); // divu.w #$ffff, d2
	memory_map.write_address(0x000014, 0x86fcu16);
	memory_map.write_address(0x000016, 0x0002u16); // divu.w #2, d3
	memory_map.write_address(0x000018, 0x88fcu16);
	memory_map.write_address(0x00001a, 0x0002u16); // divu.w #2, d4
	memory_map.write_address(0x00001c, 0x8afcu16);
	memory_map.write_address(0x00001e, 0x0001u16); // divu.w #1, d5
	memory_map.write_address(0x000020, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0, d1, d2, d3, d4 and d5 to sensible values
	cpu.write_data_register(0, 0x00010041);
	cpu.write_data_register(1, 0x00068000);
	cpu.write_data_register(2, 0x80000000);
	cpu.write_data_register(3, 0x00010041);
	cpu.write_data_register(4, 0x0000f00d);
	cpu.write_data_register(5, 0x00000000);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x00014010);
	assert_eq!(cpu.read_status(), 0x0010);
	// Set the status register to some other improbable value that makes it easy to see if it changes
	cpu.write_status(0x000f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(1), 0x00068000);
	assert_eq!(cpu.read_status(), 0x000a);
	// Step the third instruction and validate
	run_step(&mut cpu, true); // Expect this one to trap because of DBZ
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_data_register(1), 0x00068000);
	assert_eq!(cpu.read_status(), 0x000a);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000010);
	assert_eq!(cpu.read_data_register(2), 0x80000000);
	assert_eq!(cpu.read_status(), 0x0002);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000014);
	assert_eq!(cpu.read_data_register(2), 0x80008000);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the sixth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000018);
	assert_eq!(cpu.read_data_register(3), 0x00018020);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the seventh instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000001c);
	assert_eq!(cpu.read_data_register(4), 0x00017806);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the eighth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000020);
	assert_eq!(cpu.read_data_register(5), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

// XXX: Need to implement a test for EOR execution here (normal and CCR/SR forms)

#[test]
fn run_eori()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x0a38u16);
	memory_map.write_address(0x000002, 0x0024u16);
	memory_map.write_address(0x000004, 0x0100u16); // eori.b #$24, $0100.w
	memory_map.write_address(0x000006, 0x0a40u16);
	memory_map.write_address(0x000008, 0x0000u16); // eori.w #0, d0
	memory_map.write_address(0x00000a, 0x0a80u16);
	memory_map.write_address(0x00000c, 0xc0000001u32); // eori.l #$c0000001, d0
	memory_map.write_address(0x000010, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x21u8);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0 so we get zero = true from the second instruction, but get something interesting for the 3rd
	cpu.write_data_register(0, 0x45a00000);
	// Set the carry and overflow bits in the status register so we can observe them being cleared
	// And the extend bit to make sure that's left alone
	cpu.write_status(0x0013);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0013);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0x05);
	assert_eq!(cpu.read_status(), 0x0010);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_data_register(0), 0x45a00000);
	assert_eq!(cpu.read_status(), 0x0014);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000010);
	assert_eq!(cpu.read_data_register(0), 0x85a00001);
	assert_eq!(cpu.read_status(), 0x0018);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_eori_special()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x0a3cu16);
	memory_map.write_address(0x000002, 0x000au16); // eori #$0a, ccr
	memory_map.write_address(0x000004, 0x0a3cu16);
	memory_map.write_address(0x000006, 0xff15u16); // eori #$15, ccr w/ intentional value misencode
	memory_map.write_address(0x000008, 0x0a7cu16);
	memory_map.write_address(0x00000a, 0x070eu16); // eori #$070e, sr
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set the status register to some improbable value that makes it easy to how it changes
	cpu.write_status(0x000c);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_status(), 0x0006);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_status(), 0x0013);
	// Switch the processor into supervisor mode
	cpu.write_status(0x2700);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_status(), 0x200e);
	// Switch the processor back out into user mode
	cpu.write_status(0x0000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);

	// This sequence should trap
	memory_map.write_address(0x000000, 0x0a7cu16);
	memory_map.write_address(0x000002, 0x2705u16); // eori #$2705, sr
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	run_step(&mut cpu, true);
}

#[test]
fn run_ext()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x4880u16); // ext.w d0
	memory_map.write_address(0x000002, 0x4881u16); // ext.w d1
	memory_map.write_address(0x000004, 0x48c0u16); // ext.l d0
	memory_map.write_address(0x000006, 0x48c1u16); // ext.l d1
	memory_map.write_address(0x000008, 0x49c0u16); // extb.l d0
	memory_map.write_address(0x00000a, 0x49c2u16); // extb.l d2
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0, d1 and d2 to sensible values
	cpu.write_data_register(0, 0x00000000);
	cpu.write_data_register(1, 0x000000c3);
	cpu.write_data_register(2, 0x000000f0);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001b);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(0), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0014);
	// Set the status register to some other improbable value that makes it easy to see if it changes
	cpu.write_status(0x0007);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(1), 0x0000ffc3);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(0), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(1), 0xffffffc3);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_data_register(0), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Set the status register to a third improbable value that makes it easy to see if it changes
	cpu.write_status(0x0017);
	// Step the sixth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_data_register(2), 0xfffffff0);
	assert_eq!(cpu.read_status(), 0x0018);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_jsr()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x4e90u16); // jsr (a0)
	memory_map.write_address(0x000002, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x4e75u16); // rts to return from JSR
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0 to point to the "subroutine"
	cpu.write_addr_register(0, 0x00000100);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000100);
	assert_eq!(cpu.read_addr_register(7), 0x007ffff8);
	assert_eq!(cpu.read_status(), 0x001f);
	assert_eq!(memory_map.read_address::<u32>(0x007ffff8), 0x00000002);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_lea()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x43d0u16); // lea (a0), a1
	memory_map.write_address(0x000002, 0x43e8u16);
	memory_map.write_address(0x000004, 0xfeedu16); // lea $feed(a0), a1
	memory_map.write_address(0x000006, 0x43f0u16);
	memory_map.write_address(0x000008, 0x0c10u16); // lea $10(a0, d0.l*4), a1
	memory_map.write_address(0x00000a, 0x43f8u16);
	memory_map.write_address(0x00000c, 0xca15u16); // lea ($ca15).w, a1
	memory_map.write_address(0x00000e, 0x43f9u16);
	memory_map.write_address(0x000010, 0x0badf00du32); // lea ($0badf00d).l, a1
	memory_map.write_address(0x000014, 0x43fau16);
	memory_map.write_address(0x000016, 0xfffau16); // lea -6(pc), a1
	memory_map.write_address(0x000018, 0x43fbu16);
	memory_map.write_address(0x00001a, 0x0727u16);
	memory_map.write_address(0x00001c, 0x0026u16);
	memory_map.write_address(0x00001e, 0xf00daca1u32); // lea $f00daca1($26[pc], d0.w*8), a1
	memory_map.write_address(0x000022, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000040, 0xdeadbeefu32);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0 and d0 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0x00000200);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_addr_register(1), 0x00000100);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_addr_register(1), 0xffffffed);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_addr_register(1), 0x00000910);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000e);
	assert_eq!(cpu.read_addr_register(1), 0xffffca15);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000014);
	assert_eq!(cpu.read_addr_register(1), 0x0badf00d);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the sixth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000018);
	assert_eq!(cpu.read_addr_register(1), 0x00000010);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the seventh instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000022);
	// 0xdeadbeef + 0x00000200 * 8 + 0xf00daca1
	assert_eq!(cpu.read_addr_register(1), 0xcebb7b90);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_lsl()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xe509u16); // lsl.b #2, d1
	memory_map.write_address(0x000002, 0xe169u16); // lsl.w d0, d1
	memory_map.write_address(0x000004, 0xe529u16); // lsl.b d2, d1
	memory_map.write_address(0x000006, 0xe189u16); // lsl.l #8, d1
	memory_map.write_address(0x000008, 0xe3d0u16); // lsl (a0)
	memory_map.write_address(0x00000a, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x55aau16);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, d0, d1 and d2 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0x00000004);
	cpu.write_data_register(1, 0x00000060);
	cpu.write_data_register(2, 0x00000000);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x0006);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(1), 0x00000080);
	assert_eq!(cpu.read_status(), 0x0019);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(1), 0x00000800);
	assert_eq!(cpu.read_status(), 0x0000);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001b);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(1), 0x00000800);
	assert_eq!(cpu.read_status(), 0x0014);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(1), 0x00080000);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(memory_map.read_address::<u16>(0x000100), 0xab54);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_lsr()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xe409u16); // lsr.b #2, d1
	memory_map.write_address(0x000002, 0xe069u16); // lsr.w d0, d1
	memory_map.write_address(0x000004, 0xe429u16); // lsr.b d2, d1
	memory_map.write_address(0x000006, 0xe089u16); // lsr.l #8, d1
	memory_map.write_address(0x000008, 0xe2d0u16); // lsr (a0)
	memory_map.write_address(0x00000a, 0xe029u16); // lsr.b d0, d1
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x55aau16);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, d0, d1 and d2 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0x00000004);
	cpu.write_data_register(1, 0x01008806);
	cpu.write_data_register(2, 0x00000000);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x000e);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(1), 0x01008801);
	assert_eq!(cpu.read_status(), 0x0011);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(1), 0x01000880);
	assert_eq!(cpu.read_status(), 0x0000);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x0017);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(1), 0x01000880);
	assert_eq!(cpu.read_status(), 0x0018);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(1), 0x00010008);
	assert_eq!(cpu.read_status(), 0x0011);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(memory_map.read_address::<u16>(0x000100), 0x2ad5);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the sixth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_data_register(1), 0x00010000);
	assert_eq!(cpu.read_status(), 0x0015);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_move()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x103cu16);
	memory_map.write_address(0x000002, 0x0048u16); // move.b #$48, d0
	memory_map.write_address(0x000004, 0x303cu16);
	memory_map.write_address(0x000006, 0x0000u16); // move.w #0, d0
	memory_map.write_address(0x000008, 0x203cu16);
	memory_map.write_address(0x00000a, 0xff000000u32); // move.w #$ff000000, d0
	memory_map.write_address(0x00000e, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x00000048);
	assert_eq!(cpu.read_status(), 0x0010);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(0), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0014);
	// Set the status register to some other value that makes it easy to see if it changes
	cpu.write_status(0x0000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000e);
	assert_eq!(cpu.read_data_register(0), 0xff000000);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_move_special_ccr()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x44c0u16); // move d0, ccr
	memory_map.write_address(0x000002, 0x42c2u16); // move ccr, d2
	memory_map.write_address(0x000004, 0x44c1u16); // move d1, ccr
	memory_map.write_address(0x000006, 0x42c2u16); // move ccr, d2
	memory_map.write_address(0x000008, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence in user mode
	cpu.write_status(0x0000);
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Set up d0 and d1 to sensible values
	cpu.write_data_register(0, 0x0000270f);
	cpu.write_data_register(1, 0x00000015);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_status(), 0x000f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_status(), 0x000f);
	assert_eq!(cpu.read_data_register(2), 0x0000000f);
	// Switch into supervisor mode
	cpu.write_status(0x2000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_status(), 0x2015);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_status(), 0x2015);
	assert_eq!(cpu.read_data_register(2), 0x00000015);
	// Switch back into user mode
	cpu.write_status(0x0000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_move_special_sr()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x46c0u16); // move d0, sr
	memory_map.write_address(0x000002, 0x40c2u16); // move sr, d2
	memory_map.write_address(0x000004, 0x46c1u16); // move d1, sr
	memory_map.write_address(0x000006, 0x40c2u16); // move sr, d2
	memory_map.write_address(0x000008, 0x46c0u16); // move d0, sr
	// Set the CPU to execute this sequence in supervisor mode
	cpu.write_status(0x0000);
	cpu.execute_from(0x00000000, 0x00800000, false);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x2000);
	// Set up d0 and d1 to sensible values
	cpu.write_data_register(0, 0x0000270f);
	cpu.write_data_register(1, 0x00000015);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_status(), 0x270f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_status(), 0x270f);
	assert_eq!(cpu.read_data_register(2), 0x0000270f);
	// Step the third instruction and validate
	// this transitions us out of supervisor mode
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_status(), 0x0015);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_status(), 0x0015);
	assert_eq!(cpu.read_data_register(2), 0x00000015);
	// Step the fifth instruction and validate (this should trap)
	run_step(&mut cpu, true);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_status(), 0x0015);
}

#[test]
fn run_move_special_usp()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x4e60u16); // move a0, usp
	memory_map.write_address(0x000002, 0x4e69u16); // move usp, a1
	memory_map.write_address(0x000004, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence in supervisor mode
	cpu.write_status(0x0000);
	assert_eq!(cpu.read_addr_register(7), 0x00000000);
	cpu.execute_from(0x00000000, 0x00800000, false);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x2000);
	// Set up a0 to a sensible value
	cpu.write_addr_register(0, 0x1234abcd);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	// Switch into user mode
	cpu.write_status(0x0000);
	assert_eq!(cpu.read_addr_register(7), 0x1234abcd);
	// Update the USP with a new value then switch back into supervisor mode
	cpu.write_addr_register(7, 0xdeadbeef);
	cpu.write_status(0x2000);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_addr_register(1), 0xdeadbeef);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);

	// Set the CPU back up to execute the same sequence in user mode
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the first instruction and validate that it traps
	run_step(&mut cpu, true);
}

#[test]
fn run_movea()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x3048u16); // movea.w a0, a0
	memory_map.write_address(0x000002, 0x2040u16); // movea.l d0, a0
	memory_map.write_address(0x000004, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0 and d0 to sensible values
	cpu.write_addr_register(0, 0xfeedaca7);
	cpu.write_data_register(0, 0x01248421);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_addr_register(0), 0xffffaca7);
	assert_eq!(cpu.read_status(), 0x001f);
	// Set the status register to some other value that makes it easy to see if it changes
	cpu.write_status(0x0000);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_addr_register(0), 0x01248421);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_movem()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x48f8u16);
	memory_map.write_address(0x000002, 0x00ffu16);
	memory_map.write_address(0x000004, 0x0100u16); // movem.l d0-d7, ($0100).w
	memory_map.write_address(0x000006, 0x48f8u16);
	memory_map.write_address(0x000008, 0xff00u16);
	memory_map.write_address(0x00000a, 0x0100u16); // movem.l a0-a7, ($0100).w
	memory_map.write_address(0x00000c, 0x48a7u16);
	memory_map.write_address(0x00000e, 0xc0c0u16); // movem.w d0-d1/a0-a1, -(sp)
	memory_map.write_address(0x000010, 0x4c9fu16);
	memory_map.write_address(0x000012, 0x001eu16); // movem.w +(sp), d1-d4
	memory_map.write_address(0x000014, 0x3043u16); // movea.w d3, a0
	memory_map.write_address(0x000016, 0x4cd8u16);
	memory_map.write_address(0x000018, 0x2418u16); // movem.l +(a0), d3-d4/a2,a5
	memory_map.write_address(0x00001a, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up all the data registers so it's easy to tell which one got put where
	cpu.write_data_register(0, 0x00010001);
	cpu.write_data_register(1, 0x00020002);
	cpu.write_data_register(2, 0x00030003);
	cpu.write_data_register(3, 0x00040004);
	cpu.write_data_register(4, 0x00050005);
	cpu.write_data_register(5, 0x00060006);
	cpu.write_data_register(6, 0x00070007);
	cpu.write_data_register(7, 0x00080008);
	// Set up all the address registers so it's easy to tell which one got put where
	cpu.write_addr_register(0, 0x01000100);
	cpu.write_addr_register(1, 0x02000200);
	cpu.write_addr_register(2, 0x03000300);
	cpu.write_addr_register(3, 0x04000400);
	cpu.write_addr_register(4, 0x05000500);
	cpu.write_addr_register(5, 0x06000600);
	cpu.write_addr_register(6, 0x07000700);
	// NB, we skip a7 as that's the stack pointer and we know what value that has already.
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(memory_map.read_address::<u32>(0x000100), 0x00010001);
	assert_eq!(memory_map.read_address::<u32>(0x000104), 0x00020002);
	assert_eq!(memory_map.read_address::<u32>(0x000108), 0x00030003);
	assert_eq!(memory_map.read_address::<u32>(0x00010c), 0x00040004);
	assert_eq!(memory_map.read_address::<u32>(0x000110), 0x00050005);
	assert_eq!(memory_map.read_address::<u32>(0x000114), 0x00060006);
	assert_eq!(memory_map.read_address::<u32>(0x000118), 0x00070007);
	assert_eq!(memory_map.read_address::<u32>(0x00011c), 0x00080008);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(memory_map.read_address::<u32>(0x000100), 0x01000100);
	assert_eq!(memory_map.read_address::<u32>(0x000104), 0x02000200);
	assert_eq!(memory_map.read_address::<u32>(0x000108), 0x03000300);
	assert_eq!(memory_map.read_address::<u32>(0x00010c), 0x04000400);
	assert_eq!(memory_map.read_address::<u32>(0x000110), 0x05000500);
	assert_eq!(memory_map.read_address::<u32>(0x000114), 0x06000600);
	assert_eq!(memory_map.read_address::<u32>(0x000118), 0x07000700);
	assert_eq!(memory_map.read_address::<u32>(0x00011c), 0x007ffffc);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000010);
	assert_eq!(cpu.read_addr_register(7), 0x007ffff4);
	assert_eq!(memory_map.read_address::<u16>(0x7ffffa), 0x0200);
	assert_eq!(memory_map.read_address::<u16>(0x7ffff8), 0x0100);
	assert_eq!(memory_map.read_address::<u16>(0x7ffff6), 0x0002);
	assert_eq!(memory_map.read_address::<u16>(0x7ffff4), 0x0001);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000014);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_data_register(1), 0x00000001);
	assert_eq!(cpu.read_data_register(2), 0x00000002);
	assert_eq!(cpu.read_data_register(3), 0x00000100);
	assert_eq!(cpu.read_data_register(4), 0x00000200);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000016);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_addr_register(0), 0x00000100);
	// Step the sixth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000001a);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_addr_register(0), 0x00000110);
	assert_eq!(cpu.read_data_register(3), 0x01000100);
	assert_eq!(cpu.read_data_register(4), 0x02000200);
	assert_eq!(cpu.read_addr_register(2), 0x03000300);
	assert_eq!(cpu.read_addr_register(5), 0x04000400);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_movep()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x0188u16);
	memory_map.write_address(0x000002, 0x0000u16); // movep.w d0, $0(a0)
	memory_map.write_address(0x000004, 0x01c8u16);
	memory_map.write_address(0x000006, 0x0011u16); // movep.l d0, $11(a0)
	memory_map.write_address(0x000008, 0x0308u16);
	memory_map.write_address(0x00000a, 0x0000u16); // movep.w $0(a0), d1
	memory_map.write_address(0x00000c, 0x0548u16);
	memory_map.write_address(0x00000e, 0x0010u16); // movep.l $10(a0), d2
	memory_map.write_address(0x000010, 0x0548u16);
	memory_map.write_address(0x000012, 0x0011u16); // movep.l $11(a0), d2
	memory_map.write_address(0x000014, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0 and d0 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0xfeedaca7);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(memory_map.read_address::<u16>(0x000100), 0xac00);
	assert_eq!(memory_map.read_address::<u16>(0x000102), 0xa700);
	assert_eq!(cpu.read_status(), 0x001f);
	// Set the status register to some other value that makes it easy to see if it changes
	cpu.write_status(0x0000);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(memory_map.read_address::<u16>(0x000110), 0x00fe);
	assert_eq!(memory_map.read_address::<u16>(0x000112), 0x00ed);
	assert_eq!(memory_map.read_address::<u16>(0x000114), 0x00ac);
	assert_eq!(memory_map.read_address::<u16>(0x000116), 0x00a7);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_data_register(1), 0x0000aca7);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000010);
	assert_eq!(cpu.read_data_register(2), 0x00000000);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000014);
	assert_eq!(cpu.read_data_register(2), 0xfeedaca7);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_moveq()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x7080u16); // moveq #$80, d0
	memory_map.write_address(0x000002, 0x7000u16); // moveq #$00, d0
	memory_map.write_address(0x000004, 0x705au16); // moveq #$5a, d0
	memory_map.write_address(0x000006, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Zero d0 so it's easy to see if the first move works properly
	cpu.write_data_register(0, 0x00000000);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x0017);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(0), 0xffffff80);
	assert_eq!(cpu.read_status(), 0x0018);
	// Set the status register to some other improbable value that makes it easy to see if it changes
	cpu.write_status(0x000b);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(0), 0x0000005a);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_muls()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xc1fcu16);
	memory_map.write_address(0x000002, 0x0003u16); // muls.w #3, d0
	memory_map.write_address(0x000004, 0xc3c0u16); // muls.w d0, d1
	memory_map.write_address(0x000006, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0 and d1 to sensible values
	cpu.write_data_register(0, 0x0ca78132);
	cpu.write_data_register(1, 0x00010000);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0xfffe8396);
	assert_eq!(cpu.read_status(), 0x0018);
	// Set the status register to some other improbable value that makes it easy to see if it changes
	cpu.write_status(0x0001);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(1), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_mulu()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0xc0fcu16);
	memory_map.write_address(0x000002, 0x0003u16); // mulu.w #3, d0
	memory_map.write_address(0x000004, 0xc2c0u16); // mulu.w d0, d1
	memory_map.write_address(0x000006, 0xc4c2u16); // mulu.w d2, d2
	memory_map.write_address(0x000008, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0 and d1 to sensible values
	cpu.write_data_register(0, 0x0ca78132);
	cpu.write_data_register(1, 0x00010000);
	cpu.write_data_register(2, 0x0000ffff);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x00018396);
	assert_eq!(cpu.read_status(), 0x0010);
	// Set the status register to some other improbable value that makes it easy to see if it changes
	cpu.write_status(0x0001);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(1), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(2), 0xfffe0001);
	assert_eq!(cpu.read_status(), 0x0008);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_neg()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x4400u16); // neg.b d0
	memory_map.write_address(0x000002, 0x4440u16); // neg.w d0
	memory_map.write_address(0x000004, 0x4480u16); // neg.l d0
	memory_map.write_address(0x000006, 0x4458u16); // neg.w (a0)+
	memory_map.write_address(0x000008, 0x4450u16); // neg.w (a0)
	memory_map.write_address(0x00000a, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x0000u16);
	memory_map.write_address(0x000102, 0x8000u16);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0 and d0 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0xffff00ff);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x000e);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(0), 0xffff0001);
	assert_eq!(cpu.read_status(), 0x0011);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0xffffffff);
	assert_eq!(cpu.read_status(), 0x0019);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(cpu.read_data_register(0), 0x00000001);
	assert_eq!(cpu.read_status(), 0x0011);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_addr_register(0), 0x00000102);
	assert_eq!(memory_map.read_address::<u16>(0x000100), 0x0000);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the fifth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_addr_register(0), 0x00000102);
	assert_eq!(memory_map.read_address::<u16>(0x000102), 0x8000);
	assert_eq!(cpu.read_status(), 0x001b);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_nop()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x4e71u16); // nop
	memory_map.write_address(0x000002, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_or()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x8080u16); // or.l d0, d0
	memory_map.write_address(0x000002, 0x8050u16); // or.w (a0), d0
	memory_map.write_address(0x000004, 0x803cu16);
	memory_map.write_address(0x000006, 0x0040u16); // or.b #$40, d0
	memory_map.write_address(0x000008, 0x8199u16); // or.l d0, (a1)+
	memory_map.write_address(0x00000a, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x84000000u32);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Re-use the 3rd instruction as the data for the 2nd, and set a reasonable destination for the 4th
	cpu.write_addr_register(0, 0x00000004);
	cpu.write_addr_register(1, 0x00000100);
	// Make sure d0 starts out as 0 for this sequence
	cpu.write_data_register(0, 0x00000000);
	// Set the carry and overflow bits in the status register so we can observe them being cleared
	// And the extend bit to make sure that's left alone
	cpu.write_status(0x0013);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_data_register(0), 0x00000000);
	assert_eq!(cpu.read_addr_register(0), 0x00000004);
	assert_eq!(cpu.read_addr_register(1), 0x00000100);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0013);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(0), 0x00000000);
	assert_eq!(cpu.read_status(), 0x0014);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x0000803c);
	assert_eq!(cpu.read_status(), 0x0018);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(0), 0x0000807c);
	assert_eq!(cpu.read_status(), 0x0010);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_data_register(0), 0x0000807c);
	assert_eq!(cpu.read_addr_register(1), 0x00000104);
	assert_eq!(memory_map.read_address::<u32>(0x000100), 0x8400807c);
	assert_eq!(cpu.read_status(), 0x0018);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_ori()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x0038u16);
	memory_map.write_address(0x000002, 0x0024u16);
	memory_map.write_address(0x000004, 0x0100u16); // ori.b #$24, $0100.w
	memory_map.write_address(0x000006, 0x0040u16);
	memory_map.write_address(0x000008, 0x0000u16); // ori.w #0, d0
	memory_map.write_address(0x00000a, 0x0080u16);
	memory_map.write_address(0x00000c, 0x80000001u32); // ori.l #$80000001, d0
	memory_map.write_address(0x000010, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x01u8);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0 so we get zero = true from the second instruction, but get something interesting for the 3rd
	cpu.write_data_register(0, 0x05a00000);
	// Set the carry and overflow bits in the status register so we can observe them being cleared
	// And the extend bit to make sure that's left alone
	cpu.write_status(0x0013);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	assert_eq!(cpu.read_status(), 0x0013);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000006);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0x25);
	assert_eq!(cpu.read_status(), 0x0010);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000a);
	assert_eq!(cpu.read_data_register(0), 0x05a00000);
	assert_eq!(cpu.read_status(), 0x0014);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000010);
	assert_eq!(cpu.read_data_register(0), 0x85a00001);
	assert_eq!(cpu.read_status(), 0x0018);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_ori_special()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x003cu16);
	memory_map.write_address(0x000002, 0x000au16); // ori #$0a, ccr
	memory_map.write_address(0x000004, 0x003cu16);
	memory_map.write_address(0x000006, 0xff15u16); // ori #$15, ccr w/ intentional value misencode
	memory_map.write_address(0x000008, 0x007cu16);
	memory_map.write_address(0x00000a, 0x070eu16); // ori #$070e, sr
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Set the status register to some improbable value that makes it easy to how it changes
	cpu.write_status(0x0000);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_status(), 0x000a);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_status(), 0x001f);
	// Switch the processor into supervisor mode
	cpu.write_status(0x2000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_status(), 0x270e);
	// Switch the processor back out into user mode
	cpu.write_status(0x0000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);

	// This sequence should trap
	memory_map.write_address(0x000000, 0x007cu16);
	memory_map.write_address(0x000002, 0x2705u16); // ori #$2705, sr
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, true);
}

#[test]
fn run_scc()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x50c0u16); // st d0
	memory_map.write_address(0x000002, 0x51c0u16); // sf d0
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up d0 so we can observe which bits change with the condition tests
	cpu.write_data_register(0, 0x00000000);
	// Set up the status bits in a way where it's trivial to observe if they change
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_data_register(0), 0x000000ff);
	assert_eq!(cpu.read_status(), 0x001f);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x00000000);
	assert_eq!(cpu.read_status(), 0x001f);
	// Switch up the status bits in a way where it's trivial to observe if they change
	cpu.write_status(0x0010);
	// XXX: Need to finish writing this test sequence
}

#[test]
fn run_sub()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x9590u16); // sub.l d2, (a0)
	memory_map.write_address(0x000002, 0x9088u16); // sub.l a0, d0
	memory_map.write_address(0x000004, 0x9030u16);
	memory_map.write_address(0x000006, 0x8800u16); // sub.b (a0, a0.l), d0
	memory_map.write_address(0x000008, 0x9070u16);
	memory_map.write_address(0x00000a, 0x1204u16); // sub.w 4(a0, d1.w*2), d0
	memory_map.write_address(0x00000c, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0u32);
	memory_map.write_address(0x000108, 0x8080u16);
	memory_map.write_address(0x000200, 0xc1u8);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, d0, d1 and d2 to sensible values
	cpu.write_addr_register(0, 0x00000100);
	cpu.write_data_register(0, 0x40008145);
	cpu.write_data_register(1, 0x00000002);
	cpu.write_data_register(2, 0x00000000);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(memory_map.read_address::<u32>(0x000100), 0);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_data_register(0), 0x40008045);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_data_register(0), 0x40008084);
	assert_eq!(cpu.read_status(), 0x001b);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000c);
	assert_eq!(cpu.read_data_register(0), 0x40000004);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_suba()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x90e1u16); // suba.w -(a1), a0
	memory_map.write_address(0x000002, 0x93c0u16); // suba.l d0, a1
	memory_map.write_address(0x000004, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0xc100u16);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0, a1 and d0 to sensible values
	cpu.write_addr_register(0, 0x00000010);
	cpu.write_addr_register(1, 0x00000102);
	cpu.write_data_register(0, 0x40000004);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000002);
	assert_eq!(cpu.read_addr_register(0), 0x00003f10);
	assert_eq!(cpu.read_addr_register(1), 0x00000100);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_addr_register(1), 0xc00000fc);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}

#[test]
fn run_subi()
{
	let (mut cpu, mut memory_map) = new_m68k();
	memory_map.write_address(0x000000, 0x0418u16);
	memory_map.write_address(0x000002, 0x0004u16); // subi.b #4, (a0)+
	memory_map.write_address(0x000004, 0x0458u16);
	memory_map.write_address(0x000006, 0x8001u16); // subi.w #$8001, (a0)+
	memory_map.write_address(0x000008, 0x0498u16);
	memory_map.write_address(0x00000a, 0x7fffffffu32); // subi.l #$7fffffff, (a0)+
	memory_map.write_address(0x00000e, 0x0428u16);
	memory_map.write_address(0x000010, 0x003cu16);
	memory_map.write_address(0x000012, 0xfff9u16); // subi.b #$3c, -7(a0)
	memory_map.write_address(0x000014, 0x4e75u16); // rts to end the test
	memory_map.write_address(0x000100, 0x40u8);
	memory_map.write_address(0x000101, 0x7ffeu16);
	memory_map.write_address(0x000103, 0xfeed4ca7u32);
	// Set the CPU to execute this sequence
	cpu.execute_from(0x00000000, 0x00800000, true);
	// Set up a0 to point to the data pool at +0x0100U
	cpu.write_addr_register(0, 0x00000100);
	// Set the status register to some improbable value that makes it easy to see if it changes
	cpu.write_status(0x001f);
	// Validate starting conditions
	assert_eq!(cpu.read_program_counter(), 0x00000000);
	assert_eq!(cpu.read_addr_register(7), 0x007ffffc);
	// Step the first instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000004);
	assert_eq!(cpu.read_addr_register(0), 0x00000101);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0x3c);
	assert_eq!(cpu.read_status(), 0x0000);
	// Step the second instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000008);
	assert_eq!(cpu.read_addr_register(0), 0x00000103);
	assert_eq!(memory_map.read_address::<u16>(0x000101), 0xfffd);
	assert_eq!(cpu.read_status(), 0x001b);
	// Step the third instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x0000000e);
	assert_eq!(cpu.read_addr_register(0), 0x00000107);
	assert_eq!(memory_map.read_address::<u32>(0x000103), 0x7eed4ca8);
	assert_eq!(cpu.read_status(), 0x0002);
	// Step the fourth instruction and validate
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0x00000014);
	assert_eq!(memory_map.read_address::<u8>(0x000100), 0x00);
	assert_eq!(cpu.read_status(), 0x0004);
	// Step the final instruction to complete the test
	run_step(&mut cpu, false);
	assert_eq!(cpu.read_program_counter(), 0xffffffff);
	assert_eq!(cpu.read_addr_register(7), 0x00800000);
}
