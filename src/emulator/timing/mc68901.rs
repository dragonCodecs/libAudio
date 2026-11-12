// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ffi::c_void;

use crate::emulator::Peripheral;
use crate::emulator::clock_manager::ClockManager;

pub struct MC68901
{
	this: *const c_void,

	gpio: u8,
	active_edge: u8,
	data_direction: u8,
	itr_enable: u16,
	itr_pending: u16,
	itr_servicing: u16,
	itr_mask: u16,
	vector_reg: u8,
	sync_char: u8,
	usart_ctrl: u8,
	rx_status: u8,
	tx_status: u8,
	usart_data: u8,

	timers: [Timer; 4],
}

#[derive(Clone, Copy)]
pub struct Timer
{
	control: u8,
	counter: u8,
	reload_value: u8,
	clock_manager: ClockManager,
	external_event: bool,
}

impl MC68901
{
	pub fn new(this: *const c_void, clock_frequency: u32) -> Self
	{
		Self {
			this,

			gpio: 0,
			active_edge: 0,
			data_direction: 0,
			// Timer C interrupts should be enabled and unmasked by default
			itr_enable: 1 << 5,
			itr_pending: 0,
			itr_servicing: 0,
			itr_mask: 1 << 5,
			vector_reg: 0,
			sync_char: 0,
			usart_ctrl: 0,
			rx_status: 0,
			tx_status: 0,
			usart_data: 0,

			timers: [Timer::new(clock_frequency); 4],
		}
	}
}

impl Peripheral for MC68901
{
	fn read_address(&self, address: u32, data: &mut [u8])
	{
		// Extract how wide an access this is
		let access_width = data.len();
		// Only admit 8- and 16-bit reads
		if access_width > 2 {
			return;
		}
		// Adjust the address to take into account u16 vs u8 accesses to odd bytes
		let adjusted_address = if access_width == 1 {
			address
		} else {
			address + 1
		};
		// If the resulting address is not odd, we're done
		if (adjusted_address & 1) == 0 {
			return;
		}
		// Extract out the byte to read into
		let value = if access_width == 1 {
			&mut data[0]
		} else {
			&mut data[1]
		};
		// Chop off the now unused bit of the address to make selection easier
		match adjusted_address >> 1 {
			0 => *value = self.gpio,
			1 => *value = self.active_edge,
			2 => *value = self.data_direction,
			// itrEnableA is the upper half of the enable register
			3 => *value = (self.itr_enable >> 8) as u8,
			// itrEnableB is the lower half of the enable register
			4 => *value = (self.itr_enable & 0xff) as u8,
			// itrPendingA is the upper half of the pending register
			5 => *value = (self.itr_pending >> 8) as u8,
			// itrPendingB is the lower half of the pending register
			6 => *value = (self.itr_pending & 0xff) as u8,
			// itrServicingA is the upper half of the servicing register
			7 => *value = (self.itr_servicing >> 8) as u8,
			// itrServicingB is the lower half of the servicing register
			8 => *value = (self.itr_servicing & 0xff) as u8,
			// itrMaskA is the upper half of the mask register
			9 => *value = (self.itr_mask >> 8) as u8,
			// itrMaskB is the lower half of the mask register
			10 => *value = (self.itr_mask & 0xff) as u8,
			11 => *value = self.vector_reg,
			12 => *value = self.timers[0].read_ctrl(),
			13 => *value = self.timers[1].read_ctrl(),
			// Read is from 2 timers, C's control value is in the upper nibble,
			// and D's is in the lower - neither can have more than 3 bits set
			// so just combine
			14 => *value = (self.timers[2].read_ctrl() << 4) | self.timers[3].read_ctrl(),
			// Extract the given timer's counter value
			15..=18 => *value = self.timers[(adjusted_address >> 1) as usize - 15].read_data(),
			19 => *value = self.sync_char,
			20 => *value = self.usart_ctrl,
			21 => *value = self.rx_status,
			22 => *value = self.tx_status,
			23 => *value = self.usart_data,
			_ => {},
		}
	}

