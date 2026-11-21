// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use super::{AtariSTeROMs, HANDLER_ADDRESS_GEMDOS};
use crate::emulator::MemoryMap;
use crate::emulator::cpu::m68k::Motorola68k;
use crate::emulator::ram::Ram;

const RAM_SIZE: usize = 1024 * 1024;

const HEAP_BASE: u32 = 0x080000;
const HEAP_SIZE: u32 = 0x080000;

fn setup_memory_map<'a>(cpu: Motorola68k<'a>, memory_map: &mut MemoryMap<'a, u32>)
{
	memory_map.add_peripheral_at(0x000000..0x100000, Ram::<RAM_SIZE>::new().into());
	memory_map.add_peripheral_at(
		0x100000..0x200000,
		AtariSTeROMs::new(cpu, memory_map.clone(), HEAP_BASE, HEAP_SIZE).into(),
	);

	// Set up the GEMDOS TRAP handler so we can use it in the tests
	memory_map.write_address(0x000084, 0x100000 + HANDLER_ADDRESS_GEMDOS);
}

#[test]
fn allocator()
{
	let mut memory_map = MemoryMap::new(0x00ffffff);
	let mut cpu = Motorola68k::new(memory_map.clone(), 8_000_000);
	setup_memory_map(cpu.clone(), &mut memory_map);

	// Set up to allocate 4KiB with Malloc()
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00001000u32); // move.l #4096, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x00080000);

	// Set up to allocate 2KiB with Mxalloc()
	memory_map.write_address(0x000100, 0x4267u16); // clr.w -(sp)
	memory_map.write_address(0x000102, 0x2f3cu16);
	memory_map.write_address(0x000104, 0x00000800u32); // move.l #2048, -(sp)
	memory_map.write_address(0x000108, 0x3f3cu16);
	memory_map.write_address(0x00010a, 0x0044u16); // move.w #$44, -(sp)
	memory_map.write_address(0x00010c, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010e, 0x508fu16); // addq.l #8, sp
	memory_map.write_address(0x000110, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x00081004);

	// Check how much heap memory is free after that
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0xffffffffu32); // move.l #-1, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the largest free block size returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x0007e7f8);

	// Try to allocate 0 bytes and check that fails properly
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00000000u32); // move.l #0, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x00000000);

	// Try to allocate more heap memory than we have and check that fails properly
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00800000u32); // move.l #$00800000, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x00000000);

	// Try to allocate the entire heap in one shot and check that fails properly
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x0007fff8u32); // move.l #$0007fff8, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x00000000);

	// Try to free our first allocation with Mfree()
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00080000u32); // move.l #$00080000, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0049u16); // move.w #$49, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Check that the result was E_OK
	assert_eq!(cpu.read_data_register(0), 0x00000000);

	// Set up to allocate 2KiB with Malloc() to check it uses a block from the free list
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00000800u32); // move.l #2048, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x00080000);

	// Set up to allocate 2KiB with Malloc() to check what happens when there's not enough free in
	// a non-empty free list
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00000800u32); // move.l #2048, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x00081808);

	// Set up to allocate 1KiB with Malloc() to check it still uses a block from the free list
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00000400u32); // move.l #1024, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x00080804);

	// Set up to allocate the remaining 1016 bytes of the current free list block with Malloc()
	// to check it dequeues the block from the free list
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x000003f8u32); // move.l #1016, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x00080c08);

	// Free that allocation
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00080c08u32); // move.l #$00080c08, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0049u16); // move.w #$49, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Check that the result was E_OK
	assert_eq!(cpu.read_data_register(0), 0x00000000);

	// Free our second ever allocation
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00081004u32); // move.l #$00081004, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0049u16); // move.w #$49, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Check that the result was E_OK
	assert_eq!(cpu.read_data_register(0), 0x00000000);

	// Set up to allocate 4KiB with Malloc() to check that the allocator uses fresh heap
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00001000u32); // move.l #4096, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x0008200c);

	// Set up to allocate 2KiB with Malloc() to check that the allocator uses a free block
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00000800u32); // move.l #2048, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x00081004);

	// Try to free a non-existent block
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x00080004u32); // move.l #$00080004, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0049u16); // move.w #$49, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Check that the result was an error
	assert_eq!(cpu.read_data_register(0), 0x0000ffff);

	// Free the last 4KiB allocation
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x0008200cu32); // move.l #$0008200c, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0049u16); // move.w #$49, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Check that the result was E_OK
	assert_eq!(cpu.read_data_register(0), 0x00000000);

	// Set up to allocate the entire rest of the heap with Malloc() to check that the allocator uses
	// that final free block and expands it to meet the request
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x0007dff0u32); // move.l #$0007dff0, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x0008200c);

	// Free that allocation
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x0008200cu32); // move.l #$0008200c, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0049u16); // move.w #$49, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Check that the result was E_OK
	assert_eq!(cpu.read_data_register(0), 0x00000000);

	// Set up to allocate a little more than the entire rest of the heap with Malloc() to check
	// that the allocator errors out with the allocation being too big
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0x0007e000u32); // move.l #$0007e000, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct (an error value)
	assert_eq!(cpu.read_data_register(0), 0x00000000);

	// Check how much heap memory is free after that
	memory_map.write_address(0x000100, 0x2f3cu16);
	memory_map.write_address(0x000102, 0xffffffffu32); // move.l #-1, -(sp)
	memory_map.write_address(0x000106, 0x3f3cu16);
	memory_map.write_address(0x000108, 0x0048u16); // move.w #$48, -(sp)
	memory_map.write_address(0x00010a, 0x4e41u16); // trap #1
	memory_map.write_address(0x00010c, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x00010e, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the largest free block size returned and check it's correct
	assert_eq!(cpu.read_data_register(0), 0x0007dff4);
}
