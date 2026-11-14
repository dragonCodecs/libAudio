// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ptr::null_mut;

use super::MC68901;
use crate::emulator::Peripheral;

fn write_register_u8(mfp: &mut impl Peripheral<u32>, reg: u32, value: u8)
{
	mfp.write_address(reg, &[value]);
}

fn read_register_u8(mfp: &impl Peripheral<u32>, reg: u32) -> u8
{
	let mut value = [0; 1];
	mfp.read_address(reg, &mut value);
	value[0]
}

fn write_register_u16(mfp: &mut impl Peripheral<u32>, reg: u32, value: u16)
{
	mfp.write_address(reg, &value.to_be_bytes());
}

fn read_register_u16(mfp: &impl Peripheral<u32>, reg: u32) -> u16
{
	let mut value = [0; 2];
	mfp.read_address(reg, &mut value);
	u16::from_be_bytes(value)
}

fn write_register_u32(mfp: &mut impl Peripheral<u32>, reg: u32, value: u32)
{
	mfp.write_address(reg, &value.to_be_bytes());
}

fn read_register_u32(mfp: &impl Peripheral<u32>, reg: u32) -> u32
{
	let mut value = [0; 4];
	mfp.read_address(reg, &mut value);
	u32::from_be_bytes(value)
}

#[test]
fn register_io()
{
	let mut mfp = MC68901::new(2_457_600, null_mut(), 6);

	// Verify the function of the GPIO registers
	write_register_u16(&mut mfp, 0x00, 0xa55a);
	assert_eq!(read_register_u16(&mfp, 0x00), 0x005a);
	assert_eq!(read_register_u8(&mfp, 0x01), 0x5a);
	write_register_u8(&mut mfp, 0x01, 0xff);
	assert_eq!(read_register_u8(&mfp, 0x01), 0xff);
	write_register_u16(&mut mfp, 0x02, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x02), 0x00ff);
	write_register_u16(&mut mfp, 0x04, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x04), 0x00ff);
	// Verify the function of the interrupt control registers
	write_register_u16(&mut mfp, 0x06, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x06), 0x00ff);
	write_register_u16(&mut mfp, 0x08, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x08), 0x00ff);
	write_register_u16(&mut mfp, 0x0a, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x0a), 0x00ff);
	write_register_u16(&mut mfp, 0x0c, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x0c), 0x00ff);
	write_register_u16(&mut mfp, 0x0e, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x0e), 0x00ff);
	write_register_u16(&mut mfp, 0x10, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x10), 0x00ff);
	write_register_u16(&mut mfp, 0x12, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x12), 0x00ff);
	write_register_u16(&mut mfp, 0x14, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x14), 0x00ff);
	// Verify the function of the vector register
	write_register_u16(&mut mfp, 0x16, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x16), 0x00f8);
	// Verify the function of the timer registers
	write_register_u16(&mut mfp, 0x18, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x18), 0x001f);
	write_register_u16(&mut mfp, 0x1e, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x1e), 0x0000);
	write_register_u8(&mut mfp, 0x19, 0x00);
	write_register_u16(&mut mfp, 0x1a, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x1a), 0x001f);
	write_register_u16(&mut mfp, 0x20, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x20), 0x0000);
	write_register_u8(&mut mfp, 0x1b, 0x00);
	write_register_u16(&mut mfp, 0x1c, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x1c), 0x0077);
	write_register_u16(&mut mfp, 0x22, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x22), 0x0000);
	write_register_u16(&mut mfp, 0x24, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x24), 0x0000);
	write_register_u8(&mut mfp, 0x1d, 0x00);
	write_register_u16(&mut mfp, 0x1e, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x1e), 0x00ff);
	write_register_u16(&mut mfp, 0x20, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x20), 0x00ff);
	write_register_u16(&mut mfp, 0x22, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x22), 0x00ff);
	write_register_u16(&mut mfp, 0x24, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x24), 0x00ff);
	// Verify the function of the UART registers
	write_register_u16(&mut mfp, 0x26, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x26), 0x00ff);
	write_register_u16(&mut mfp, 0x28, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x28), 0x00ff);
	write_register_u16(&mut mfp, 0x2a, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x2a), 0x00ff);
	write_register_u16(&mut mfp, 0x2c, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x2c), 0x00ff);
	write_register_u16(&mut mfp, 0x2e, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x2e), 0x00ff);
}

#[test]
fn bad_begister_io()
{
	let mut mfp = MC68901::new(2_457_600, null_mut(), 6);

	// Should be discarded as writing to an even address for an odd byte register
	write_register_u8(&mut mfp, 0x00, 0xff);
	assert_eq!(read_register_u8(&mfp, 0x00), 0x00);
	assert_eq!(read_register_u16(&mfp, 0x00), 0x0000);
	// Should be discarded as writing 32 bits in a go is inadmissable
	write_register_u32(&mut mfp, 0x00, u32::MAX);
	assert_eq!(read_register_u16(&mfp, 0x00), 0x0000);
	assert_eq!(read_register_u16(&mfp, 0x02), 0x0000);
	// Should be a no-op as reading 32 bits in a go is inadmissable
	assert_eq!(read_register_u32(&mfp, 0x02), 0x00000000);
}

#[test]
fn configure_timer()
{
	let mut mfp = MC68901::new(2_457_600, null_mut(), 6);

	// Check that we can configure timer A for ~50Hz operation
	mfp.configure_timer(0, 245, 7);
	assert_eq!(read_register_u8(&mfp, 0x19), 0x07);
	assert_eq!(read_register_u8(&mfp, 0x1f), 245);

	// Check that we can configure timer C for 200Hz operation
	mfp.configure_timer(2, 192, 5);
	assert_eq!(read_register_u8(&mfp, 0x1d), 0x50);
	assert_eq!(read_register_u8(&mfp, 0x23), 192);

	// This call will be a no-op
	mfp.configure_timer(4, 192, 5);
}

#[test]
fn prescaling_mapping()
{
	assert_eq!(super::timer_prescaling_for(0), 1);
	assert_eq!(super::timer_prescaling_for(1), 4);
	assert_eq!(super::timer_prescaling_for(2), 10);
	assert_eq!(super::timer_prescaling_for(3), 16);
	assert_eq!(super::timer_prescaling_for(4), 50);
	assert_eq!(super::timer_prescaling_for(5), 64);
	assert_eq!(super::timer_prescaling_for(6), 100);
	assert_eq!(super::timer_prescaling_for(7), 200);
}
