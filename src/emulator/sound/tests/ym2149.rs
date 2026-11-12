// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use super::Ym2149;
use crate::emulator::Peripheral;

fn write_register(psg: &mut impl Peripheral, reg: u8, value: u8)
{
	psg.write_address(0, &[reg]);
	psg.write_address(2, &[value]);
}

fn read_register(psg: &mut impl Peripheral, reg: u8) -> u8
{
	psg.write_address(0, &[reg]);
	let mut result = [0; 1];
	psg.read_address(0, &mut result);
	result[0]
}

#[test]
fn register_io()
{
	// Set up a dummy PSG
	let mut psg = Ym2149::new(2_000_000, 48_000);
	// For each register, try to write and then read back some value
	// Channel A frequency
	assert_eq!(read_register(&mut psg, 0), 0x00);
	write_register(&mut psg, 0, 0xff);
	assert_eq!(read_register(&mut psg, 0), 0xff);
	assert_eq!(read_register(&mut psg, 1), 0x00);
	write_register(&mut psg, 1, 0xff);
	assert_eq!(read_register(&mut psg, 0), 0xff);
	assert_eq!(read_register(&mut psg, 1), 0x0f);
	// Channel B frequency
	assert_eq!(read_register(&mut psg, 2), 0x00);
	write_register(&mut psg, 2, 0xff);
	assert_eq!(read_register(&mut psg, 2), 0xff);
	assert_eq!(read_register(&mut psg, 3), 0x00);
	write_register(&mut psg, 3, 0xff);
	assert_eq!(read_register(&mut psg, 2), 0xff);
	assert_eq!(read_register(&mut psg, 3), 0x0f);
	// Channel C frequency
	assert_eq!(read_register(&mut psg, 4), 0x00);
	write_register(&mut psg, 4, 0xff);
	assert_eq!(read_register(&mut psg, 4), 0xff);
	assert_eq!(read_register(&mut psg, 5), 0x00);
	write_register(&mut psg, 5, 0xff);
	assert_eq!(read_register(&mut psg, 4), 0xff);
	assert_eq!(read_register(&mut psg, 5), 0x0f);
	// Noise frequency
	assert_eq!(read_register(&mut psg, 6), 0x00);
	write_register(&mut psg, 6, 0xff);
	assert_eq!(read_register(&mut psg, 6), 0x1f);
	// Mixer settings
	assert_eq!(read_register(&mut psg, 7), 0x3f);
	write_register(&mut psg, 7, 0xff);
	assert_eq!(read_register(&mut psg, 7), 0xff);
	// Channel level settings
	assert_eq!(read_register(&mut psg, 8), 0x00);
	write_register(&mut psg, 8, 0xff);
	assert_eq!(read_register(&mut psg, 8), 0x1f);
	assert_eq!(read_register(&mut psg, 9), 0x00);
	write_register(&mut psg, 9, 0xff);
	assert_eq!(read_register(&mut psg, 9), 0x1f);
	assert_eq!(read_register(&mut psg, 10), 0x00);
	write_register(&mut psg, 10, 0xff);
	assert_eq!(read_register(&mut psg, 10), 0x1f);
	// Envelope frequency
	assert_eq!(read_register(&mut psg, 11), 0x00);
	write_register(&mut psg, 11, 0xff);
	assert_eq!(read_register(&mut psg, 11), 0xff);
	assert_eq!(read_register(&mut psg, 12), 0x00);
	write_register(&mut psg, 12, 0xff);
	assert_eq!(read_register(&mut psg, 12), 0xff);
	// Envelope shape
	assert_eq!(read_register(&mut psg, 13), 0x00);
	write_register(&mut psg, 13, 0xff);
	assert_eq!(read_register(&mut psg, 13), 0x0f);
	// GPIO ports
	assert_eq!(read_register(&mut psg, 14), 0x00);
	write_register(&mut psg, 14, 0xff);
	assert_eq!(read_register(&mut psg, 14), 0xff);
	assert_eq!(read_register(&mut psg, 15), 0x00);
	write_register(&mut psg, 15, 0xff);
	assert_eq!(read_register(&mut psg, 15), 0xff);
}
