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
