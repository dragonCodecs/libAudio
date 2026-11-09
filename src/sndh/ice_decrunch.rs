// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::fs::File;
use std::io::{self, Read};

pub struct SndhDecruncher
{
	data: Box<[u8]>,
	offset: usize,
}

struct ByteBlock
{
	bits: u16,
	mask: u16,
}

struct Decruncher<'a>
{
	crunched_data: Box<[u8]>,
	decrunched_data: &'a mut Box<[u8]>,

	input_offset: Downcounter,
	output_offset: usize,
	working_data: u16,
}

struct Downcounter(usize);

pub enum Error
{
	Io(io::Error),
}

const CHUNK_TABLE: [ByteBlock; 5] = [
	ByteBlock {
		bits: 1,
		mask: 0x0003,
	},
	ByteBlock {
		bits: 1,
		mask: 0x0003,
	},
	ByteBlock {
		bits: 2,
		mask: 0x0007,
	},
	ByteBlock {
		bits: 7,
		mask: 0x00ff,
	},
	ByteBlock {
		bits: 14,
		mask: 0x7fff,
	},
];

const EXTRA_BYTES: [u16; 5] = [2, 5, 8, 15, 270];

const LENGTH_TABLE: [i8; 10] = [9, 1, 0, -1, -1, 8, 4, 2, 1, 0];
const INT8_OFFSETS: [i8; 10] = [11, 4, 7, 0, 1, 31, -1, -1, 0, 31];
const INT16_OFFSETS: [i16; 5] = [2820, 1792, 287, -1, 31];

impl SndhDecruncher
{
	pub fn depack(&mut self, file: File) -> Result<(), Error>
	{
		let decruncher = Decruncher::new(file, &mut self.data)?;
		Ok(decruncher.decrunch())
	}
}

impl<'a> Decruncher<'a>
{
	pub fn new(mut file: File, data: &'a mut Box<[u8]>) -> Result<Self, Error>
	{
		let crunched_data = {
			// Compute how big the file is, if possible
			let size = file
				.metadata()
				.map(|metadata| usize::try_from(metadata.len()).unwrap_or(usize::MAX))
				.ok();
			// Allocate a Vec with enough space and read the file contents in
			let mut buffer = Vec::with_capacity(size.unwrap_or(0));
			file.read_to_end(&mut buffer)?;
			buffer.into_boxed_slice()
		};

		Ok(Self {
			crunched_data,
			decrunched_data: data,

			input_offset: Downcounter::default(),
			output_offset: 0,
			working_data: 0,
		})
	}

	pub fn decrunch(mut self)
	{
		// Decrunching starts at the end of the input and works backwards
		self.input_offset = Downcounter::new(self.crunched_data.len());
		self.output_offset = self.decrunched_data.len();
		// Grab our first working data information if possible
		self.working_data = self.crunched_data[self.input_offset.next()] as u16;
	}
}

impl Downcounter
{
	pub fn new(value: usize) -> Self
	{
		Self(value)
	}

	pub fn next(&mut self) -> usize
	{
		self.0 = self.0.saturating_sub(1);
		self.0
	}
}

impl Default for Downcounter
{
	fn default() -> Self
	{
		Self::new(0)
	}
}

impl From<io::Error> for Error
{
	fn from(error: io::Error) -> Self
	{
		Error::Io(error)
	}
}
