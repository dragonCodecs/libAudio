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

	fn handle_gemdos_access(&self)
	{
		// Extract the current stack pointer
		let stack_pointer = unsafe { cpu_read_addr_register(self.cpu, 7) };
		// Figure out where on the stack the opcode value is and extract it
		let opcode = unsafe { peripherals_read_address_u16(self.peripherals, stack_pointer + 8) };
		// Dispatch the opcode requested
		match opcode {
			// GEMDOS version number, respond with being version 0.30 via D0
			0x30 => unsafe { cpu_write_data_register(self.cpu, 0, 0x3000) },
			// mxalloc (we ignore the mode value.. sp + 14U)
			0x44 |
			// malloc
			0x48 => {
				// Set D0 in case of failure to a NULL pointer for the platform
				unsafe { cpu_write_data_register(self.cpu, 0, 0) };
				// Extract how large and allocation is being requested
				let amount = unsafe { peripherals_read_address_i32(self.peripherals, stack_pointer + 10) };
				// If the amount requested is the special value -1, return the largest free block
				if amount == -1 {
					unsafe { cpu_write_data_register(self.cpu, 0, self.allocator.borrow().largest_free_block()) };
				}
				// Otherwise if it's a non-zero positive amount to allocate, actually try to
				else if amount > 0 {
					let result = self.allocator.borrow_mut().alloc(amount as usize);
					// Check if it succeeded, and if so then write the new pointer into D0
					if let Some(ptr) = result {
						unsafe { cpu_write_data_register(self.cpu, 0, ptr) };
					}
				}
			},
			// free
			0x49 => {
				// Extract the pointer for the allocation being released
				let ptr = unsafe { peripherals_read_address_u32(self.peripherals, stack_pointer + 10) };
				// Execute on that with our actual allocator
				if self.allocator.borrow_mut().free(ptr) {
					// Deallocation succeeded, so return 0 (E_OK)
					unsafe { cpu_write_data_register(self.cpu, 0, 0); };
				} else {
					// Deallocation failed, signal this with a failure code
					unsafe { cpu_write_data_register(self.cpu, 0, u16::MAX as u32); };
				}
			},
			_ => panic!("Unimplemented GEMDOS operation"),
		}
	}
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

unsafe extern "C" {
	fn cpu_read_addr_register(cpu: *mut c_void, reg: usize) -> u32;
	fn cpu_write_data_register(cpu: *mut c_void, reg: usize, value: u32);

	fn peripherals_read_address_u16(peripherals: *mut c_void, address: u32) -> u16;
	fn peripherals_read_address_i32(peripherals: *mut c_void, address: u32) -> i32;
	fn peripherals_read_address_u32(peripherals: *mut c_void, address: u32) -> u32;
}
