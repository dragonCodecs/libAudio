// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::cell::{Ref, RefCell, RefMut};
use std::ops::{Index, IndexMut, Range};
use std::rc::Rc;

use num_traits::{AsPrimitive, Unsigned};

use crate::emulator::Peripheral;

pub struct Ram<const N: usize>
{
	inner: Rc<RefCell<RamInner<N>>>,
}

struct RamInner<const N: usize>
{
	memory: [u8; N],
}

impl<const N: usize> Ram<N>
{
	pub fn new() -> Self
	{
		Self {
			inner: Rc::new(RefCell::new(RamInner::new())),
		}
	}

	pub fn index(&self, index: Range<usize>) -> Ref<'_, [u8]>
	{
		Ref::map(self.inner.borrow(), |inner| inner.index(index))
	}

	pub fn index_mut(&mut self, index: Range<usize>) -> RefMut<'_, [u8]>
	{
		RefMut::map(self.inner.borrow_mut(), |inner| inner.index_mut(index))
	}
}

impl<Address, const N: usize> Peripheral<Address> for Ram<N>
where
	Address: Unsigned + AsPrimitive<usize>,
{
	fn read_address(&self, address: Address, data: &mut [u8])
	{
		self.inner.borrow().read_address(address, data);
	}

	fn write_address(&mut self, address: Address, data: &[u8])
	{
		self.inner.borrow_mut().write_address(address, data);
	}
}

impl<'a, Address, const N: usize> From<Ram<N>> for Rc<RefCell<dyn Peripheral<Address> + 'a>>
where
	Address: Unsigned + AsPrimitive<usize>,
{
	fn from(ram: Ram<N>) -> Self
	{
		ram.inner
	}
}

impl<const N: usize> RamInner<N>
{
	pub fn new() -> Self
	{
		Self {
			memory: [0; N],
		}
	}
}

impl<Address, const N: usize> Peripheral<Address> for RamInner<N>
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

impl<const N: usize> Index<Range<usize>> for RamInner<N>
{
	type Output = [u8];

	fn index(&self, index: Range<usize>) -> &Self::Output
	{
		&self.memory[index]
	}
}

impl<const N: usize> IndexMut<Range<usize>> for RamInner<N>
{
	fn index_mut(&mut self, index: Range<usize>) -> &mut Self::Output
	{
		&mut self.memory[index]
	}
}
