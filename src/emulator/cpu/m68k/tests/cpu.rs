// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ffi::c_void;
use std::ptr::NonNull;

use super::Motorola68k;
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

#[test]
fn rts()
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

#[unsafe(no_mangle)]
pub fn cpu_request_interrupt(cpu: *mut c_void, level: u8)
{
	let mut cpu = NonNull::new(cpu as *mut Motorola68k).expect("CPU is a valid instance");
	unsafe { cpu.as_mut() }.request_interrupt(level);
}

#[unsafe(no_mangle)]
pub fn cpu_read_addr_register(cpu: *mut c_void, reg: usize) -> u32
{
	let cpu = NonNull::new(cpu as *mut Motorola68k).expect("CPU is a valid instance");
	unsafe { cpu.as_ref() }.read_addr_register(reg)
}

#[unsafe(no_mangle)]
pub fn cpu_write_addr_register(cpu: *mut c_void, reg: usize, value: u32)
{
	let mut cpu = NonNull::new(cpu as *mut Motorola68k).expect("CPU is a valid instance");
	unsafe { cpu.as_mut() }.write_addr_register(reg, value);
}

#[unsafe(no_mangle)]
pub fn cpu_read_data_register(cpu: *mut c_void, reg: usize) -> u32
{
	let cpu = NonNull::new(cpu as *mut Motorola68k).expect("CPU is a valid instance");
	unsafe { cpu.as_ref() }.read_data_register(reg)
}

#[unsafe(no_mangle)]
pub fn cpu_write_data_register(cpu: *mut c_void, reg: usize, value: u32)
{
	let mut cpu = NonNull::new(cpu as *mut Motorola68k).expect("CPU is a valid instance");
	unsafe { cpu.as_mut() }.write_data_register(reg, value);
}
