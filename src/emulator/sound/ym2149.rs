// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use rand::RngExt;
use rand::rngs::StdRng;

use crate::emulator::clock_manager::ClockManager;
use crate::emulator::{ClockedPeripheral, Peripheral};

pub struct Ym2149
{
	clock_frequency: u32,

	selected_register: u8,
	cycles_till_update: u8,

	channels: [Channel; 3],
	channel_state: [bool; 3],
	noise_period: u8,
	noise_counter: u8,
	mixer_config: u8,
	envelope_period: u16,
	envelope_counter: u16,
	envelope_shape: u8,
	envelope_position: u8,
	io_port: [u8; 2],

	noise_state: bool,
	noise_lfsr: u32,

	clock_manager: ClockManager,
	ready: bool,
	read: bool,

	// Have enough space for 2048 samples of adjustment history
	dc_adjustment_buffer: [u16; 1 << DC_ADJUSTMENT_LENGTH_LOG2],
	dc_adjustment_position: usize,
	dc_adjustment_sum: u32,
}

#[derive(Default, Clone, Copy)]
struct Channel
{
	period: u16,
	counter: u16,
	edge_state: bool,
	pub level: u8,
}

const DC_ADJUSTMENT_LENGTH_LOG2: usize = 11;

#[rustfmt::skip]
const LOG_LEVEL: [u16; 32] = [
	0x0032, 0x003c, 0x0047, 0x0055, 0x0065, 0x0078, 0x008f, 0x00aa,
	0x00ca, 0x00f1, 0x011f, 0x0155, 0x0195, 0x01e2, 0x023e, 0x02aa,
	0x032b, 0x03c5, 0x047c, 0x0555, 0x0657, 0x078a, 0x08f8, 0x0aaa,
	0x0caf, 0x0f15, 0x11f0, 0x1555, 0x195e, 0x1e2b, 0x23e0, 0x2aaa,
];

impl Ym2149
{
	pub fn new(clock_frequency: u32, sample_frequency: u32) -> Self
	{
		let mut this = Self {
			clock_frequency,

			selected_register: 0,
			cycles_till_update: 0,

			channels: [Channel::default(); 3],
			channel_state: [false; 3],
			noise_period: 0,
			noise_counter: 0,
			// Default to no output from any channel of any kind
			mixer_config: 0x3f,
			envelope_period: 0,
			envelope_counter: 0,
			envelope_shape: 0,
			envelope_position: 64,
			io_port: [0; 2],

			noise_state: false,
			noise_lfsr: 1,

			clock_manager: ClockManager::new(clock_frequency, sample_frequency),
			ready: false,
			read: false,

			dc_adjustment_buffer: [0; 1 << DC_ADJUSTMENT_LENGTH_LOG2],
			dc_adjustment_position: 0,
			dc_adjustment_sum: 0,
		};
		// Set up random start states for each channel, just like happens in the real YM2149
		let mut rng: StdRng = rand::make_rng();
		for channel in this.channels.iter_mut() {
			channel.reset_edge_state(rng.random_bool(0.5));
		}
		this
	}

