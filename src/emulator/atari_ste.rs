// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::cell::RefCell;
use std::ffi::c_void;
use std::ops::Range;
use std::rc::Rc;

use crate::emulator::atari_ste_roms::{self, AtariSTeROMs};
use crate::emulator::ram::Ram;
use crate::emulator::sound::ste_dac::STeDac;
use crate::emulator::sound::ym2149::YM2149;
use crate::emulator::timing::mc68901::MC68901;
use crate::emulator::{ClockedPeripheral, MemoryMap};

pub struct AtariSTe<'a>
{
	// m68k has a 24-bit address bus, but we can't directly represent that, so use a 32-bit address value instead.
	memory_map: Box<MemoryMap<'a, u32>>,

	cpu: *mut c_void,
	psg: YM2149,
	dac: STeDac,
	mfp: MC68901,
}

const STE_RAM_SIZE: usize = 8 * 1024 * 1024;

const TIMER_VECTOR_ADDRESSES: [u32; 4] = [0x000134, 0x000120, 0x000114, 0x000110];

const GPIO7_VECTOR_ADDRESS: u32 = 0x00013c;

// Private address we stick an RTE instruction at for vector returns and such
const RTE_ADDRESS: u32 = 0x000700;
// Private address we stick a STOP instruction at for unimplemented vectors
const STOP_ADDRESS: u32 = 0x000704;

const STACK_BASE: u32 = 0x600000;
const STACK_SIZE: u32 = 0x100000;
const STACK_TOP: u32 = STACK_BASE + STACK_SIZE;
const HEAP_BASE: u32 = 0x700000;
const HEAP_SIZE: u32 = 0x100000;

const SAMPLE_RATE: u32 = 48_000;

impl<'a> AtariSTe<'a>
{
	pub fn new(cpu: *mut c_void) -> Self
	{
		// Build instances of all the peripherals, including their interlinking
		let psg = YM2149::new(2_000_000, SAMPLE_RATE);
		let mfp = MC68901::new(2_457_600, cpu, 6);
		let dac = STeDac::new(50_066, mfp.clone());

		// Create an instance of the STe to work with
		let mut this = Self {
			memory_map: Box::new(MemoryMap::new(0x00ffffff)),
			cpu,
			psg,
			dac,
			mfp,
		};
		let peripherals = Box::as_mut_ptr(&mut this.memory_map) as *mut c_void;

		// Set up its memory map
		this.memory_map
			.add_peripheral_at(0x000000..0x800000, Ram::<STE_RAM_SIZE>::new().into());
		this.memory_map.add_peripheral_at(
			0xe00000..0xf00000,
			AtariSTeROMs::new(cpu, peripherals, HEAP_BASE, HEAP_SIZE).into(),
		);
		// Cartridge ROM at 0xfa0000, 128KiB
		// pre-TOS 2.0 OS ROMs at 0xfc0000, 128KiB
		this.add_clocked_peripheral(0xff8800..0xff8804, this.psg.to_owned().into());
		this.add_clocked_peripheral(0xff8900..0xff8926, this.dac.to_owned().into());
		this.add_clocked_peripheral(0xfffa00..0xfffa40, this.mfp.to_owned().into());

		// Set up our dummy RTE for vector handling
		this.memory_map.write_address(RTE_ADDRESS, 0x4e73u16);
		// Set up a dummy STOP for unimplemented vectors
		this.memory_map.write_address(STOP_ADDRESS, 0x4e72u16);
		this.memory_map.write_address(STOP_ADDRESS + 2, 0x2700u16);

		// Set up the illegal instruction handler using STOP
		this.memory_map.write_address(0x000010, STOP_ADDRESS);

		// Set up the TRAP handlers using the dummy STOP for unused ones
		this.memory_map.write_address(0x000080, STOP_ADDRESS);
		// Set up the GEMDOS TRAP handler so the ROM will handle it
		this.memory_map
			.write_address(0x000084, 0xe00000 + atari_ste_roms::HANDLER_ADDRESS_GEMDOS);
		this.memory_map.write_address(0x000088, STOP_ADDRESS);
		this.memory_map.write_address(0x00008c, STOP_ADDRESS);
		this.memory_map.write_address(0x000090, STOP_ADDRESS);
		this.memory_map.write_address(0x000094, STOP_ADDRESS);
		this.memory_map.write_address(0x000098, STOP_ADDRESS);
		this.memory_map.write_address(0x00009c, STOP_ADDRESS);
		this.memory_map.write_address(0x0000a0, STOP_ADDRESS);
		this.memory_map.write_address(0x0000a4, STOP_ADDRESS);
		this.memory_map.write_address(0x0000a8, STOP_ADDRESS);
		this.memory_map.write_address(0x0000ac, STOP_ADDRESS);
		this.memory_map.write_address(0x0000b0, STOP_ADDRESS);
		this.memory_map.write_address(0x0000b4, STOP_ADDRESS);
		this.memory_map.write_address(0x0000b8, STOP_ADDRESS);
		this.memory_map.write_address(0x0000bc, STOP_ADDRESS);

		// Make all timer handlers RTEs for now
		for address in TIMER_VECTOR_ADDRESSES {
			this.memory_map.write_address(address, RTE_ADDRESS);
		}
		// And the GPIO7 handler too
		this.memory_map.write_address(GPIO7_VECTOR_ADDRESS, RTE_ADDRESS);
		// Tell the MFP its vector number information and that we're in automatic mode
		this.memory_map.write_address(0xfffa17, 0x40u8);
		// Register it as an interrupt source
		unsafe { cpu_register_interrupt_requester_mfp(cpu, this.mfp.clone()) };

		this
	}

	fn add_clocked_peripheral(&mut self, mapping: Range<u32>, peripheral: Rc<RefCell<dyn ClockedPeripheral<u32> + 'a>>)
	{
		// Add the device to the clocking map according to the clocking ratio set up
		// XXX: Clocked peripherals map still needs setting up
		// Add the device to the address map
		self.memory_map.add_peripheral_at(mapping, peripheral);
	}
}

unsafe extern "C" {
	fn cpu_register_interrupt_requester_mfp(cpu: *mut c_void, mfp: MC68901);
}
