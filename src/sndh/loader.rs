// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::fmt::{self, Display};
use std::fs::File;
use std::mem::MaybeUninit;
use std::num::ParseIntError;
use std::str::Utf8Error;

use crate::sndh::ice_decrunch::{self, SndhDecruncher};

#[repr(C)]
pub struct SndhEntryPoints
{
	init: u32,
	exit: u32,
	play: u32,
}

#[allow(unused)]
pub struct SndhMetadata
{
	title: String,
	artist: String,
	ripper: String,
	converter: String,
	tune_count: u8,
	default_tune: u8,
	timer_frequency: u16,
	year: u32,
	timer: SndhTimer,
	tune_times: Box<[u16]>,
	tune_frame_counts: Box<[u32]>,
	tune_names: Box<[String]>,
}

#[derive(Default)]
#[repr(u8)]
pub enum SndhTimer
{
	A = b'A',
	B = b'B',
	#[default]
	C = b'C',
	D = b'D',
	Vbl = b'V',
}

pub struct SndhLoader
{
	decruncher: SndhDecruncher,
	entry_points: SndhEntryPoints,
	#[allow(unused)]
	metadata: SndhMetadata,
}

pub enum Error
{
	Decrunch(ice_decrunch::Error),
	String(Utf8Error),
	IntConversion(ParseIntError),
	InvalidSNDH,
	InvalidTimer(char),
}

const TYPE_HEADER: [u8; 4] = *b"SNDH";
const TYPE_TITLE: [u8; 4] = *b"TITL";
const TYPE_COMPOSER: [u8; 4] = *b"COMM";
const TYPE_RIPPER: [u8; 4] = *b"RIPP";
const TYPE_CONVERTER: [u8; 4] = *b"CONV";
const TYPE_TUNE_NAMES: [u8; 4] = *b"#!SN";
const TYPE_TUNE_COUNT: [u8; 2] = *b"##";
const TYPE_DEFAULT_TUNE: [u8; 2] = *b"!#";
const TYPE_TIMER_A: [u8; 2] = *b"TA";
const TYPE_TIMER_B: [u8; 2] = *b"TB";
const TYPE_TIMER_C: [u8; 2] = *b"TC";
const TYPE_TIMER_D: [u8; 2] = *b"TD";
const TYPE_TIMER_VBL: [u8; 2] = *b"!V";
const TYPE_YEAR: [u8; 4] = *b"YEAR";
const TYPE_TIME: [u8; 4] = *b"TIME";
const TYPE_FRAMES: [u8; 4] = *b"FRMS";
const TYPE_END: [u8; 4] = *b"HDNS";

impl TryFrom<&mut SndhDecruncher> for SndhEntryPoints
{
	type Error = Error;

	fn try_from(decruncher: &mut SndhDecruncher) -> Result<Self, Error>
	{
		Ok(Self {
			init: decruncher.read_be_u32()?,
			exit: decruncher.read_be_u32()?,
			play: decruncher.read_be_u32()?,
		})
	}
}

impl SndhMetadata
{
	fn read_from(&mut self, decruncher: &mut SndhDecruncher) -> Result<(), Error>
	{
		let mut tag_type = [0; 4];
		while tag_type != TYPE_END {
			// Try to read the next metadata tag
			decruncher.read(&mut tag_type)?;
			// And match it to the list of known metdata tags
			match tag_type {
				TYPE_TITLE => self.title = decruncher.read_string()?,
				TYPE_COMPOSER => self.artist = decruncher.read_string()?,
				TYPE_RIPPER => self.ripper = decruncher.read_string()?,
				TYPE_CONVERTER => self.converter = decruncher.read_string()?,
				// SNDHv2.1 metadata tag
				TYPE_TUNE_NAMES => {
					// Make sure we allocated space for the tune names (meaning tune count was consumed)
					if self.tune_names.is_empty() {
						return Err(Error::InvalidSNDH);
					}
					// Then consume each a string at a time and stuff it into storage
					for tune_name in self.tune_names.iter_mut() {
						*tune_name = decruncher.read_string()?;
					}
				},
				// SNDHv2.0 metadata tag
				TYPE_YEAR => {
					// try to read the year number that follows this tag and then parse it into a u32
					let year = decruncher.read_string()?;
					self.year = year.parse()?;
				},
				// SNDHv2.0 metadata tag
				TYPE_TIME => {
					// Make sure we allocated space for the tune times (meaning tune count was consumed)
					if self.tune_times.is_empty() {
						return Err(Error::InvalidSNDH);
					}
					// Then consume each of the tune times one big endian u16 at a time and stuff them into storage
					for tune_time in self.tune_times.iter_mut() {
						*tune_time = decruncher.read_be_u16()?;
					}
				},
				// SNDHv2.2 metadata tag
				TYPE_FRAMES => {
					// Make sure we've not seen this tag before
					if !self.tune_frame_counts.is_empty() {
						// As it's an error to see it more than once
						return Err(Error::InvalidSNDH);
					}
					// If the tune count is 0 (because we never saw ##) then assume there's only 1 tune
					let tunes = if self.tune_count > 0 {
						self.tune_count
					} else {
						1
					} as usize;
					// Allocate enough storage for the frame counts
					let frame_counts = Box::new_zeroed_slice(tunes);
					self.tune_frame_counts = unsafe { frame_counts.assume_init() };
					// Then loop through and try and read the frame counts from the SNDH data
					for frame_count in self.tune_frame_counts.iter_mut() {
						*frame_count = decruncher.read_be_u32()?;
					}
				},
				// SNDHv2.0 metadata tag marking the end of the metadata block
				TYPE_END => break,
				// Try and match 2 byte metdata tags next
				// This chunking is safe as the tag type is guaranteed to be 4 bytes, thus no remainder
				_ => match unsafe { tag_type.as_chunks_unchecked::<2>() } {
					[TYPE_TUNE_COUNT, count] => {
						// Reconstruct the tune count from the 2 bytes we get from the tag, and any remaining
						// after that if we try and read a string from the SNDH data
						let count = str::from_utf8(count)?.to_owned() + &decruncher.read_string()?;
						// Now try and parse the resulting value to a u8
						self.tune_count = count.parse()?;
						// Pre-init the tune times and names to have sufficient storage
						let tune_times = Box::new_zeroed_slice(self.tune_count as usize);
						self.tune_times = unsafe { tune_times.assume_init() };
						let mut tune_names = Box::new_uninit_slice(self.tune_count as usize);
						for tune_name in tune_names.iter_mut() {
							*tune_name = MaybeUninit::new(String::new());
						}
						self.tune_names = unsafe { tune_names.assume_init() };
					},
					// SNDHv2.1 metadata tag
					[TYPE_DEFAULT_TUNE, value] => {
						// Reconstruct the default tune number from the 2 bytes we get from the tag, and
						// any remaining after that if we try and read a string from the SNDH data
						let value = str::from_utf8(value)?.to_owned() + &decruncher.read_string()?;
						// Now try and parse the resulting vaue to a u8
						self.default_tune = value.parse()?;
					},
					[TYPE_TIMER_A, frequency] |
					[TYPE_TIMER_B, frequency] |
					[TYPE_TIMER_C, frequency] |
					[TYPE_TIMER_D, frequency] |
					[TYPE_TIMER_VBL, frequency] => {
						// Try and convert the timer tag into a [`SndhTimer`] value
						self.timer = tag_type[1].try_into()?;
						// Reconstruct the frequency to run the tune at from the 2 bytes we get in the tag,
						// and any remaining after that if we try and read a string from the SNDH data
						let frequency = str::from_utf8(frequency)?.to_owned() + &decruncher.read_string()?;
						// Now try and parse the resulting value to a u16
						self.timer_frequency = frequency.parse()?;
					},
					_ => {
						// If it did not match one of the known types, most likely we're in a SNDH v1.1 file
						// and came to the end of the header (does not end HDNS) so we're done
						decruncher.seek_back(4)?;
						break;
					},
				},
			};
		}
		Ok(())
	}
}

