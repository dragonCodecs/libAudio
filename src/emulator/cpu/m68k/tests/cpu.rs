// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ffi::c_void;
use std::ptr::NonNull;

use bitmask_enum::bitmask;

use crate::emulator::MemoryMap;

#[allow(unused)]
pub struct Motorola68k<'a>
{
	peripherals: MemoryMap<'a, u32>,
	clock_frequency: u32,

	// There are 8 data registers, 7 address registers, 3 stack pointers,
	// a program counter and a status register in a m68k
	d: [u32; 8],
	a: [u32; 7],
	system_stack_pointer: u32,
	interrupt_stack_pointer: u32,
	user_stack_pointer: u32,
	program_counter: u32,

	status: M68kStatusBits,
	// What IRQs are pending execution (7 levels, masked by the status register IRQ bits)
	pending_irqs: u8,
}

#[bitmask(u16)]
enum M68kStatusBits
{
	// User byte bit meanings
	Carry = 0,
	Overflow = 1,
	Zero = 2,
	Negative = 3,
	Extend = 4,

	// System byte bit meanings
	// 8-10 are interrupt bits
	Supervisor = 13,
	Trace = 15,
}

impl<'a> Motorola68k<'a>
{
	pub fn new(peripherals: MemoryMap<'a, u32>, clock_frequency: u32) -> Self
	{
		Self {
			peripherals,
			clock_frequency,

			d: [0; 8],
			a: [0; 7],
			system_stack_pointer: 0,
			interrupt_stack_pointer: 0,
			user_stack_pointer: 0,
			program_counter: u32::MAX,

			// By default the system starts up in supervisor (system) mode
			status: M68kStatusBits::Supervisor,
			pending_irqs: 0,
		}
	}

	pub(self) fn request_interrupt(&mut self, level: u8)
	{
		// Make sure the caller isn't asking for an insane interrupt level, then mark it pending
		if level > 0 && level <= 7 {
			self.pending_irqs |= 1 << level;
		}
	}

	pub fn has_pending_interrupts(&self) -> bool
	{
		// Extract the current exception mask from the status register
		let min_irq_priority = ((self.status.bits() & 0x0700) >> 8) as u8;
		// Calculate a mask from this number
		let irq_mask = !((1 << (min_irq_priority + 1)) - 1) as u8;
		// Now see which (if any) pending IRQ bits are still set (level 7 is always allowed, it is NMI)
		let unmasked_irqs = self.pending_irqs & (irq_mask | 0x80);
		unmasked_irqs != 0
	}

	fn data_register(&self, reg: usize) -> &u32
	{
		&self.d[reg]
	}

	fn data_register_mut(&mut self, reg: usize) -> &mut u32
	{
		&mut self.d[reg]
	}

	fn addr_register(&self, reg: usize) -> &u32
	{
		if reg == 7 {
			self.active_stack_pointer()
		} else {
			&self.a[reg]
		}
	}

	fn addr_register_mut(&mut self, reg: usize) -> &mut u32
	{
		if reg == 7 {
			self.active_stack_pointer_mut()
		} else {
			&mut self.a[reg]
		}
	}

	pub fn read_data_register(&self, reg: usize) -> u32
	{
		*self.data_register(reg)
	}

	pub fn write_data_register(&mut self, reg: usize, value: u32)
	{
		*self.data_register_mut(reg) = value;
	}

	pub fn read_addr_register(&self, reg: usize) -> u32
	{
		*self.addr_register(reg)
	}

	pub fn write_addr_register(&mut self, reg: usize, value: u32)
	{
		*self.addr_register_mut(reg) = value;
	}

	fn active_stack_pointer(&self) -> &u32
	{
		if self.status.contains(M68kStatusBits::Supervisor) {
			&self.system_stack_pointer
		} else {
			&self.user_stack_pointer
		}
	}

	fn active_stack_pointer_mut(&mut self) -> &mut u32
	{
		if self.status.contains(M68kStatusBits::Supervisor) {
			&mut self.system_stack_pointer
		} else {
			&mut self.user_stack_pointer
		}
	}
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
