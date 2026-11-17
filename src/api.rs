// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use core::slice;
use std::ffi::c_char;
use std::fs::File;
#[cfg(not(target_family = "windows"))]
use std::os::fd::{FromRawFd, IntoRawFd};
#[cfg(target_family = "windows")]
use std::os::windows::io::{FromRawHandle, IntoRawHandle, RawHandle};
use std::ptr::{NonNull, null_mut};

use crate::emulator::atari_ste::AtariSTe;
use crate::sndh::ice_decrunch::SndhDecruncher;
use crate::sndh::loader::{SndhLoader, SndhMetadata, SndhTimer};

#[repr(C)]
pub struct Slice<T>
{
	ptr: *const T,
	len: usize,
}

#[repr(C)]
pub struct MutSlice<T>
where
	T: Sized + Copy,
{
	ptr: *mut T,
	len: usize,
}

impl<T> Slice<T>
{
	pub fn from_slice(data: &[T]) -> Self
	{
		Self {
			ptr: data.as_ptr(),
			len: data.len(),
		}
	}

	pub fn as_slice(&self) -> &[T]
	{
		unsafe { slice::from_raw_parts(self.ptr, self.len) }
	}
}

impl<T> MutSlice<T>
where
	T: Sized + Copy,
{
	pub fn from_mut_slice(data: &mut [T]) -> Self
	{
		Self {
			ptr: data.as_mut_ptr(),
			len: data.len(),
		}
	}

	pub fn as_mut_slice(&mut self) -> &mut [T]
	{
		unsafe { slice::from_raw_parts_mut(self.ptr, self.len) }
	}
}

/// Allocate a new [`SndhLoader`] and "leak" that pointer into C++ land
#[cfg(not(target_family = "windows"))]
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

