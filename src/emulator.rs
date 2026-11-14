// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::cell::{Ref, RefCell, RefMut};
use std::cmp::Ordering;
use std::collections::BTreeMap;
use std::ops::{BitAnd, Range};
use std::rc::Rc;

use num_traits::sign::Unsigned;

pub mod atari_ste;
pub mod atari_ste_roms;
mod clock_manager;
pub mod cpu;
mod gemdos_alloc;
pub mod ram;
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

#[derive(PartialEq, Eq)]
pub struct MemoryRange<Address>
where
	Address: Copy + Unsigned + PartialEq + Eq + PartialOrd + Ord,
{
	begin: Address,
	end: Address,
}

pub struct MemoryMap<'a, Address>
where
	Address: Copy + Unsigned + BitAnd<Output = Address> + PartialEq + Eq + PartialOrd + Ord,
{
	valid_address_mask: Address,
	address_map: BTreeMap<MemoryRange<Address>, Rc<RefCell<dyn Peripheral<Address> + 'a>>>,
}

pub trait PeripheralValue
{
	fn read_from_be<Address>(peripheral: Ref<'_, dyn Peripheral<Address>>, address: Address) -> Self
	where
		Address: Unsigned;

	fn write_to_be<Address>(&self, peripheral: RefMut<'_, dyn Peripheral<Address>>, address: Address)
	where
		Address: Unsigned;
}

impl PeripheralValue for u8
{
	fn read_from_be<Address>(peripheral: Ref<'_, dyn Peripheral<Address>>, address: Address) -> Self
	where
		Address: Unsigned,
	{
		let mut bytes = [0];
		peripheral.read_address(address, &mut bytes);
		bytes[0]
	}

	fn write_to_be<Address>(&self, mut peripheral: RefMut<'_, dyn Peripheral<Address>>, address: Address)
	where
		Address: Unsigned,
	{
		let bytes = [*self];
		peripheral.write_address(address, &bytes);
	}
}

impl PeripheralValue for u16
{
	fn read_from_be<Address>(peripheral: Ref<'_, dyn Peripheral<Address>>, address: Address) -> Self
	where
		Address: Unsigned,
	{
		let mut bytes = [0; 2];
		peripheral.read_address(address, &mut bytes);
		u16::from_be_bytes(bytes)
	}

	fn write_to_be<Address>(&self, mut peripheral: RefMut<'_, dyn Peripheral<Address>>, address: Address)
	where
		Address: Unsigned,
	{
		let bytes = self.to_be_bytes();
		peripheral.write_address(address, &bytes);
	}
}

impl PeripheralValue for u32
{
	fn read_from_be<Address>(peripheral: Ref<'_, dyn Peripheral<Address>>, address: Address) -> Self
	where
		Address: Unsigned,
	{
		let mut bytes = [0; 4];
		peripheral.read_address(address, &mut bytes);
		u32::from_be_bytes(bytes)
	}

	fn write_to_be<Address>(&self, mut peripheral: RefMut<'_, dyn Peripheral<Address>>, address: Address)
	where
		Address: Unsigned,
	{
		let bytes = self.to_be_bytes();
		peripheral.write_address(address, &bytes);
	}
}

impl PeripheralValue for u64
{
	fn read_from_be<Address>(peripheral: Ref<'_, dyn Peripheral<Address>>, address: Address) -> Self
	where
		Address: Unsigned,
	{
		let mut bytes = [0; 8];
		peripheral.read_address(address, &mut bytes);
		u64::from_be_bytes(bytes)
	}

	fn write_to_be<Address>(&self, mut peripheral: RefMut<'_, dyn Peripheral<Address>>, address: Address)
	where
		Address: Unsigned,
	{
		let bytes = self.to_be_bytes();
		peripheral.write_address(address, &bytes);
	}
}

impl<Address> MemoryRange<Address>
where
	Address: Copy + Unsigned + PartialEq + Eq + PartialOrd + Ord,
{
	/// Check if a given address is in this particular address range
	pub fn in_range(&self, address: Address) -> bool
	{
		self.begin <= address && address < self.end
	}

	/// Convert the address to one relative to the start of the range
	pub fn relative(&self, address: Address) -> Address
	{
		address - self.begin
	}
}

impl<Address> PartialOrd<Self> for MemoryRange<Address>
where
	Address: Copy + Unsigned + PartialEq + Eq + PartialOrd + Ord,
{
	fn partial_cmp(&self, other: &Self) -> Option<Ordering>
	{
		Some(self.cmp(other))
	}
}

impl<Address> Ord for MemoryRange<Address>
where
	Address: Copy + Unsigned + PartialEq + Eq + PartialOrd + Ord,
{
	fn cmp(&self, other: &Self) -> Ordering
	{
		if self.end <= other.begin {
			// If we end before the other begins, we're before it
			Ordering::Less
		} else if self.begin >= other.end {
			// If we begin after the other ends, we're after it
			Ordering::Greater
		} else {
			// If however the ranges somehow overlap then there is no ordering
			Ordering::Equal
		}
	}
}

impl<Address> From<Range<Address>> for MemoryRange<Address>
where
	Address: Copy + Unsigned + PartialEq + Eq + PartialOrd + Ord,
{
	fn from(range: Range<Address>) -> Self
	{
		Self {
			begin: range.start,
			end: range.end,
		}
	}
}

impl<'a, Address> MemoryMap<'a, Address>
where
	Address: Copy + Unsigned + BitAnd<Output = Address> + PartialEq + Eq + PartialOrd + Ord,
{
	pub fn new(valid_address_mask: Address) -> Self
	{
		Self {
			valid_address_mask,
			address_map: BTreeMap::new(),
		}
	}

	pub fn add_peripheral_at(&mut self, mapping: Range<Address>, peripheral: Rc<RefCell<dyn Peripheral<Address> + 'a>>)
	{
		self.address_map.insert(mapping.into(), peripheral);
	}

	#[allow(unused)]
	pub fn read_address<Value>(&self, address: Address) -> Value
	where
		Value: Default + Unsigned + PeripheralValue,
	{
		let adjusted_address = address & self.valid_address_mask;
		// Try to find a peripheral mapped for the address
		for (mapping, peripheral) in self.address_map.iter() {
			// If the peripheral's mapping contains this address
			if mapping.in_range(adjusted_address) {
				// Convert the address to a relative one
				let relative_address = mapping.relative(adjusted_address);
				// Read the data associated with that address in the peripheral
				return Value::read_from_be(peripheral.borrow(), relative_address);
			}
		}
		Value::default()
	}

	pub fn write_address<Value>(&mut self, address: Address, data: Value)
	where
		Value: Unsigned + PeripheralValue,
	{
		let adjusted_address = address & self.valid_address_mask;
		// Try to find a peripheral to write the value to
		for (mapping, peripheral) in self.address_map.iter_mut() {
			// If the peripheral's mapping contains this address
			if mapping.in_range(adjusted_address) {
				// Convert the address to a relative one
				let relative_address = mapping.relative(adjusted_address);
				// Write the data to the associated address in the peripheral
				data.write_to_be(peripheral.borrow_mut(), relative_address);
				break;
			}
		}
	}
}
