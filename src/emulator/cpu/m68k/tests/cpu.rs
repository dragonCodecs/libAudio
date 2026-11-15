// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::cell::RefCell;
use std::rc::Rc;

use crate::emulator::MemoryMap;

#[allow(unused)]
pub struct Motorola68k<'a>
{
	peripherals: Rc<RefCell<MemoryMap<'a, u32>>>,
	clock_frequency: u32,
}

impl<'a> Motorola68k<'a>
{
	pub fn new(peripherals: Rc<RefCell<MemoryMap<'a, u32>>>, clock_frequency: u32) -> Self
	{
		Self {
			peripherals,
			clock_frequency,
		}
	}
}
