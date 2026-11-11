// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use core::slice;
use std::ffi::c_char;
use std::fs::File;
use std::os::fd::{FromRawFd, IntoRawFd};
use std::ptr::{NonNull, null_mut};

use crate::emulator::sound::ym2149::Ym2149Channel;
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
}

impl<T> MutSlice<T>
where
	T: Sized + Copy,
{
	pub fn as_mut_slice(&mut self) -> &mut [T]
	{
		unsafe { slice::from_raw_parts_mut(self.ptr, self.len) }
	}
}

/// Using the pointer from `sndh_loader_decruncher`, get the length of the data buffer containing
/// the decrunched SNDH file data
/// # Safety
/// As long as the pointer given is from `sndh_loader_decruncher`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_decruncher_length(decruncher: *mut SndhDecruncher) -> usize
{
	let decruncher = unsafe { NonNull::new_unchecked(decruncher) };
	unsafe { decruncher.as_ref() }.length()
}

/// Using the pointer from `sndh_loader_decruncher`, seek the read position back to the start of the
/// decrunched SNDH file data
/// # Safety
/// As long as the pointer given is from `sndh_loader_decruncher`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_decruncher_seek_start(decruncher: *mut SndhDecruncher)
{
	let mut decruncher = unsafe { NonNull::new_unchecked(decruncher) };
	unsafe { decruncher.as_mut() }.seek_start()
}

/// Using the pointer from `sndh_loader_decruncher`, read a span of data from the SNDH file data
/// # Safety
/// As long as the pointer given is from `sndh_loader_decruncher`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn sndh_decruncher_read(decruncher: *mut SndhDecruncher, mut slice: MutSlice<u8>) -> bool
{
	let mut decruncher = unsafe { NonNull::new_unchecked(decruncher) };
	match unsafe { decruncher.as_mut() }.read_slice(slice.as_mut_slice()) {
		Ok(()) => true,
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

/// Allocate a new [`Ym2149Channel`] and "leak" that pointer into C++ land
#[unsafe(no_mangle)]
pub fn ym2149_channel_new() -> *mut Ym2149Channel
{
	let channel = Box::new(Ym2149Channel::default());
	Box::into_raw(channel)
}

/// Consume the pointer for a [`Ym2149Channel`] allocated with `ym2149_channel_new`
/// and deallocate it cleanly
/// # Safety
/// As long as the pointer given is from `ym2149_channel_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_channel_drop(channel: *mut Ym2149Channel)
{
	let channel = unsafe { Box::from_raw(channel) };
	drop(channel);
}

/// Using the pointer from `ym2149_channel_new`, reset the edge state for the channel
/// # Safety
/// As long as the pointer given is from `ym2149_channel_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_channel_reset_edge_state(channel: *mut Ym2149Channel, state: bool)
{
	let mut channel: NonNull<_> = unsafe { NonNull::new_unchecked(channel) };
	unsafe { channel.as_mut() }.reset_edge_state(state);
}

/// Using the pointer from `ym2149_channel_new`, write the note frequency for the channel
/// # Safety
/// As long as the pointer given is from `ym2149_channel_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_channel_write_frequency(channel: *mut Ym2149Channel, value: u8, rough_adjust: bool)
{
	let mut channel: NonNull<_> = unsafe { NonNull::new_unchecked(channel) };
	unsafe { channel.as_mut() }.write_frequency(value, rough_adjust);
}

/// Using the pointer from `ym2149_channel_new`, read the note frequency for the channel
/// # Safety
/// As long as the pointer given is from `ym2149_channel_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_channel_read_frequency(channel: *mut Ym2149Channel, rough_adjust: bool) -> u8
{
	let channel: NonNull<_> = unsafe { NonNull::new_unchecked(channel) };
	unsafe { channel.as_ref() }.read_frequency(rough_adjust)
}

/// Using the pointer from `ym2149_channel_new`, run a time step for the channel
/// # Safety
/// As long as the pointer given is from `ym2149_channel_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_channel_step(channel: *mut Ym2149Channel)
{
	let mut channel: NonNull<_> = unsafe { NonNull::new_unchecked(channel) };
	unsafe { channel.as_mut() }.step();
}

/// Using the pointer from `ym2149_channel_new`, read the edge state for the channel
/// # Safety
/// As long as the pointer given is from `ym2149_channel_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_channel_state(channel: *mut Ym2149Channel, tone_inhibit: bool) -> bool
{
	let channel: NonNull<_> = unsafe { NonNull::new_unchecked(channel) };
	unsafe { channel.as_ref() }.state(tone_inhibit)
}

/// Using the pointer from `ym2149_channel_new`, check whether a shift is required for the channel
/// # Safety
/// As long as the pointer given is from `ym2149_channel_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_channel_shift_required(channel: *mut Ym2149Channel) -> bool
{
	let channel: NonNull<_> = unsafe { NonNull::new_unchecked(channel) };
	unsafe { channel.as_ref() }.shift_required()
}

/// Using the pointer from `ym2149_channel_new`, force the edge state for the channel
/// # Safety
/// As long as the pointer given is from `ym2149_channel_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_channel_force_edge_state(channel: *mut Ym2149Channel, state: bool)
{
	let mut channel: NonNull<_> = unsafe { NonNull::new_unchecked(channel) };
	unsafe { channel.as_mut() }.force_edge_state(state);
}

/// Using the pointer from `ym2149_channel_new`, get the level of the channel
/// # Safety
/// As long as the pointer given is from `ym2149_channel_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_channel_get_level(channel: *mut Ym2149Channel) -> u8
{
	let channel: NonNull<_> = unsafe { NonNull::new_unchecked(channel) };
	unsafe { channel.as_ref() }.level
}

/// Using the pointer from `ym2149_channel_new`, set the level of the channel
/// # Safety
/// As long as the pointer given is from `ym2149_channel_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_channel_set_level(channel: *mut Ym2149Channel, level: u8)
{
	let mut channel: NonNull<_> = unsafe { NonNull::new_unchecked(channel) };
	unsafe { channel.as_mut() }.level = level;
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
