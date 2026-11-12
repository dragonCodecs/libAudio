// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

// Implements the Atari STe's Sound DMA, Microwire and DAC interfaces
//
// References:
// - Sound DMA is implemented by the GST Shifter chip, no official documentation exists however some RE has been done as
//   detailed here: https://info-coach.fr/atari/hardware/STE-HW.php#shifter
// - The Microwire interface is under-documented, but at heart provides access to the registers of various blocks on the
//   STe, including the LMC1992 which provides volume control
// - LMC1992 is a National Semi part through which all audio flows to get out the STe archived datasheet: https://www.tomek.cedro.info/files/electronics/doc/ic_various/LMC1992.PDF
// - STe and other schematics: https://docs.dev-docs.org/htm/search.php?find=_s

use std::cell::RefCell;
use std::ffi::c_void;
use std::ops::AddAssign;

use crate::emulator::Peripheral;

pub struct STeDac
{
	mfp: *mut c_void,

	// Values that control the DMA engine
	begin_address: Register24b,
	end_address: Register24b,
	sample_address: Register24b,
	control: u8,
	sample_mono: bool,
	sample_rate_divider: u8,
	sample_rate_counter: u8,

	// RefCell for internal mutability inside microwire_cycle(), so we don't have problems
	// with read_address being it takes `&self`, but these need mutability during that processing.
	microwire_data: u16,
	microwire_mask: RefCell<u16>,
	microwire_cycles: RefCell<u8>,
	main_volume: u8,
}

#[derive(Default, Clone, Copy, PartialEq, Eq)]
struct Register24b
{
	value: u32,
}

impl STeDac
{
	pub fn new(mfp: *mut c_void) -> Self
	{
		Self {
			mfp,

			begin_address: Register24b::default(),
			end_address: Register24b::default(),
			sample_address: Register24b::default(),
			control: 0,
			sample_mono: true,
			sample_rate_divider: 0,
			sample_rate_counter: 0,

			microwire_data: 0,
			microwire_mask: RefCell::new(0),
			microwire_cycles: RefCell::new(0),
			main_volume: 64,
		}
	}

	pub fn clock_cycle(&mut self) -> bool
	{
		// Check if there's anything playing or not, and if not then exit early
		if (self.control & (1 << 0)) == 0x00 {
			return true;
		}

		// Calculate the current sample rate tick count to hit the required play rate
		let sample_rate_ticks = {
			let ticks = 1 << self.sample_rate_divider;
			if self.sample_mono {
				ticks
			} else {
				// Stero playback requires us stretch the tick rate to half to get the correct cadence
				ticks << 1
			}
		};
		// Something's playing, great.. step the rate counter to see if we should do something
		// in this cycle, and check if the counter overflowed
		self.sample_rate_counter += 1;
		if self.sample_rate_counter == sample_rate_ticks {
			// Yes, so reset it
			self.sample_rate_counter = 0;
		} else {
			// No, okay - we're done.. exit early
			return true;
		}

		// Step the sample counter based on whether we're playing mono or stereo
		self.sample_address += if self.sample_mono {
			1
		} else {
			2
		};

		// If the sample address equals the end address
		if self.sample_address == self.end_address {
			// Reset the counter back to the start
			self.sample_address = self.begin_address;
			// If we're not looping playback, disable DMA
			if (self.control & (1 << 1)) == 0x00 {
				self.control &= 0xfe;
			}
			// Ping the MFP with the completion event
			unsafe { mc68901_fire_dma_event(self.mfp) };
		}

		true
	}

	fn run_microwire_transaction(&mut self)
	{
		// Every microwire transaction is 16 cycles
		let microwire_cycles = 16;
		*self.microwire_cycles.borrow_mut() = microwire_cycles;
		let microwire_mask = *self.microwire_mask.borrow();
		// Cycle through the bits and count the number actually used by the mask,
		// building a condensed value to work with
		let (value, bitsUsed) = {
			let mut data = 0;
			let mut bits = 0usize;
			// Loop through all the bits
			for bit in 0..microwire_cycles {
				// If this bit is used per the mask
				if (microwire_mask & (1 << bit)) != 0 {
					// Then copy the value of the data into our local and count it
					data |= ((self.microwire_data >> bit) & 1) << bits;
					bits += 1;
				}
			}
			(data, bits)
		};
		// Now check for a specific write of a specific width representing an access
		// to the main volume control register for the DAC
		if bitsUsed == 1 && (value >> 9) == 2 {
			// Extract out the write data and the write address
			let data = value & 0x3f;
			let address = (value >> 6) & 0x07;
			// And see where the write is targeted
			#[allow(clippy::single_match)]
			match address {
				// DAC main volume register
				3 => {
					if data > 40 {
						// If the value of the write is more than the max allowed volume, cap it
						self.main_volume = 64;
					} else {
						// Otherwise rescale it to our internal 0-64 levels
						self.main_volume = ((data * 64) / 40) as u8;
					}
				},
				_ => {},
			};
		}
	}