impl Default for SndhMetadata
{
	fn default() -> Self
	{
		Self {
			title: String::new(),
			artist: String::new(),
			ripper: String::new(),
			converter: String::new(),
			tune_count: 0,
			default_tune: 1,
			// Default is 50Hz on timer C
			timer_frequency: 50,
			year: 0,
			timer: SndhTimer::C,
			tune_times: Box::new([]),
			tune_frame_counts: Box::new([]),
			tune_names: Box::new([]),
		}
	}
}

impl TryFrom<&mut SndhDecruncher> for SndhMetadata
{
	type Error = Error;

	fn try_from(decruncher: &mut SndhDecruncher) -> Result<Self, Error>
	{
		let mut metadata = SndhMetadata::default();
		metadata.read_from(decruncher)?;
		Ok(metadata)
	}
}

impl TryFrom<u8> for SndhTimer
{
	type Error = Error;

	fn try_from(timer: u8) -> Result<Self, Error>
	{
		match timer {
			b'A' => Ok(Self::A),
			b'B' => Ok(Self::B),
			b'C' => Ok(Self::C),
			b'D' => Ok(Self::D),
			b'V' => Ok(Self::Vbl),
			_ => Err(Error::InvalidTimer(timer as char)),
		}
	}
}

impl SndhLoader
{
	pub fn new(file: &mut File) -> Result<Self, Error>
	{
		// Try decrunching the file given to extract the SNDH data into memory
		let mut decruncher = SndhDecruncher::new(file)?;
		// Try then and extract the entrypoints for the SNDH file from the resulting data
		let entry_points = SndhEntryPoints::try_from(&mut decruncher)?;
		// Try and read the magic number that follows them and validate that
		let mut magic = [0; 4];
		decruncher.read(&mut magic)?;
		if magic != TYPE_HEADER {
			return Err(Error::InvalidSNDH);
		}
		let this = Self {
			decruncher,
			entry_points,
			metadata: SndhMetadata::default(),
		};

		Ok(this)
	}

	pub fn entry_points(&self) -> &SndhEntryPoints
	{
		&self.entry_points
	}

	pub fn decruncher_mut(&mut self) -> &mut SndhDecruncher
	{
		&mut self.decruncher
	}
}

impl From<ice_decrunch::Error> for Error
{
	fn from(error: ice_decrunch::Error) -> Self
	{
		Error::Decrunch(error)
	}
}

impl From<Utf8Error> for Error
{
	fn from(error: Utf8Error) -> Self
	{
		Error::String(error)
	}
}

impl From<ParseIntError> for Error
{
	fn from(error: ParseIntError) -> Self
	{
		Error::IntConversion(error)
	}
}

impl Display for Error
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		writeln!(fmt, "Error loading SNDH file")?;
		match self {
			Self::Decrunch(error) => write!(fmt, "{error}"),
			Self::String(error) => write!(fmt, "{error}"),
			Self::IntConversion(error) => write!(fmt, "{error}"),
			Self::InvalidSNDH => write!(fmt, "Invalid SNDH header"),
			Self::InvalidTimer(timer) => write!(fmt, "Invalid timer specified: '{timer}'"),
		}
	}
}
