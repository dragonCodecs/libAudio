// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::cell::RefCell;
use std::collections::BTreeMap;
use std::ffi::c_void;
use std::ops::Range;
use std::rc::Rc;

use crate::emulator::atari_ste_roms::{self, AtariSTeROMs};
use crate::emulator::clock_manager::ClockManager;
use crate::emulator::ram::Ram;
use crate::emulator::sound::ste_dac::STeDac;
use crate::emulator::sound::ym2149::YM2149;
use crate::emulator::timing::mc68901::{MC68901, timer_prescaling_for};
use crate::emulator::{ClockedPeripheral, MemoryMap};
use crate::sndh::ice_decrunch::{self, SndhDecruncher};
use crate::sndh::loader::SndhTimer;

pub struct AtariSTe<'a>
{
	// m68k has a 24-bit address bus, but we can't directly represent that, so use a 32-bit address value instead.
	memory_map: Box<MemoryMap<'a, u32>>,

	cpu: *mut c_void,
	ram: StRam,
	psg: YM2149,
	dac: STeDac,
	mfp: MC68901,

	clocked_peripherals: BTreeMap<*mut (dyn ClockedPeripheral<u32> + 'a), ClockManager>,
}

struct UDiv
{
	quotient: u32,
	remainder: u32,
}

const SYSTEM_CLOCK_FREQUENCY: u32 = 8_000_000;
const STE_RAM_SIZE: usize = 8 * 1024 * 1024;
type StRam = Ram<STE_RAM_SIZE>;

const COOKIE_SND: [u8; 4] = *b"_SND";
const COOKIE_MCH: [u8; 4] = *b"_MCH";

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
		let ram = StRam::new();
		let psg = YM2149::new(2_000_000, SAMPLE_RATE);
		let mfp = MC68901::new(2_457_600, cpu, 6);
		let dac = STeDac::new(50_066, mfp.clone());

		// Create an instance of the STe to work with
		let mut this = Self {
			memory_map: Box::new(MemoryMap::new(0x00ffffff)),

			cpu,
			ram,
			psg,
			dac,
			mfp,

			clocked_peripherals: BTreeMap::new(),
		};
		let peripherals = Box::as_mut_ptr(&mut this.memory_map) as *mut c_void;