	fn microwire_cycle(&self) -> u16
	{
		// If there are still cycles to do
		if *self.microwire_cycles.borrow() != 0 {
			*self.microwire_cycles.borrow_mut() -= 1;
			// Rotate the mask one more bit left
			let mut microwire_mask = self.microwire_mask.borrow_mut();
			*microwire_mask = microwire_mask.rotate_left(1);
		}
		*self.microwire_mask.borrow()
	}
}

impl Peripheral for STeDac
{
	fn read_address(&self, address: u32, data: &mut [u8])
	{
		// Extract how wide an access this is
		let access_width = data.len();
		// Only admit 8- and 16-bit reads
		if access_width > 2 {
			return;
		}
		// Microwire registers are only accessible as u16's
		if access_width == 2 {
			// u16 accesses must be to even addresses
			if (address & 1) != 0 {
				return;
			}

			// Handle microwire address accesses
			match address {
				0x22 => data.copy_from_slice(&self.microwire_data.to_be_bytes()),
				0x24 => data.copy_from_slice(&self.microwire_cycle().to_be_bytes()),
				// Redirect everything else to 8-bit access
				_ => self.read_address(address + 1, &mut data[1..2]),
			}
		}
		// The rest of the registers are only accessible as u8's
		else if access_width == 1 {
			// Registers in this peripheral are on the odd byte
			if (address & 1) != 1 {
				return;
			}

			// Handle DMA sound system register accesses
			match address >> 1 {
				0 => data[0] = self.control,
				1..=3 => data[0] = self.begin_address.read_byte(((address >> 1) - 1) as u8),
				4..=6 => data[0] = self.sample_address.read_byte(((address >> 1) - 4) as u8),
				7..=9 => data[0] = self.end_address.read_byte(((address >> 1) - 7) as u8),
				// Convert the sample channel count and rate divider back into their forms for the peripheral interface
				10 => data[0] = ((self.sample_mono as u8) << 7) | (3 - self.sample_rate_divider),
				_ => {},
			}
		}
	}

	fn write_address(&mut self, address: u32, data: &[u8])
	{
		// Extract how wide an access this is
		let access_width = data.len();
		// Only admit 8- and 16-bit reads
		if access_width > 2 {
			return;
		}
		// Microwire registers are only accessible as u16's
		if access_width == 2 {
			// u16 accesses must be to even addresses
			if (address & 1) != 0 {
				return;
			}

			// Handle microwire address accesses
			match address {
				0x22 => {
					self.microwire_data = u16::from_be_bytes(
						*data
							.as_array::<2>()
							.expect("Slice of 2 bytes converts to array of 2 bytes"),
					);
					self.run_microwire_transaction();
				},
				0x24 => {
					*self.microwire_mask.borrow_mut() = u16::from_be_bytes(
						*data
							.as_array::<2>()
							.expect("Slice of 2 bytes converts to array of 2 bytes"),
					)
				},
				// Redirect everything else to 8-bit access
				_ => self.write_address(address + 1, &data[1..2]),
			}
		}
		// The rest of the registers are only accessible as u8's
		else if access_width == 1 {
			// Registers in this peripheral are on the odd byte
			if (address & 1) != 1 {
				return;
			}

			// Handle DMA sound system register accesses
			match address >> 1 {
				0 => {
					// If playback state is being switched, reset the sample address
					if (self.control & (1 << 0)) != (data[0] & (1 << 0)) {
						self.sample_address = self.begin_address;
					}
					// Only the bottom two bits of the control byte are valid
					self.control = data[0] & 0x03;
				},
				1..=3 => self.begin_address.write_byte(((address >> 1) - 1) as u8, data[0]),
				4..=6 => self.sample_address.write_byte(((address >> 1) - 4) as u8, data[0]),
				7..=9 => self.end_address.write_byte(((address >> 1) - 7) as u8, data[0]),
				10 => {
					// Determine whether the new mode should be mono or stereo
					self.sample_mono = (data[0] & (1 << 7)) == (1 << 7);
					// Grab the frequency bits and turn them into a divider (3 == 2^3, 0 == 2^0)
					self.sample_rate_divider = 3 - (data[0] & 0x03);
					// Reset the rate counter to adjust for the new sample rate
					self.sample_rate_counter = 0;
				},
				_ => {},
			}
		}
	}
}

impl Register24b
{
	pub fn write_byte(&mut self, position: u8, byte: u8)
	{
		let shift = 8 * (2 - position as usize);
		self.value &= !(0xff << shift);
		self.value |= (byte as u32) << shift;
		// The bottom most bit in these registers must remain 0, as must the top 8
		self.value &= 0x00fffffe;
	}

	pub fn read_byte(&self, position: u8) -> u8
	{
		let shift = 8 * (2 - position as usize);
		(self.value >> shift) as u8
	}
}

impl AddAssign<u32> for Register24b
{
	fn add_assign(&mut self, rhs: u32)
	{
		self.value += rhs;
	}
}

unsafe extern "C" {
	fn mc68901_fire_dma_event(this: *mut c_void);
}
