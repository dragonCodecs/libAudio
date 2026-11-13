// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ops::{Index, IndexMut, Range};

use num_traits::{AsPrimitive, Unsigned};

use crate::emulator::Peripheral;

pub struct Ram<const N: usize>
{
	memory: [u8; N],
}

impl<const N: usize> Ram<N>
{
	pub fn new() -> Self
	{
		Self {
			memory: [0; N],
		}
	}
}

impl<Address, const N: usize> Peripheral<Address> for Ram<N>
where
	Address: Unsigned + AsPrimitive<usize>,
{
	fn read_address(&self, address: Address, data: &mut [u8])
	{
		let address = address.as_();
		data.copy_from_slice(&self.memory[address..address + data.len()]);
	}

	fn write_address(&mut self, address: Address, data: &[u8])
	{
		let address = address.as_();
		self.memory[address..address + data.len()].copy_from_slice(data);
	}
}

impl<const N: usize> Index<Range<usize>> for Ram<N>
{
	type Output = [u8];

	fn index(&self, index: Range<usize>) -> &Self::Output
	{
		&self.memory[index]
	}
}

impl<const N: usize> IndexMut<Range<usize>> for Ram<N>
{
	fn index_mut(&mut self, index: Range<usize>) -> &mut Self::Output
	{
		&mut self.memory[index]
	}
}
