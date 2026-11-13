// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use super::ClockManager;

#[test]
fn whole_ratio()
{
	let mut manager = ClockManager::new(32_000_000, 2_000_000);
	// Run a good few cycles
	for _ in 0..32 {
		// 32:2 gives a ratio of 16
		for cycle in 0..16 {
			// Expect advance_cycle to return true only on the 16th cycle
			let last_cycle = cycle == 15;
			assert_eq!(manager.advance_cycle(), last_cycle);
		}
	}
}

#[test]
fn fractional_ratio()
{
	let mut manager = ClockManager::new(32_000_000, 2_457_600);
	// Run a good few cycles
	for iter in 0..64 {
		// The 47th cycle should be the correction cycle
		let correction = iter == 47;
		// 32:2.4576 gives a ratio of 13.020833333
		let cycles = 13 + correction as usize;
		for cycle in 0..cycles {
			// Expect advance_cycle to return true only on the last cycle of the loop
			let last_cycle = cycle == cycles - 1;
			assert_eq!(manager.advance_cycle(), last_cycle);
		}
	}
}
