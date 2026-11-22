// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ffi::{CString, c_char};
use std::fmt::{Arguments, format};
pub use std::format_args;

// Define macros for each of the levels of message that we can display
macro_rules! error {
	($($arg:tt)+) => {
		$crate::console::_error($crate::console::format_args!($($arg)+))
	};
}

macro_rules! debug {
	($($arg:tt)+) => {
		$crate::console::_debug($crate::console::format_args!($($arg)+))
	};
}

// Make those macros visible to the claling contexts
pub(crate) use debug;
pub(crate) use error;

use crate::api::Slice;

/// Converts a [`format_args!`] formatting arguments into a [`String`] and then into a [`CString`]
fn to_message(args: Arguments) -> CString
{
	let message = format(args);
	CString::new(message).expect("Formatted message does not contain internal NUL bytes")
}

#[cfg(not(test))]
pub fn _error(args: Arguments)
{
	let message = to_message(args);
	unsafe { console_error(Slice::from_slice(message.as_bytes())) };
}

#[cfg(not(test))]
pub fn _debug(args: Arguments)
{
	let message = to_message(args);
	unsafe { console_debug(Slice::from_slice(message.as_bytes())) };
}

#[cfg(test)]
pub fn _error(args: Arguments)
{
	println!("{args}");
}

#[cfg(test)]
pub fn _debug(args: Arguments)
{
	println!("{args}");
}

unsafe extern "C" {
	fn console_error(message: Slice<u8>);
	fn console_debug(message: Slice<u8>);
}
