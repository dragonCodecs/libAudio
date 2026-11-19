// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use super::AtariSTe;
use crate::sndh::loader::SndhTimer;

#[test]
fn memory_map()
{
	let emulator = AtariSTe::new();
	let mut mmio = emulator.memory_map();

	// Check that the RAM is present and the GEMDOS trap handler has been configured properly
	assert_eq!(mmio.read_address::<u32>(0x000084), 0x00e00000);
	// Check the MFP is present and the vector register has been configured properly
	assert_eq!(mmio.read_address::<u8>(0xfffa17), 0x40);
	// Check the DMA DAC is present and defaulted to play 50066Hz mono
	assert_eq!(mmio.read_address::<u8>(0xff8921), 0x83);
	// Check the YM2149 is present and the mixer configured for silence
	mmio.write_address(0xff8800, 0x07u8);
	assert_eq!(mmio.read_address::<u8>(0xff8800), 0x3f);
}

#[test]
fn configure_timer()
{
	let mut emulator = AtariSTe::new();
	let mut mmio = emulator.memory_map();

	// Ask the STe to configure Timer C for 200Hz operation
	emulator.configure_timer(SndhTimer::C, 200);
	// Read back the interrupt vector address to make sure it points at the timer routine
	assert_eq!(mmio.read_address::<u32>(0x000114), 0x001008);
	// Now read back the MFP settings for Timer C to check they're correct
	assert_eq!(mmio.read_address::<u8>(0xfffa23), 192);
	assert_eq!(mmio.read_address::<u8>(0xfffa1d), 0x50);
	// Read back the settings area and make sure those values are correct
	assert_eq!(mmio.read_address::<u8>(0x001000), 64);
	assert_eq!(mmio.read_address::<u8>(0x001001), 192);
	assert_eq!(mmio.read_address::<u8>(0x001002), 0);
	assert_eq!(mmio.read_address::<u8>(0x001003), 0);

	// Turn the timer back off
	mmio.write_address(0xfffa1d, 0x00u8);

	// Now ask the STe to reconfigure Timer C for 50Hz operation
	emulator.configure_timer(SndhTimer::C, 50);
	// Read back the interrupt vector address to make sure it (still) points at the timer routine
	assert_eq!(mmio.read_address::<u32>(0x000114), 0x001008);
	// Now read back the MFP settings for Timer C to check they're correct
	assert_eq!(mmio.read_address::<u8>(0xfffa23), 245);
	assert_eq!(mmio.read_address::<u8>(0xfffa1d), 0x70);
	// Read back the settings area and make sure those values are correct
	assert_eq!(mmio.read_address::<u8>(0x001000), 200);
	assert_eq!(mmio.read_address::<u8>(0x001001), 245);
	assert_eq!(mmio.read_address::<u8>(0x001002), 152);
	assert_eq!(mmio.read_address::<u8>(0x001003), 0);
}
