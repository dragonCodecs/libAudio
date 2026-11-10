// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ffi::c_char;
use std::fs::File;
use std::os::fd::{FromRawFd, IntoRawFd};
use std::ptr::{NonNull, null_mut};

use crate::sndh::ice_decrunch::SndhDecruncher;
use crate::sndh::loader::{SndhEntryPoints, SndhLoader};

/// Allocate a new [`SndhDecruncher`] and "leak" that pointer into C++ land
#[unsafe(no_mangle)]
pub fn sndh_decruncher_new(fd: i32) -> *mut SndhDecruncher
{
	let mut file = unsafe { File::from_raw_fd(fd) };
	let decruncher = SndhDecruncher::new(&mut file);
	let _fd = file.into_raw_fd();
	match decruncher {
		Ok(decruncher) => {
			let decruncher = Box::new(decruncher);
			Box::into_raw(decruncher)
		},
		Err(error) => {
			console::error(format!("{error}"));
			null_mut()
		},
	}
}

/// Consume the pointer for a [`SndhDecruncher`] allocated with `sndh_decruncher_new`
/// and deallocate it cleanly
/// # Safety
/// As long as the pointer given is from `sndh_decruncher_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_decruncher_drop(decruncher: *mut SndhDecruncher)
{
	let decruncher = unsafe { Box::from_raw(decruncher) };
	drop(decruncher);
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

/// Using the pointer from `sndh_decruncher_new`, get a pointer to the read offset in the decrunched SNDH file data
/// # Safety
/// As long as the pointer given is from `sndh_decrunch_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_decruncher_offset(decruncher: *mut SndhDecruncher) -> *const usize
{
	let decruncher = unsafe { NonNull::new_unchecked(decruncher) };
	unsafe { decruncher.as_ref().offset() as *const usize }
}

/// Using the pointer from `sndh_decruncher_new`, read the entry points from the SNDH data into the [`SndhEntryPoints`]
/// struct
/// # Safety
/// As long as the pointer given is from `sndh_decrunch_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_loader_entry_points(decruncher: *mut SndhDecruncher, entry_points: *mut SndhEntryPoints) -> bool
{
	let mut decruncher = unsafe { NonNull::new_unchecked(decruncher) };
	let mut entry_points = unsafe { NonNull::new_unchecked(entry_points) };
	match SndhEntryPoints::try_from(unsafe { decruncher.as_mut() }) {
		Ok(result) => {
			*unsafe { entry_points.as_mut() } = result;
			true
		},
		Err(error) => {
			console::error(format!("{error}"));
			false
		},
	}
}

/// Allocate a new [`SndhLoader`] and "leak" that pointer into C++ land
#[unsafe(no_mangle)]
pub fn sndh_loader_new(fd: i32) -> *mut SndhLoader
{
	let mut file = unsafe { File::from_raw_fd(fd) };
	let loader = SndhLoader::new(&mut file);
	let _fd = file.into_raw_fd();
	match loader {
		Ok(loader) => {
			let loader = Box::new(loader);
			Box::into_raw(loader)
		},
		Err(error) => {
			console::error(format!("{error}"));
			null_mut()
		},
	}
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
