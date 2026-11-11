// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

#[allow(unused)]
pub struct Ym2149
{
	selected_register: u8,
	cycles_till_update: u8,

	channels: [Ym2149Channel; 3],
	channel_state: [bool; 3],
	noise_period: u8,
	noise_counter: u8,
	// Default to no output from any channel of any kind
	mixer_config: u8,
	envelope_period: u16,
	envelope_counter: u16,
	envelope_shape: u8,
	envelope_position: u8,
	io_port: [u8; 2],

	noise_state: bool,
	noise_lfsr: u32,
}

#[derive(Default, Clone, Copy)]
pub struct Ym2149Channel
{
	period: u16,
	counter: u16,
	edge_state: bool,
	pub level: u8,
}

impl Ym2149
{
	pub fn new() -> Self
	{
		Self {
			selected_register: 0,
			cycles_till_update: 0,
			channels: [Ym2149Channel::default(); 3],
			channel_state: [false; 3],
			noise_period: 0,
			noise_counter: 0,
			mixer_config: 0x3f,
			envelope_period: 0,
			envelope_counter: 0,
			envelope_shape: 0,
			envelope_position: 64,
			io_port: [0; 2],
			noise_state: false,
			noise_lfsr: 1,
		}
	}

	pub fn channel_mut(&mut self, index: usize) -> &mut Ym2149Channel
	{
		&mut self.channels[index]
	}
}

impl Ym2149Channel
{
	pub fn reset_edge_state(&mut self, state: bool)
	{
		self.edge_state = state;
	}

	pub fn write_frequency(&mut self, value: u8, rough_adjust: bool)
	{
		if rough_adjust {
			self.period &= 0x00ff;
			// Only 4 bits are valid, throw away the upper 4
			self.period |= ((value & 0x0f) as u16) << 8;
		} else {
			self.period &= 0xff00;
			self.period |= value as u16;
		}
	}

	pub fn read_frequency(&self, rough_adjust: bool) -> u8
	{
		if rough_adjust {
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

	pub fn force_edge_state(&mut self, state: bool)
	{
		self.edge_state = state;
	}
}
