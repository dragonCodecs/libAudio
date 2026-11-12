// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

pub mod sound;

pub trait Peripheral
{
	fn read_address(&self, address: u32, data: &mut [u8]);
	fn write_address(&mut self, address: u32, data: &[u8]);
}
