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
use std::ops::AddAssign;

#[allow(unused)]
pub struct STeDac
{
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

#[derive(Default)]
#[allow(unused)]
struct Register24b
{
	value: u32,
}

impl STeDac
{
	pub fn new() -> Self
	{
		Self {
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
