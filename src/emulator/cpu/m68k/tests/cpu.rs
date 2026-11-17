// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ffi::c_void;
use std::ptr::NonNull;

use super::Motorola68k;

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
