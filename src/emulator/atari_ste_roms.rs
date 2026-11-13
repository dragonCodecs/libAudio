// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::cell::RefCell;
use std::ffi::c_void;

use crate::emulator::Peripheral;
use crate::emulator::gemdos_alloc::GemdosAllocator;

pub struct AtariSTeROMs
{
	cpu: *mut c_void,
	peripherals: *mut c_void,
	allocator: RefCell<GemdosAllocator>,
}

pub const HANDLER_ADDRESS_GEMDOS: u32 = 0x000000;

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

	fn handle_gemdos_access(&self) {}
}

impl Peripheral<u32> for AtariSTeROMs
{
	fn read_address(&self, address: u32, data: &mut [u8])
	{
		// Extract how wide an access this is
		let access_width = data.len();
		// We only support u16 access to the "ROM"
		if access_width != 2 {
			return;
		}
		// If this is an access to GEMDOS via a `trap` instruction, dispatch that
		if address == HANDLER_ADDRESS_GEMDOS {
			self.handle_gemdos_access();
			// And then load an RTE as the result of this read
			*data
				.as_mut_array::<2>()
				.expect("Slice of 2 bytes converts to array of 2 bytes") = 0x4e73u16.to_be_bytes();
		}
	}

	fn write_address(&mut self, _address: u32, _data: &[u8])
	{
		// This function is intentionally a no-op as this is a ROM!
	}
}
