// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ptr::null_mut;

use super::STeDac;
use crate::emulator::Peripheral;
use crate::emulator::timing::mc68901::MC68901;

fn write_register_u8(periph: &mut impl Peripheral<u32>, reg: u32, value: u8)
{
	periph.write_address(reg, &[value]);
}

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

fn write_register_u32(periph: &mut impl Peripheral<u32>, reg: u32, value: u32)
{
	periph.write_address(reg, &value.to_be_bytes());
}

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

	// Set up another write to the main volume register to set the volume to max (40 internally, 64 externally)
	write_register_u16(&mut dac, 0x22, 0x09d1);
	assert_eq!(read_register_u16(&dac, 0x22), 0x09d1);
	// Execute the transaction
	run_microwire_cycle(&dac, 0x0ffe);
	// Read the main volume to check what it's now set to (should be 64)
	assert_eq!(dac.output_level(), 0x40);

	// Set up another write to the main volume register to set the volume to mid (20 internally, 32 externally)
	write_register_u16(&mut dac, 0x22, 0x09a9);
	assert_eq!(read_register_u16(&dac, 0x22), 0x09a9);
	// Execute the transaction
	run_microwire_cycle(&dac, 0x0ffe);
	// Read the main volume to check what it's now set to (should be 32)
	assert_eq!(dac.output_level(), 0x20);

	// Set up another write to the main volume register to over-set the volume to 63
	write_register_u16(&mut dac, 0x22, 0x09ff);
	assert_eq!(read_register_u16(&dac, 0x22), 0x09ff);
	// Execute the transaction
	run_microwire_cycle(&dac, 0x0ffe);
	// Read the main volume to check what it's now set to (should be 64)
	assert_eq!(dac.output_level(), 0x40);

	// Set up a write to the left front fader register
	write_register_u16(&mut dac, 0x22, 0x0aa9);
	assert_eq!(read_register_u16(&dac, 0x22), 0x0aa9);
	// Execute the transaction
	run_microwire_cycle(&dac, 0x0ffe);
	// Read the main volume to check what it's now set to (should be 64)
	assert_eq!(dac.output_level(), 0x40);

	// Set up a write to the right front fader register
	write_register_u16(&mut dac, 0x22, 0x0a29);
	assert_eq!(read_register_u16(&dac, 0x22), 0x0a29);
	// Execute the transaction
	run_microwire_cycle(&dac, 0x0ffe);
	// Read the main volume to check what it's now set to (should be 64)
	assert_eq!(dac.output_level(), 0x40);

	// Do a nonsense write with the wrong number of bits used (should be discarded)
	write_register_u16(&mut dac, 0x24, 0x0ff0);
	write_register_u16(&mut dac, 0x22, 0x0981);
	run_microwire_cycle(&dac, 0x0ff0);
	// Read the main volume to check what it's now set to (should be 64)
	assert_eq!(dac.output_level(), 0x40);
}

#[test]
fn dma_register_io()
{
	let mfp = MC68901::new(2_457_600, null_mut(), 6);
	let mut dac = STeDac::new(50_066, mfp);

	write_register_u16(&mut dac, 0x00, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x00), 0x0003);
	write_register_u16(&mut dac, 0x02, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x02), 0x00ff);
	write_register_u16(&mut dac, 0x04, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x04), 0x00ff);
	write_register_u16(&mut dac, 0x06, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x06), 0x00fe);
	write_register_u16(&mut dac, 0x08, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x08), 0x00ff);
	write_register_u16(&mut dac, 0x0a, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x0a), 0x00ff);
	write_register_u16(&mut dac, 0x0c, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x0c), 0x00fe);
	write_register_u16(&mut dac, 0x0e, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x0e), 0x00ff);
	write_register_u16(&mut dac, 0x10, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x10), 0x00ff);
	write_register_u16(&mut dac, 0x12, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x12), 0x00fe);
	write_register_u16(&mut dac, 0x20, 0xffff);
	assert_eq!(read_register_u16(&dac, 0x20), 0x0083);
}

#[test]
fn bad_dma_register_io()
{
	let mfp = MC68901::new(2_457_600, null_mut(), 6);
	let mut dac = STeDac::new(50_066, mfp);

	// 8-bit writes to even numbered registers should be discarded as all 8-bit registers here are odd byte
	write_register_u8(&mut dac, 0x00, 0xff);
	assert_eq!(read_register_u16(&dac, 0x00), 0x0000);
	// 16-bit writes to odd addresses should be inadmissable
	write_register_u16(&mut dac, 0x01, 0xc33c);
	assert_eq!(read_register_u8(&dac, 0x01), 0x00);
	// 32-bit writes similarly are inadmissable and should be discarded
	write_register_u32(&mut dac, 0x00, u32::MAX);
	assert_eq!(read_register_u8(&dac, 0x01), 0x00);
	assert_eq!(read_register_u8(&dac, 0x03), 0x00);
	// 32-bit reads should also be inadmissable and discarded
	assert_eq!(read_register_u32(&dac, 0x00), 0x00000000);
	// Likewise 16-bit odd address reads
	assert_eq!(read_register_u16(&dac, 0x01), 0x0000);
	// And 8-bit even reads
	assert_eq!(read_register_u8(&dac, 0x00), 0x00);
}
