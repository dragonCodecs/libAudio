// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ffi::c_char;
use std::fs::File;
use std::os::fd::{FromRawFd, IntoRawFd};
use std::ptr::NonNull;

use crate::sndh::ice_decrunch::SndhDecruncher;

/// Allocate a new SndhDecruncher and "leak" that pointer into C++ land
#[unsafe(no_mangle)]
pub fn sndh_decruncher_new(unpacked_size: usize) -> *mut SndhDecruncher
{
	let decruncher = Box::new(SndhDecruncher::new(unpacked_size));
	Box::into_raw(decruncher)
}

/// Consume the pointer for a SndhDecruncher allocated with `sndh_decruncher_new`
/// and deallocate it cleanly
/// # Safety
/// As long as the pointer given is from `sndh_decruncher_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_decruncher_drop(decruncher: *mut SndhDecruncher)
{
	let decruncher = unsafe { Box::from_raw(decruncher) };
	drop(decruncher);
}

/// Using the pointer from `sndh_decruncher_new`, decrunch the contents of the file passed
/// # Safety
/// As long as the pointer given is from `sndh_decrunch_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_decruncher_depack(decruncher: *mut SndhDecruncher, fd: i32) -> bool
{
	let file = unsafe { File::from_raw_fd(fd) };
	let mut decruncher = unsafe { NonNull::new_unchecked(decruncher) };
	let result = unsafe { decruncher.as_mut() }.depack(&file);
	let _fd = file.into_raw_fd();
	if let Err(err) = result {
		console::error(format!("{err}"));
		false
	} else {
		true
	}
}

/// Using the pointer from `sndh_decruncher_new`, get a pointer to the data buffer containing
/// the decrunched SNDH file data
/// # Safety
/// As long as the pointer given is from `sndh_decrunch_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_decruncher_data(decruncher: *mut SndhDecruncher) -> *mut u8
{
	let mut decruncher = unsafe { NonNull::new_unchecked(decruncher) };
	unsafe { decruncher.as_mut() }.data_mut().as_mut_ptr()
}

/// Using the pointer from `sndh_decruncher_new`, get the length of the data buffer containing
/// the decrunched SNDH file data
/// # Safety
/// As long as the pointer given is from `sndh_decrunch_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_decruncher_length(decruncher: *mut SndhDecruncher) -> usize
{
	let decruncher = unsafe { NonNull::new_unchecked(decruncher) };
	unsafe { decruncher.as_ref() }.data().len()
}

mod console
{
	use std::ffi::CString;

	use super::console_error;

	pub(super) fn error(message: String)
	{
		let message = unsafe { CString::from_vec_unchecked(message.into_bytes()) };
		unsafe { console_error(message.into_raw()) };
	}
}

unsafe extern "C" {
	fn console_error(message: *const c_char);
}
