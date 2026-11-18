// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use super::AtariSTe;

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