	fn write_address(&mut self, address: u32, data: &[u8])
	{
		// Extract how wide an access this is
		let access_width = data.len();
		// Only admit 8- and 16-bit reads
		if access_width > 2 {
			return;
		}
		// Adjust the address to take into account u16 vs u8 accesses to odd bytes
		let adjusted_address = if access_width == 1 {
			address
		} else {
			address + 1
		};
		// If the resulting address is not odd, we're done
		if (adjusted_address & 1) == 0 {
			return;
		}
		// Extract out the byte from the write to use
		let value = if access_width == 1 {
			data[0]
		} else {
			data[1]
		};
		// Chop off the now unused bit of the address to make selection easier
		match adjusted_address >> 1 {
			0 => self.gpio = value,
			1 => self.active_edge = value,
			2 => self.data_direction = value,
			3 => {
				// itrEnableA is the upper half of the enable register
				self.itr_enable &= 0x00ff;
				self.itr_enable |= (value as u16) << 8;
			},
			4 => {
				// itrEnableB is the lower half of the enable register
				self.itr_enable &= 0xff00;
				self.itr_enable |= value as u16;
			},
			5 => {
				// itrPendingA is the upper half of the pending register
				self.itr_pending &= 0x00ff;
				self.itr_pending |= (value as u16) << 8;
			},
			6 => {
				// itrPendingB is the lower half of the pending register
				self.itr_pending &= 0xff00;
				self.itr_pending |= value as u16;
			},
			7 => {
				// itrServicingA is the upper half of the servicing register
				self.itr_servicing &= 0x00ff;
				self.itr_servicing |= (value as u16) << 8;
			},
			8 => {
				// itrServicingB is the lower half of the servicing register
				self.itr_servicing &= 0xff00;
				self.itr_servicing |= value as u16;
			},
			9 => {
				// itrMaskA is the upper half of the mask register
				self.itr_mask &= 0x00ff;
				self.itr_mask |= (value as u16) << 8;
			},
			10 => {
				// itrMaskB is the lower half of the mask register
				self.itr_mask &= 0xff00;
				self.itr_mask |= value as u16;
			},
			// The bottom 3 bits of the vector register are unused (RAZ)
			11 => self.vector_reg = value & 0xf8,
			// Only 5 bits used, mask off the upper 3
			12 => self.timers[0].write_ctrl(value & 0x1f, unsafe { mc68901_clock_frequency(self.this) }),
			// Only 5 bits used, mask off the upper 3
			13 => self.timers[1].write_ctrl(value & 0x1f, unsafe { mc68901_clock_frequency(self.this) }),
			14 => {
				// Write controls 2 timers, C's control value is in the upper nibble,
				// but the timer doesn't support event modes or reset output, so only 3 bits used
				self.timers[2].write_ctrl((value >> 4) & 0x07, unsafe { mc68901_clock_frequency(self.this) });
				// D's control value is in the lower nibble and has the same support restrictions
				self.timers[3].write_ctrl(value & 0x07, unsafe { mc68901_clock_frequency(self.this) });
			},
			// Update the given timer's counter value
			15..=18 => self.timers[(adjusted_address >> 1) as usize - 15].write_data(value),
			19 => self.sync_char = value,
			20 => self.usart_ctrl = value,
			21 => self.rx_status = value,
			22 => self.tx_status = value,
			23 => self.usart_data = value,
			_ => {},
		}
	}
}

impl Timer
{
	pub fn new(base_frequency: u32) -> Self
	{
		Self {
			control: 0,
			counter: 0,
			reload_value: 0,
			clock_manager: ClockManager::new(base_frequency, base_frequency),
			external_event: false,
		}
	}

	pub fn read_ctrl(&self) -> u8
	{
		self.control
	}

	pub fn read_data(&self) -> u8
	{
		self.counter
	}

	pub fn write_ctrl(&mut self, value: u8, base_frequency: u32)
	{
		self.control = value;
		// Turn the operation mode into a prescaling value
		let prescale = Self::prescaling_for(value & 0x07);
		// Convert that into a clock ratio for the clock manager, and reinitialise the
		// clock manager accordingly so we get the right generated frequency
		self.clock_manager = ClockManager::new(base_frequency, base_frequency / prescale);
	}

	pub fn write_data(&mut self, value: u8)
	{
		// Always store the new value as the reload value
		self.reload_value = value;
		// If the counter is stopped, also store it as the new counter value
		if (self.control & 0x0f) == 0 {
			self.counter = value;
		}
	}

	pub fn mark_external_event(&mut self)
	{
		if (self.control & 0x0f) == 0x08 {
			self.external_event = true;
		}
	}

	fn count(&mut self) -> bool
	{
		// Apply a clock pulse to the counter, and if that counter is now 0, reload it to the value in reloadValue
		self.counter -= 1;
		if self.counter == 0 {
			self.counter = self.reload_value;
			// Signal that this was an interrupt generating cycle
			true
		} else {
			// Signal that this was not an interrupt generating cycle
			false
		}
	}

	pub fn clock_cycle(&mut self) -> bool
	{
		// Check if the timer is stopped
		if (self.control & 0x0f) == 0 ||
			// If it is not, check if this is a clock advancement cycle
			!self.clock_manager.advance_cycle()
		{
			false
		}
		// Check if the timer is in a counting mode
		else if (self.control & 0x08) == 0 {
			self.count()
		}
		// Check if the timer is in event count mode
		else if (self.control & 0x0f) == 0x08 {
			// If there is a pending external event
			if !self.external_event {
				return false;
			}
			// Clear the event marker
			self.external_event = false;
			self.count()
		} else {
			// For now we don't support the PWM modes
			// Signal that this was not an interrupt generating cycle
			false
		}
	}

	pub fn prescaling_for(mode: u8) -> u32
	{
		match mode {
			0 => 1,
			1 => 4,
			2 => 10,
			3 => 16,
			4 => 50,
			5 => 64,
			6 => 100,
			7 => 200,
			_ => unreachable!(),
		}
	}
}

unsafe extern "C" {
	fn mc68901_clock_frequency(mc68901: *const c_void) -> u32;
}

#[cfg(test)]
mod tests;