	fn update_fsm(&mut self)
	{
		// Update the channel states to reflect the current chip state
		for (index, channel) in self.channels.iter().enumerate() {
			// Check to see the state for tone generation
			let channel_state = channel.state(((self.mixer_config >> index) & 1) != 0);
			// Followed by noise generation for the channel
			let noise_state = self.noise_state || ((self.mixer_config >> (index + 3)) & 1) != 0;
			// Combine it all together and update the channel state
			self.channel_state[index] |= channel_state && noise_state;
		}

		// Step all the channels forward one internal cycle
		for channel in self.channels.iter_mut() {
			channel.step();
		}

		// Step the envelope generator forward one internal cycle
		// If the envelope counter exceeds the period of the envelope
		self.envelope_counter += 1;
		if self.envelope_counter >= self.envelope_period {
			// Reset the counter
			self.envelope_counter = 0;
			// Update the envelope position (128 possible positions, the last 64 loop)
			self.envelope_position += 1;
			if self.envelope_position > 64 {
				self.envelope_position = (self.envelope_position & 0x3f) | 0x40;
			}
		}

		// Step the noise generator forward one internal cycle, noting it runs
		// at half speed to everything else. If the noise counter exceeds the
		// period of the noise to be generated
		self.noise_counter += 1;
		if self.noise_counter >= self.noise_period {
			// Reset the counter
			self.noise_counter = 0;
			// Set the noise state if the 0th and 2nd bits of the noise LFSR are not the same value as each other
			self.noise_state = ((self.noise_lfsr ^ (self.noise_lfsr >> 2)) & 1) != 0;
			// Shift the new noise state in as single bit on the LHS of the noise LFSR
			self.noise_lfsr >>= 1;
			self.noise_lfsr |= if self.noise_state {
				1 << 16
			} else {
				0
			};
		}
	}

	pub fn sample_ready(&self) -> bool
	{
		self.ready && !self.read
	}

	pub fn sample(&mut self) -> i16
	{
		self.read = true;
		// Grab the current envelope level
		let envelope_level = self.compute_envelope_level();

		let mut levels = [0; 3];
		// Go through each channel and compute the volume level associated with it
		for (channel, &state) in self.channel_state.iter().enumerate() {
			let level = {
				if state {
					// Extract the level information from the channel
					let raw_level = self.channels[channel].level;
					// If it indicates the level is controlled by the envelope, use that
					if (raw_level & 0x10) != 0 {
						envelope_level
					}
					// Otherwise, use the raw level for the channel, scaled appropriately
					else {
						raw_level << 1
					}
				} else {
					0
				}
			};
			// Check to see what shift should be applied (1 if there is one needed, 0 otherwise)
			let shift = self.channels[channel].shift_required() as usize;
			// Compute and store the final level using the logarithm table for the chip
			levels[channel] = LOG_LEVEL[level as usize] >> shift;
		}
		// Compute and store the final level using the logarithm table for the chip
		self.dc_adjust(levels.into_iter().sum())
	}

	fn compute_envelope_level(&self) -> u8
	{
		// Extract out the actual position and phase data (32 unique positions, 4 phases)
		let position = self.envelope_position & 0x1f;
		let phase = self.envelope_position >> 5;
		match phase {
			// Initial envelope phase, behaviour determined by attack bit
			0 => {
				// If the attack bit is set, the envelope level is the position value, otherwise
				// the ramp runs backwards, so invert the position to make the level.
				if (self.envelope_shape & 0x4) != 0 {
					position
				} else {
					0x1f - position
				}
			},
			// The 2nd and 4th phase is determined identically by the rest of the shape bits
			1 | 3 => {
				// If this is not a continuous envelope, the level is low permanently
				if (self.envelope_shape & 0x8) == 0 {
					0
				}
				// Otherwise, if the envelope should hold its value
				else if (self.envelope_shape & 0x1) != 0 {
					// Low or high determined by attack and alternate bits xor'd together
					let state = (self.envelope_shape & 0x2) ^ ((self.envelope_shape & 0x4) >> 1);
					if state != 0 {
						0x1f
					} else {
						0
					}
				}
				// If the alternate bit is set, then ramp the opposite direction to before
				else if (self.envelope_shape & 0x2) != 0 {
					if (self.envelope_shape & 0x4) != 0 {
						0x1f - position
					} else {
						position
					}
				}
				// Otherwise, ramp the same as before
				else if (self.envelope_shape & 0x4) != 0 {
					position
				} else {
					0x1f - position
				}
			},
			// The 3rd phase is also determined by the rest of the shape bits, but has the opposite
			// logic for the alternate-controlled ramping when alternate set
			2 => {
				// If this is not a continuous envelope, the level is low permanently
				if (self.envelope_shape & 0x8) == 0 {
					0
				}
				// Otherwise, if the envelope should hold its value
				else if (self.envelope_shape & 0x1) != 0 {
					// Low or high determined by attack and alternate bits xor'd together
					let state = (self.envelope_shape & 0x2) ^ ((self.envelope_shape & 0x4) >> 1);
					if state != 0 {
						0x1f
					} else {
						0
					}
				}
				// Ramp in the original direction regardless of alternate bit
				else if (self.envelope_shape & 0x4) != 0 {
					position
				} else {
					0x1f - position
				}
			},
			_ => unreachable!(),
		}
	}

