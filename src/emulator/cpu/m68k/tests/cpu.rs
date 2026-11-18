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
