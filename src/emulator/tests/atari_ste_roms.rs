// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use super::AtariSTeROMs;
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
	memory_map.write_address(0x000106, 0x5c8fu16); // addq.l #6, sp
	memory_map.write_address(0x000106, 0x4e75u16); // rts
	// Set the CPU to execute this sequence and run it
	// assert!(cpu.execute_to_return(0x00000100, 0x00080000, false));
	// Extract the allocation address returned and check it's correct
	// assert_eq!(cpu.read_data_register(0), 0x00080000);
}
