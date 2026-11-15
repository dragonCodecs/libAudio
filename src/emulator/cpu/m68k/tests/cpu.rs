// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::cell::RefCell;
use std::rc::Rc;

use bitmask_enum::bitmask;

use crate::emulator::MemoryMap;

#[allow(unused)]
pub struct Motorola68k<'a>
{
	peripherals: Rc<RefCell<MemoryMap<'a, u32>>>,
	clock_frequency: u32,

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
	pub fn new(peripherals: Rc<RefCell<MemoryMap<'a, u32>>>, clock_frequency: u32) -> Self
	{
		Self {
			peripherals,
			clock_frequency,

			// By default the system starts up in supervisor (system) mode
			status: M68kStatusBits::Supervisor,
			pending_irqs: 0,
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
}
