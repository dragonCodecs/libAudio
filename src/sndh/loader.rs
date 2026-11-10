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

#[repr(u8)]
#[allow(unused)]
pub enum SndhTimer
{
	Invalid = b'\0',
	A = b'A',
	B = b'B',
	C = b'C',
	D = b'D',
	Vbl = b'V',
}

#[allow(unused)]
pub struct SndhLoader
{
	decruncher: SndhDecruncher,
	entry_points: SndhEntryPoints,
	metadata: SndhMetadata,
}

pub enum Error
{
	Decrunch(ice_decrunch::Error),
	String(Utf8Error),
	IntConversion(ParseIntError),
	InvalidSNDH,
}

const TYPE_HEADER: [u8; 4] = *b"SNDH";
const TYPE_TITLE: [u8; 4] = *b"TITL";
const TYPE_COMPOSER: [u8; 4] = *b"COMM";
const TYPE_RIPPER: [u8; 4] = *b"RIPP";
const TYPE_CONVERTER: [u8; 4] = *b"CONV";
const TYPE_TUNE_NAMES: [u8; 4] = *b"#!SN";
const TYPE_TUNE_COUNT: [u8; 2] = *b"##";
const TYPE_DEFAULT_TUNE: [u8; 2] = *b"!#";
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
				TYPE_TUNE_NAMES => {
					if self.tune_names.is_empty() {
						return Err(Error::InvalidSNDH);
					}
					for tune_name in self.tune_names.iter_mut() {
						*tune_name = decruncher.read_string()?;
					}
				},
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
					[TYPE_DEFAULT_TUNE, value] => {
						// Reconstruct the default tune number from the 2 bytes we get from the tag, and
						// any remaining after that if we try and read a string from the SNDH data
						let value = str::from_utf8(value)?.to_owned() + &decruncher.read_string()?;
						// Now try and parse the resulting vaue to a u8
						self.default_tune = value.parse()?;
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
			timer_frequency: 0,
			year: 0,
			timer: SndhTimer::Invalid,
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
		}
	}
}
