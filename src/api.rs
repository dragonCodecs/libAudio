// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use core::slice;
use std::ffi::{c_char, c_void};
use std::fs::File;
use std::os::fd::{FromRawFd, IntoRawFd};
use std::ptr::{NonNull, null_mut};

use crate::emulator::Peripheral;
use crate::emulator::sound::ste_dac::STeDac;
use crate::emulator::sound::ym2149::Ym2149;
use crate::emulator::timing::mc68901::{self, MC68901};
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

/// Allocate a new [`Ym2149`] and "leak" that pointer into C++ land
#[unsafe(no_mangle)]
pub fn ym2149_new(clock_frequency: u32, sample_frequency: u32) -> *mut Ym2149
{
	let ym2149 = Box::new(Ym2149::new(clock_frequency, sample_frequency));
	Box::into_raw(ym2149)
}

/// Consume the pointer for a [`Ym2149`] allocated with `ym2149_new`
/// and deallocate it cleanly
/// # Safety
/// As long as the pointer given is from `ym2149_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_drop(ym2149: *mut Ym2149)
{
	let ym2149 = unsafe { Box::from_raw(ym2149) };
	drop(ym2149);
}

/// Using the pointer from `ym2149_new`, read data into the slice from the requested address
/// # Safety
/// As long as the pointer given is from `ym2149_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_read_address(ym2149: *mut Ym2149, address: u32, mut slice: MutSlice<u8>)
{
	let ym2149 = unsafe { NonNull::new_unchecked(ym2149) };
	unsafe { ym2149.as_ref() }.read_address(address, slice.as_mut_slice());
}

/// Using the pointer from `ym2149_new`, write data into the slice to the requested address
/// # Safety
/// As long as the pointer given is from `ym2149_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_write_address(ym2149: *mut Ym2149, address: u32, slice: Slice<u8>)
{
	let mut ym2149 = unsafe { NonNull::new_unchecked(ym2149) };
	unsafe { ym2149.as_mut() }.write_address(address, slice.as_slice());
}

/// Using the pointer from `ym2149_new`, check if the YM2149 has a new sample ready
/// # Safety
/// As long as the pointer given is from `ym2149_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_sample_ready(ym2149: *mut Ym2149) -> bool
{
	let ym2149 = unsafe { NonNull::new_unchecked(ym2149) };
	unsafe { ym2149.as_ref() }.sample_ready()
}

/// Using the pointer from `ym2149_new`, generate a new sample
/// # Safety
/// As long as the pointer given is from `ym2149_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_sample(ym2149: *mut Ym2149) -> i16
{
	let mut ym2149 = unsafe { NonNull::new_unchecked(ym2149) };
	unsafe { ym2149.as_mut() }.sample()
}

/// Using the pointer from `ym2149_new`, run a clock cycle
/// # Safety
/// As long as the pointer given is from `ym2149_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_clock_cycle(ym2149: *mut Ym2149) -> bool
{
	let mut ym2149 = unsafe { NonNull::new_unchecked(ym2149) };
	unsafe { ym2149.as_mut() }.clock_cycle()
}

/// Using the pointer from `ym2149_new`, force the edge state for all channels
/// # Safety
/// As long as the pointer given is from `ym2149_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ym2149_force_edge_state(ym2149: *mut Ym2149, state: bool)
{
	let mut ym2149: NonNull<_> = unsafe { NonNull::new_unchecked(ym2149) };
	unsafe { ym2149.as_mut() }.force_edge_state(state);
}

/// Allocate a new [`STeDac`] and "leak" that pointer into C++ land
#[unsafe(no_mangle)]
pub fn ste_dac_new(mfp: *mut c_void) -> *mut STeDac
{
	let ste_dac = Box::new(STeDac::new(mfp));
	Box::into_raw(ste_dac)
}

/// Consume the pointer for a [`STeDac`] allocated with `ste_dac_new`
/// and deallocate it cleanly
/// # Safety
/// As long as the pointer given is from `ste_dac_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ste_dac_drop(ste_dac: *mut STeDac)
{
	let ste_dac = unsafe { Box::from_raw(ste_dac) };
	drop(ste_dac);
}

/// Using the pointer from `ste_dac_new`, read data into the slice from the requested address
/// # Safety
/// As long as the pointer given is from `ste_dac_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ste_dac_read_address(ste_dac: *mut STeDac, address: u32, mut slice: MutSlice<u8>)
{
	let ste_dac = unsafe { NonNull::new_unchecked(ste_dac) };
	unsafe { ste_dac.as_ref() }.read_address(address, slice.as_mut_slice());
}

/// Using the pointer from `ste_dac_new`, write data into the slice to the requested address
/// # Safety
/// As long as the pointer given is from `ste_dac_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ste_dac_write_address(ste_dac: *mut STeDac, address: u32, slice: Slice<u8>)
{
	let mut ste_dac = unsafe { NonNull::new_unchecked(ste_dac) };
	unsafe { ste_dac.as_mut() }.write_address(address, slice.as_slice());
}

