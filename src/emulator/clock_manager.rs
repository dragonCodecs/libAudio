// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

#[derive(Default, Clone, Copy)]
pub struct ClockManager
{
	/// The base clock frequency the manager will be called at
	base_frequency: u32,
	/// The target (slower) clock frequency the manager will emit cycles at
	target_frequency: u32,
	/// Tracking for the current position within the target frequency clock cycle
	cycle_counter: u32,
}

impl ClockManager
{
	/// Construct a valid manager from the base and target clock frequencies
	pub fn new(base_frequency: u32, target_frequency: u32) -> Self
	{
		Self {
			base_frequency,
			target_frequency,
			cycle_counter: 0,
		}
	}

	/// Returns true if the clock being managed by this should advance a cycle, false otherwise
	pub fn advance_cycle(&mut self) -> bool
	{
		// Advance the time since the last managed cycle
		self.cycle_counter += self.target_frequency;

		// Check if enough base clock cycles have happened
		if self.base_frequency - self.cycle_counter < self.target_frequency {
			// Issue a cycle and take into account the ratio between the frequencies
			self.cycle_counter -= self.base_frequency;
			true
		} else {
			false
		}
	}
}
