// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use num_traits::sign::Unsigned;

mod clock_manager;
pub mod sound;
pub mod timing;

pub trait Peripheral<Address>
where
	Address: Unsigned,
{
	fn read_address(&self, address: Address, data: &mut [u8]);
	fn write_address(&mut self, address: Address, data: &[u8]);
}

pub trait ClockedPeripheral<Address>: Peripheral<Address>
where
	Address: Unsigned,
{
	#[allow(unused)]
	#[must_use]
	fn clock_frequency(&self) -> u32;
	#[must_use]
	fn clock_cycle(&mut self) -> bool;
}
