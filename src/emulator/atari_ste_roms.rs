// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::cell::RefCell;
use std::ffi::c_void;

use crate::emulator::gemdos_alloc::GemdosAllocator;

#[allow(unused)]
pub struct AtariSTeROMs
{
	cpu: *mut c_void,
	peripherals: *mut c_void,
	allocator: RefCell<GemdosAllocator>,
}

impl AtariSTeROMs
{
	pub fn new(cpu: *mut c_void, peripherals: *mut c_void, heap_base: u32, heap_size: u32) -> Self
	{
		Self {
			cpu,
			peripherals,
			allocator: RefCell::new(GemdosAllocator::new(heap_base, heap_size)),
		}
	}
}