	fn dc_adjust(&mut self, sample: u16) -> i16
	{
		// First adjust the DC ballancing adjustment sum
		self.dc_adjustment_sum -= self.dc_adjustment_buffer[self.dc_adjustment_position] as u32;
		self.dc_adjustment_sum += sample as u32;
		// Then store this sample in the buffer
		self.dc_adjustment_buffer[self.dc_adjustment_position] = sample;
		// Update the adjustment position to make this a circular buffer
		self.dc_adjustment_position = (self.dc_adjustment_position + 1) & (self.dc_adjustment_buffer.len() - 1);
		// Now scale the sum and apply it to the sample to get the final sample
		((sample as i32) - (self.dc_adjustment_sum >> DC_ADJUSTMENT_LENGTH_LOG2) as i32) as i16
	}

	#[cfg(test)]
	pub(self) fn force_channel_states(&mut self, edge_state: bool)
	{
		for channel in self.channels.iter_mut() {
			channel.reset_edge_state(edge_state);
		}
	}
}

impl Peripheral<u32> for Ym2149
{
	fn read_address(&self, address: u32, data: &mut [u8])
	{
		// Only admit 8- and 16-bit reads
		if data.len() > 2 {
			return;
		}
		// We only respond to reads on address 0
		if address != 0 {
			return;
		}
		// Where to read from depends on the register selection, so..
		match self.selected_register {
			// Current channel frequency rough/fine
			0..=5 => {
				// Turn the register selection into a channel number and rough adjustment indicator
				let channel = (self.selected_register >> 1) as usize;
				let rough_adjustment = (self.selected_register & 1) != 0;
				data[0] = self.channels[channel].read_frequency(rough_adjustment);
			},
			// Current noise frequency
			6 => data[0] = self.noise_period,
			// Current mixer configuration
			7 => data[0] = self.mixer_config,
			// Current channel level
			8..=10 => {
				// Turn the register selection into a channel number
				let channel = (self.selected_register - 8) as usize;
				data[0] = self.channels[channel].level;
			},
			// Envelope frequency fine adjustment
			11 => data[0] = (self.envelope_period & 0xff) as u8,
			// Envelope frequency rough adjustment
			12 => data[0] = (self.envelope_period >> 8) as u8,
			// Envelope shape adjustment
			13 => data[0] = self.envelope_shape,
			// I/O port data
			14 | 15 => {
				// Turn the register selection into a port number (repesenting A vs B)
				let port = (self.selected_register & 1) as usize;
				data[0] = self.io_port[port];
			},
			_ => unreachable!(),
		}
	}