		// Set up its memory map
		this.memory_map
			.add_peripheral_at(0x000000..0x800000, this.ram.clone().into());
		this.memory_map.add_peripheral_at(
			0xe00000..0xf00000,
			AtariSTeROMs::new(cpu, peripherals, HEAP_BASE, HEAP_SIZE).into(),
		);
		// Cartridge ROM at 0xfa0000, 128KiB
		// pre-TOS 2.0 OS ROMs at 0xfc0000, 128KiB
		this.add_clocked_peripheral(0xff8800..0xff8804, this.psg.clone().into());
		this.add_clocked_peripheral(0xff8900..0xff8926, this.dac.clone().into());
		this.add_clocked_peripheral(0xfffa00..0xfffa40, this.mfp.clone().into());

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
		self.clocked_peripherals.insert(
			peripheral.as_ptr(),
			ClockManager::new(SYSTEM_CLOCK_FREQUENCY, peripheral.borrow().clock_frequency()),
		);
		// Add the device to the address map
		self.memory_map.add_peripheral_at(mapping, peripheral);
	}

	pub fn configure_timer(&mut self, timer: SndhTimer, timer_frequency: u16)
	{
		// Convert the timer's letter code into a timer index
		let Some(index) = timer.as_index() else {
			return;
		};
		// Extract where the IRQ handler is for the required timer
		let vector_address = TIMER_VECTOR_ADDRESSES[index];

		// Now the vector slot is properly set up, enable the timer and set the call frequency
		// Start by computing the integer number of cycles per second (and its error) the requested frequency
		// will beat against the MFP
		let cycle_rate = div_rem(self.mfp.clock_frequency(), timer_frequency as u32);
		// Now figure out what divider is necessary to make that fit the 8-bit counter
		let mode = {
			let mut scaled_rate = UDiv::default();
			let mut scale_factor = 7;
			for factor in 0..8 {
				// Calculate if this amount of prescaling results in a value that fits the timer counter
				let counter = div_rem(cycle_rate.quotient, timer_prescaling_for(factor));
				// If it does and the error is less than the previously calculated value,
				// note the factor and store the value
				if counter.quotient < 256 && counter.remainder < scaled_rate.remainder {
					scaled_rate = counter;
					scale_factor = factor;
				}
			}
			scale_factor
		};

		// With a scale factor picked, write the counter value, prescaling, and error into memory for the
		// timer IRQ handler to use to adjust for the error and maintain frequency accuracy
		let prescaler = timer_prescaling_for(mode);
		let counter = div_rem(cycle_rate.quotient, prescaler);
		self.memory_map.write_address(0x001000, prescaler as u8);
		self.memory_map.write_address(0x001001, counter.quotient as u8); // Reload value
		self.memory_map.write_address(0x001002, counter.remainder as u8); // Residual error
		self.memory_map.write_address(0x001003, 0u8);

		// Write the IRQ handler routine into memory so it can be used to trampoline into the SNDH play routine
		// Stack the registers we clobber
		self.memory_map.write_address(0x001008, 0x48e7u16);
		self.memory_map.write_address(0x00100a, 0xc080u16); // movem.l d0-d1,a0, -(sp)
		// Prepare d0 to accept the accumulated error value
		self.memory_map.write_address(0x00100c, 0x4240u16); // clr.w d0
		// Grab the accumulated error value
		self.memory_map.write_address(0x00100e, 0x103au16);
		self.memory_map.write_address(0x001010, 0xfff3u16); // move.b $-0d(pc), d0
		// Prepare d1 to accept the error residual value
		self.memory_map.write_address(0x001012, 0x4241u16); // clr.w d1
		// Grab the error residual value
		self.memory_map.write_address(0x001014, 0x123au16);
		self.memory_map.write_address(0x001016, 0xffecu16); // move.b $-14(pc), d1
		// Add the two together to get a new accumulation
		self.memory_map.write_address(0x001018, 0xd240u16); // add.w d0, d1
		// Grab the prescaling value to compare the accumulation against
		self.memory_map.write_address(0x00101a, 0x103au16);
		self.memory_map.write_address(0x00101c, 0xffe4u16); // move.b $-1c(pc), d0
		// Put the timer data register location in a0
		self.memory_map.write_address(0x00101e, 0x307cu16);
		self.memory_map.write_address(0x001020, 0xfa1fu16 + (index << 1) as u16); // movea.w #$fa1f, a0
		// Compare to see if prescaler < accumulation
		self.memory_map.write_address(0x001022, 0xb041u16); // cmp.w d1, d0
		// Skip to non-adjustment code if false
		self.memory_map.write_address(0x001024, 0x6c06u16); // bge +$06 (0x00102c)
		// Otherwise, add one to the reload value
		self.memory_map.write_address(0x001026, 0x5210u16); // addq.b #1, (a0)
		// And subtract the prescaler off the accumulated error
		self.memory_map.write_address(0x001028, 0x9240u16); // sub d1, d0
		// Jump to where we store the accumulated error
		self.memory_map.write_address(0x00102a, 0x6006u16); // bra +$06 (0x001032)
		// If the accumulation has not yet exceeded the prescaler value, load the original reload value back
		self.memory_map.write_address(0x00102c, 0x103au16);
		self.memory_map.write_address(0x00102e, 0xffd3u16); // move.b $-2d(pc), d0
		self.memory_map.write_address(0x001030, 0x1080u16); // move.b d0, (a0)
		// Figure out where the accumulated error value is
		self.memory_map.write_address(0x001032, 0x41fau16);
		self.memory_map.write_address(0x001034, 0xffcfu16); // lea $-31(pc), a0
		// Write the accumulated error back
		self.memory_map.write_address(0x001036, 0x1081u16); // move.b d1, (a0)
		// nstack the clobbered registers
		self.memory_map.write_address(0x001038, 0x4cdfu16);
		self.memory_map.write_address(0x00103a, 0x0103u16); // movew.l (sp)+, d0-d1,a0
		// Jump into the play routine
		self.memory_map.write_address(0x00103c, 0x207au16);
		self.memory_map.write_address(0x00103e, 0xffc6u16); // movea $-3a(pc), a0
		self.memory_map.write_address(0x001040, 0x4e90u16); // jsr (a0)
		// Finally, return from the interrupt handler
		self.memory_map.write_address(0x001042, 0x4e73u16); // rte

		// Configure the IRQ handler to our routine, and write the entry address in the SNDH where it can
		// get to it to `bsr` into the routine
		self.memory_map.write_address(0x001004, 0x010008u32);
		self.memory_map.write_address(vector_address, 0x001008u32);

		// Write the initial timer settings to the timer
		self.mfp.configure_timer(index, counter.quotient as u8, mode);
	}

	pub fn copy_to_ram(&mut self, data: &mut SndhDecruncher) -> Result<(), ice_decrunch::Error>
	{
		let system_ram = &mut self.ram;
		// Get a span that's past the end of the system variables space, and the length of the decrunched SNDH file
		// But that also excludes the heap and stack spaces
		let destination = &mut system_ram.index_mut(0..STACK_BASE as usize)[0x0100000..data.length()];
		// Now make sure we're at the start of the data and copy it all in
		data.seek_start();
		data.read_slice(destination)
	}

	pub fn init(&mut self, subtune: u16) -> bool
	{
		// Set up the cookie jar at 0x000600 so replay routines work properly
		self.memory_map.write_address(0x0005a0, 0x000600u32);
		self.memory_map.write_address(0x000600, u32::from_be_bytes(COOKIE_SND));
		// Bit 0 -> PSG, bit 1 -> DMA sound, bit 2 -> CODEC, bit 3 -> DSP
		self.memory_map.write_address(0x000604, 0x00000003u32);
		self.memory_map.write_address(0x000608, u32::from_be_bytes(COOKIE_MCH));
		// Set the machine type to an STe
		self.memory_map.write_address(0x00060c, 0x00010000u32);
		// Sentinel that ends the cookie jar
		self.memory_map.write_address(0x000610, 0u32);

		// Set up the calling context
		unsafe { cpu_write_data_register(self.cpu, 0, subtune as u32 + 1) };
		// And run the init entry point to return
		unsafe { cpu_execute_to_return(self.cpu, 0x010000, STACK_TOP, false) }
	}

	pub fn exit(&mut self) -> bool
	{
		// Run the exit entry point to return
		unsafe { cpu_execute_to_return(self.cpu, 0x010004, STACK_TOP, false) }
	}
}

impl Default for UDiv
{
	fn default() -> Self
	{
		Self {
			quotient: 0,
			remainder: u32::MAX,
		}
	}
}

fn div_rem(numerator: u32, denominator: u32) -> UDiv
{
	UDiv {
		quotient: numerator / denominator,
		remainder: numerator % denominator,
	}
}

unsafe extern "C" {
	#[allow(improper_ctypes)]
	fn cpu_register_interrupt_requester_mfp(cpu: *mut c_void, mfp: MC68901);
	fn cpu_write_data_register(cpu: *mut c_void, reg: usize, value: u32);
	fn cpu_execute_to_return(cpu: *mut c_void, entry_address: u32, stack_top: u32, as_user: bool) -> bool;
}
