// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

#[test]
fn prescaling_mapping()
{
	assert_eq!(super::Timer::prescaling_for(0), 1);
	assert_eq!(super::Timer::prescaling_for(1), 4);
	assert_eq!(super::Timer::prescaling_for(2), 10);
	assert_eq!(super::Timer::prescaling_for(3), 16);
	assert_eq!(super::Timer::prescaling_for(4), 50);
	assert_eq!(super::Timer::prescaling_for(5), 64);
	assert_eq!(super::Timer::prescaling_for(6), 100);
	assert_eq!(super::Timer::prescaling_for(7), 200);
}