/// Allocate a new [`SndhLoader`] and "leak" that pointer into C++ land
#[cfg(target_family = "windows")]
#[unsafe(no_mangle)]
pub fn sndh_loader_new(fd: i32) -> *mut SndhLoader
{
	let handle = unsafe { _get_osfhandle(fd) as RawHandle };
	let mut file = unsafe { File::from_raw_handle(handle) };
	let loader = SndhLoader::new(&mut file);
	let _handle = file.into_raw_handle();
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
/// As long as the pointer given is from `sndh_loader_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_loader_decruncher(loader: *mut SndhLoader) -> *mut SndhDecruncher
{
	let mut loader = unsafe { NonNull::new_unchecked(loader) };
	unsafe { loader.as_mut() }.decruncher_mut() as *mut SndhDecruncher
}

/// Using the pointer from `sndh_loader_new`, extract the associated [`SndhMetadata`]
/// # Safety
/// As long as the pointer given is from `sndh_loader_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_loader_metadata(loader: *mut SndhLoader) -> *const SndhMetadata
{
	let loader = unsafe { NonNull::new_unchecked(loader) };
	unsafe { loader.as_ref() }.metadata() as *const SndhMetadata
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song title
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_title(metadata: *mut SndhMetadata) -> Slice<u8>
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	Slice::from_slice(unsafe { metadata.as_ref() }.title().as_bytes())
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song artist
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_artist(metadata: *mut SndhMetadata) -> Slice<u8>
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	Slice::from_slice(unsafe { metadata.as_ref() }.artist().as_bytes())
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song ripper
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_ripper(metadata: *mut SndhMetadata) -> Slice<u8>
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	Slice::from_slice(unsafe { metadata.as_ref() }.ripper().as_bytes())
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song converter
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_converter(metadata: *mut SndhMetadata) -> Slice<u8>
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	Slice::from_slice(unsafe { metadata.as_ref() }.converter().as_bytes())
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song tune count
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_tune_count(metadata: *mut SndhMetadata) -> u8
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	unsafe { metadata.as_ref() }.tune_count()
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song default tune
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_default_tune(metadata: *mut SndhMetadata) -> u8
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	unsafe { metadata.as_ref() }.default_tune()
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song timer frequency
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_timer_frequency(metadata: *mut SndhMetadata) -> u16
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	unsafe { metadata.as_ref() }.timer_frequency()
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song year
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_year(metadata: *mut SndhMetadata) -> u32
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	unsafe { metadata.as_ref() }.year()
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song timer
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_timer(metadata: *mut SndhMetadata) -> SndhTimer
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	unsafe { metadata.as_ref() }.timer()
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song tune times
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_tune_times(metadata: *mut SndhMetadata) -> Slice<u16>
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	Slice::from_slice(unsafe { metadata.as_ref() }.tune_times())
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song frame counts
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_tune_frame_counts(metadata: *mut SndhMetadata) -> Slice<u32>
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	Slice::from_slice(unsafe { metadata.as_ref() }.tune_frame_counts())
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song tune names count
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_tune_names_count(metadata: *mut SndhMetadata) -> usize
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	unsafe { metadata.as_ref() }.tune_names().len()
}

/// Using the pointer from `sndh_loader_metadata`, extract the associated song tune names count
/// # Safety
/// As long as the pointer given is from `sndh_loader_metadata`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_metadata_tune_name(metadata: *mut SndhMetadata, index: usize) -> Slice<u8>
{
	let metadata = unsafe { NonNull::new_unchecked(metadata) };
	Slice::from_slice(unsafe { metadata.as_ref() }.tune_names()[index].as_bytes())
}

/// Allocate a new [`AtariSTe`] and "leak" that pointer into C++ land
#[unsafe(no_mangle)]
pub fn atari_ste_new<'a>() -> *mut AtariSTe<'a>
{
	let atari_ste = Box::new(AtariSTe::new());
	Box::into_raw(atari_ste)
}

/// Consume the pointer for a [`AtariSTe`] allocated with `atari_ste_new`
/// and deallocate it cleanly
/// # Safety
/// As long as the pointer given is from `atari_ste_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn atari_ste_drop(atari_ste: *mut AtariSTe)
{
	let atari_ste = unsafe { Box::from_raw(atari_ste) };
	drop(atari_ste);
}

/// Using the pointer from `atari_ste_new`, configure a timer
/// # Safety
/// As long as the pointer given is from `atari_ste_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn atari_ste_configure_timer(atari_ste: *mut AtariSTe, timer: SndhTimer, timer_frequency: u16)
{
	let mut atari_ste = unsafe { NonNull::new_unchecked(atari_ste) };
	unsafe { atari_ste.as_mut() }.configure_timer(timer, timer_frequency);
}

/// Using the pointer from `atari_ste_new`, copy SNDH data from the decruncher to the STe's RAM
/// # Safety
/// As long as the pointer given is from `atari_ste_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn atari_ste_copy_to_ram(atari_ste: *mut AtariSTe, decruncher: *mut SndhDecruncher) -> bool
{
	let mut atari_ste = unsafe { NonNull::new_unchecked(atari_ste) };
	let mut decruncher = unsafe { NonNull::new_unchecked(decruncher) };
	match unsafe { atari_ste.as_mut() }.copy_to_ram(unsafe { decruncher.as_mut() }) {
		Ok(()) => true,
		Err(error) => {
			console::error(format!("{error}"));
			false
		},
	}
}

/// Using the pointer from `atari_ste_new`, init a SNDH's replayer
/// # Safety
/// As long as the pointer given is from `atari_ste_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn atari_ste_init(atari_ste: *mut AtariSTe, subtune: u16) -> bool
{
	let mut atari_ste = unsafe { NonNull::new_unchecked(atari_ste) };
	unsafe { atari_ste.as_mut() }.init(subtune)
}

/// Using the pointer from `atari_ste_new`, exit a SNDH's replayer
/// # Safety
/// As long as the pointer given is from `atari_ste_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn atari_ste_exit(atari_ste: *mut AtariSTe) -> bool
{
	let mut atari_ste = unsafe { NonNull::new_unchecked(atari_ste) };
	unsafe { atari_ste.as_mut() }.exit()
}

/// Using the pointer from `atari_ste_new`, advance the machine clock
/// # Safety
/// As long as the pointer given is from `atari_ste_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn atari_ste_advance_clock(atari_ste: *mut AtariSTe) -> bool
{
	let mut atari_ste = unsafe { NonNull::new_unchecked(atari_ste) };
	unsafe { atari_ste.as_mut() }.advance_clock()
}

/// Using the pointer from `atari_ste_new`, check if a new sample is ready
/// # Safety
/// As long as the pointer given is from `atari_ste_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn atari_ste_sample_ready(atari_ste: *mut AtariSTe) -> bool
{
	let atari_ste = unsafe { NonNull::new_unchecked(atari_ste) };
	unsafe { atari_ste.as_ref() }.sample_ready()
}

/// Using the pointer from `atari_ste_new`, cread a sample from the machine
/// # Safety
/// As long as the pointer given is from `atari_ste_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn atari_ste_read_sample(atari_ste: *mut AtariSTe) -> i16
{
	let mut atari_ste = unsafe { NonNull::new_unchecked(atari_ste) };
	unsafe { atari_ste.as_mut() }.read_sample()
}

/// Using the pointer from `atari_ste_new`, display the machine's CPU state
/// # Safety
/// As long as the pointer given is from `atari_ste_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn atari_ste_display_cpu_state(atari_ste: *mut AtariSTe)
{
	let atari_ste = unsafe { NonNull::new_unchecked(atari_ste) };
	unsafe { atari_ste.as_ref() }.display_cpu_state();
}

pub(crate) mod console
{
	use std::ffi::CString;

	use super::{console_debug, console_error};

	pub fn error(message: String)
	{
		let message = unsafe { CString::from_vec_unchecked(message.into_bytes()) };
		unsafe { console_error(message.into_raw()) };
	}

	pub fn debug(message: String)
	{
		let message = unsafe { CString::from_vec_unchecked(message.into_bytes()) };
		unsafe { console_debug(message.into_raw()) };
	}
}

unsafe extern "C" {
	fn console_error(message: *const c_char);
	fn console_debug(message: *const c_char);
}

#[cfg(target_family = "windows")]
unsafe extern "C" {
	fn _get_osfhandle(fd: i32) -> isize;
}
