// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::fmt::{self, Display};
use std::fs::File;

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
	InvalidSNDH,
}

const TYPE_HEADER: [u8; 4] = *b"SNDH";

impl TryFrom<&mut SndhDecruncher> for SndhEntryPoints
{
	type Error = Error;

	fn try_from(value: &mut SndhDecruncher) -> Result<Self, Error>
	{
		Ok(Self {
			init: value.read_be_u32()?,
			exit: value.read_be_u32()?,
			play: value.read_be_u32()?,
		})
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

impl Display for Error
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		writeln!(fmt, "Error loading SNDH file")?;
		match self {
			Self::Decrunch(error) => write!(fmt, "{error}"),
			Self::InvalidSNDH => write!(fmt, "Invalid SNDH header"),
		}
	}
}
