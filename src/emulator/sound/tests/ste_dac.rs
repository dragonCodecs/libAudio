// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ptr::null_mut;

use super::STeDac;
use crate::emulator::Peripheral;
use crate::emulator::timing::mc68901::MC68901;

#[allow(unused)]
fn write_register_u8(periph: &mut impl Peripheral<u32>, reg: u32, value: u8)
{
	periph.write_address(reg, &[value]);
}

#[allow(unused)]
fn read_register_u8(periph: &impl Peripheral<u32>, reg: u32) -> u8
{
	let mut value = [0; 1];
	periph.read_address(reg, &mut value);
	value[0]
}

fn write_register_u16(periph: &mut impl Peripheral<u32>, reg: u32, value: u16)
{
	periph.write_address(reg, &value.to_be_bytes());
}

fn read_register_u16(periph: &impl Peripheral<u32>, reg: u32) -> u16
{
	let mut value = [0; 2];
	periph.read_address(reg, &mut value);
	u16::from_be_bytes(value)
}

#[allow(unused)]
fn write_register_u32(periph: &mut impl Peripheral<u32>, reg: u32, value: u32)
{
	periph.write_address(reg, &value.to_be_bytes());
}

#[allow(unused)]
fn read_register_u32(periph: &impl Peripheral<u32>, reg: u32) -> u32
{
	let mut value = [0; 4];
	periph.read_address(reg, &mut value);
	u32::from_be_bytes(value)
}

fn run_microwire_cycle(dac: &STeDac, mut mask: u16)
{
	// Microwire cycles take 16 reads of the mask register
	for _ in 0..16 {
		// Compute what the next mask value should be
		mask = mask.rotate_left(1);
		// Read the mask register (which actions the transaction)
		assert_eq!(read_register_u16(dac, 0x24), mask);
	}
}

#[test]
fn microwire()
{
	let mfp = MC68901::new(2_457_600, null_mut(), 6);
	let mut dac = STeDac::new(50_066, mfp);

	// Set up a write to the main volume register
	write_register_u16(&mut dac, 0x24, 0x0ffe);
	assert_eq!(read_register_u16(&dac, 0x24), 0x0ffe);
	// Set the volume to 0
	write_register_u16(&mut dac, 0x22, 0x0981);
	assert_eq!(read_register_u16(&dac, 0x22), 0x0981);
	// Execute the transaction
	run_microwire_cycle(&dac, 0x0ffe);
	// Read the main volume to check what it's now set to (should be 0)
	assert_eq!(dac.output_level(), 0);
}