/// Using the pointer from `ste_dac_new`, generate a new sample
/// # Safety
/// As long as the pointer given is from `ste_dac_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ste_dac_sample(ste_dac: *mut STeDac, memory_map: *const c_void) -> i16
{
	let mut ste_dac = unsafe { NonNull::new_unchecked(ste_dac) };
	unsafe { ste_dac.as_mut() }.sample(memory_map)
}

/// Using the pointer from `ste_dac_new`, run a clock cycle
/// # Safety
/// As long as the pointer given is from `ste_dac_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ste_dac_clock_cycle(ste_dac: *mut STeDac) -> bool
{
	let mut ste_dac = unsafe { NonNull::new_unchecked(ste_dac) };
	unsafe { ste_dac.as_mut() }.clock_cycle()
}

/// Using the pointer from `ste_dac_new`, get the currently configured output level
/// # Safety
/// As long as the pointer given is from `ste_dac_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn ste_dac_output_level(ste_dac: *mut STeDac) -> u8
{
	let mut ste_dac = unsafe { NonNull::new_unchecked(ste_dac) };
	unsafe { ste_dac.as_mut() }.output_level()
}

/// Allocate a new [`MC68901`] and "leak" that pointer into C++ land
#[unsafe(no_mangle)]
pub fn mc68901_new(this: *const c_void, clock_frequency: u32) -> *mut MC68901
{
	let mc68901 = Box::new(MC68901::new(this, clock_frequency));
	Box::into_raw(mc68901)
}

/// Consume the pointer for a [`MC68901`] allocated with `mc68901_new`
/// and deallocate it cleanly
/// # Safety
/// As long as the pointer given is from `mc68901_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn mc68901_drop(mc68901: *mut MC68901)
{
	let mc68901 = unsafe { Box::from_raw(mc68901) };
	drop(mc68901);
}

/// Allocate a new [`mc68901::Timer`] and "leak" that pointer into C++ land
#[unsafe(no_mangle)]
pub fn mc68901_timer_new(base_frequency: u32) -> *mut mc68901::Timer
{
	let timer = Box::new(mc68901::Timer::new(base_frequency));
	Box::into_raw(timer)
}

/// Consume the pointer for a [`mc68901::Timer`] allocated with `mc68901_timer_new`
/// and deallocate it cleanly
/// # Safety
/// As long as the pointer given is from `mc68901_timer_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn mc68901_timer_drop(timer: *mut mc68901::Timer)
{
	let timer = unsafe { Box::from_raw(timer) };
	drop(timer);
}

/// Using the pointer from `mc68901_timer_new`, read the control register back
/// # Safety
/// As long as the pointer given is from `mc68901_timer_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn mc68901_timer_read_ctrl(timer: *mut mc68901::Timer) -> u8
{
	let timer = unsafe { NonNull::new_unchecked(timer) };
	unsafe { timer.as_ref() }.read_ctrl()
}

/// Using the pointer from `mc68901_timer_new`, read the data register back
/// # Safety
/// As long as the pointer given is from `mc68901_timer_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn mc68901_timer_read_data(timer: *mut mc68901::Timer) -> u8
{
	let timer = unsafe { NonNull::new_unchecked(timer) };
	unsafe { timer.as_ref() }.read_data()
}

/// Using the pointer from `mc68901_timer_new`, write to the control register
/// # Safety
/// As long as the pointer given is from `mc68901_timer_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn mc68901_timer_write_ctrl(timer: *mut mc68901::Timer, value: u8, base_frequency: u32)
{
	let mut timer = unsafe { NonNull::new_unchecked(timer) };
	unsafe { timer.as_mut() }.write_ctrl(value, base_frequency);
}

/// Using the pointer from `mc68901_timer_new`, write to the data register
/// # Safety
/// As long as the pointer given is from `mc68901_timer_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn mc68901_timer_write_data(timer: *mut mc68901::Timer, value: u8)
{
	let mut timer = unsafe { NonNull::new_unchecked(timer) };
	unsafe { timer.as_mut() }.write_data(value);
}

/// Using the pointer from `mc68901_timer_new`, mark an external event occuring on the timer
/// # Safety
/// As long as the pointer given is from `mc68901_timer_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn mc68901_timer_mark_external_event(timer: *mut mc68901::Timer)
{
	let mut timer = unsafe { NonNull::new_unchecked(timer) };
	unsafe { timer.as_mut() }.mark_external_event();
}

/// Using the pointer from `mc68901_timer_new`, run a clock cycle
/// # Safety
/// As long as the pointer given is from `mc68901_timer_new`, this is safe
#[unsafe(no_mangle)]
pub unsafe fn mc68901_timer_clock_cycle(timer: *mut mc68901::Timer) -> bool
{
	let mut timer = unsafe { NonNull::new_unchecked(timer) };
	unsafe { timer.as_mut() }.clock_cycle()
}

/// Calculate the prescaling for the given timer mode
#[unsafe(no_mangle)]
pub fn mc68901_timer_prescaling_for(mode: u8) -> u32
{
	mc68901::Timer::prescaling_for(mode)
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