	fn write_address(&mut self, address: u32, data: &[u8])
	{
		// 32-bit writes must be split into two 16-bit ones
		if data.len() == 4 {
			self.write_address(address, &data[0..2]);
			self.write_address(address + 2, &data[2..4]);
			return;
		}
		// The address to write determines if we're making a register selection or writing a register
		match address {
			// Register selection, only the bottom 4 bits of the selection are valid/used
			0 => self.selected_register = data[0] & 0x0f,
			// Register write - where we write depends on the register selection, so..
			2 => match self.selected_register {
				// Channel frequency rough/fine adjustment
				0..=5 => {
					// Turn the register selection into a channel number and rough adjustment indicator
					let channel = (self.selected_register >> 1) as usize;
					let rough_adjustment = (self.selected_register & 1) != 0;
					// Select the channel and write the adjustment
					self.channels[channel].write_frequency(data[0], rough_adjustment);
				},
				// Noise frequency adjustment
				6 => {
					// Register is actually only 5 bit, so discard the upper 3
					self.noise_period = data[0] & 0x1f
				},
				// Mixer configuration adjustment
				7 => self.mixer_config = data[0],
				// Channel level adjustment
				8..=10 => {
					// Turn the register selection into a channel number
					let channel = (self.selected_register - 8) as usize;
					// Adjust that channel's levels (only 5 bits valid, discard the upper 3)
					self.channels[channel].level = data[0] & 0x1f;
				},
				// Envelope frequency fine adjustment
				11 => {
					self.envelope_period &= 0xff00;
					self.envelope_period |= data[0] as u16;
				},
				// Envelope frequency rough adjustment
				12 => {
					self.envelope_period &= 0x00ff;
					self.envelope_period |= (data[0] as u16) << 8;
				},
				// Envelope shape adjustment
				13 => {
					self.envelope_shape = data[0] & 0x0f;
					// Reset where we are in the envelope as a result
					self.envelope_position = 0;
				},
				// I/O port data
				14 | 15 => {
					// Turn the register selection into a port number (repesenting A vs B)
					let port = (self.selected_register & 1) as usize;
					self.io_port[port] = data[0];
				},
				_ => unreachable!(),
			},
			// If it's not one of the known peripheral register addresses, ignore the write
			_ => {},
		}
	}
}

impl ClockedPeripheral<u32> for Ym2149
{
	fn clock_frequency(&self) -> u32
	{
		self.clock_frequency
	}

	fn clock_cycle(&mut self) -> bool
	{
		// Reset the channel states if ready was true
		if self.ready {
			self.read = false;
			for state in self.channel_state.iter_mut() {
				*state = false;
			}
		}

		// See if a sample should be ready this cycle or not
		self.ready = self.clock_manager.advance_cycle();

		// If we should update the FSM in this cycle, do so
		if self.cycles_till_update == 0 {
			self.update_fsm();
		}
		// The internal state machine runs at 1/8th the device clock speed
		// (presume that ~SEL is low, yielding `f(T) = (f(Master) / 2) / (16 * TP)`
		// which simplifies to `f(T) = f(Master) / (8 * TP)``)
		self.cycles_till_update = (self.cycles_till_update + 1) & 7;
		true
	}
}

impl Channel
{
	pub fn reset_edge_state(&mut self, state: bool)
	{
		self.edge_state = state;
	}

	pub fn write_frequency(&mut self, value: u8, rough_adjustment: bool)
	{
		if rough_adjustment {
			self.period &= 0x00ff;
			// Only 4 bits are valid, throw away the upper 4
			self.period |= ((value & 0x0f) as u16) << 8;
		} else {
			self.period &= 0xff00;
			self.period |= value as u16;
		}
	}

	pub fn read_frequency(&self, rough_adjustment: bool) -> u8
	{
		if rough_adjustment {
			(self.period >> 8) as u8
		} else {
			(self.period & 0x00ff) as u8
		}
	}

	pub fn step(&mut self)
	{
		// If our channel's period counter exceeds the period of the current tone frequency
		self.counter += 1;
		if self.counter >= self.period {
			// Reset the counter and update the edge states
			self.counter = 0;
			self.edge_state = !self.edge_state;
		}
	}

	pub fn state(&self, tone_inhibit: bool) -> bool
	{
		self.edge_state || tone_inhibit
	}

	pub fn shift_required(&self) -> bool
	{
		self.period <= 1
	}
}

#[cfg(test)]
mod tests
{
	include!("tests/ym2149.rs");
}
