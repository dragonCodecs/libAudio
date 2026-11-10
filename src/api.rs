// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::ffi::c_char;
use std::fs::File;
use std::os::fd::{FromRawFd, IntoRawFd};
use std::ptr::{NonNull, null_mut};

use crate::sndh::ice_decrunch::SndhDecruncher;
use crate::sndh::loader::{SndhEntryPoints, SndhLoader};

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

/// Consume the pointer for a [`SndhLoader`] allocated with `sndh_loader_new`
/// and deallocate it cleanly
/// # Safety
/// As long as the pointer given is from `sndh_loader_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_loader_drop(loader: *mut SndhLoader)
{
	let loader = unsafe { Box::from_raw(loader) };
	drop(loader);
}

/// Using the pointer from `sndh_loader_new`, extract the associated [`SndhDecruncher`]
/// # Safety
/// As long as the pointer given is from `SndhDecruncher`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_loader_decruncher(loader: *mut SndhLoader) -> *mut SndhDecruncher
{
	let mut loader = unsafe { NonNull::new_unchecked(loader) };
	unsafe { loader.as_mut() }.decruncher_mut() as *mut SndhDecruncher
}

/// Using the pointer from `sndh_loader_new`, extract the associated [`SndhEntryPoints`]
/// # Safety
/// As long as the pointer given is from `SndhDecruncher`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_loader_entry_points(loader: *mut SndhLoader) -> *const SndhEntryPoints
{
	let loader = unsafe { NonNull::new_unchecked(loader) };
	unsafe { loader.as_ref() }.entry_points() as *const SndhEntryPoints
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
