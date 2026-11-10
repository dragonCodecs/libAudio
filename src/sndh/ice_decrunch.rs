// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::fmt::{self, Display};
use std::fs::File;
use std::io::{self, Read};
use std::mem;
use std::ops::{Add, Range, SubAssign};

use super::atari_ascii::ATARI_CHARSET;

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

#[derive(Clone, Copy)]
struct Downcounter(usize);

pub enum Error
{
	Io(io::Error),
	FileToBig,
	InvalidPackIceHeader,
	NoMoreData,
}

const PACK_ICE_MAGIC: [u8; 4] = *b"ICE!";
// Because of Atari STe RAM limitations, assume no file can be larger than 4MiB
const MAX_FILE_LENGTH: u64 = 4 * 1024 * 1024;

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

fn read_be_u32(file: &mut File) -> Result<u32, Error>
{
	let mut data = [0; 4];
	file.read_exact(&mut data)?;
	Ok(u32::from_be_bytes(data))
}

impl SndhDecruncher
{
	pub fn new(file: &mut File) -> Result<Self, Error>
	{
		let mut magic = [0; 4];
		file.read_exact(&mut magic)?;
		let file_length = file.metadata()?.len();

		// If this is not an ICE Pack packed file, just copy the contents into memory having made sure it's not too big
		if magic != PACK_ICE_MAGIC {
			if file_length > MAX_FILE_LENGTH {
				return Err(Error::FileToBig);
			}
			// Allocate enough memory to hold the entire file
			let data = Box::new_zeroed_slice(file_length as usize);
			let mut data = unsafe { data.assume_init() };
			// Copy in the header we already consumed, then copy in the rest of the file's data
			data[0..4].copy_from_slice(&magic);
			file.read_exact(&mut data[4..])?;
			Ok(Self {
				data,
				offset: 0,
			})
		} else {
			// Read the two lengths that follow the Pack Ice magic
			let packed_length = read_be_u32(file)?;
			let unpacked_length = read_be_u32(file)?;
			// Validate them to make sure they're sane
			if packed_length != file_length as u32 || unpacked_length > MAX_FILE_LENGTH as u32 {
				return Err(Error::InvalidPackIceHeader);
			}

			let data = Box::new_zeroed_slice(unpacked_length as usize);
			let this = Self {
				data: unsafe { data.assume_init() },
				offset: 0,
			};
			this.unpack(file)
		}
	}

	fn unpack(mut self, file: &File) -> Result<Self, Error>
	{
		let decruncher = Decruncher::new(file, &mut self.data)?;
		decruncher.decrunch();
		Ok(self)
	}

	pub fn read<T: Sized + Copy>(&mut self, value: &mut T) -> Result<(), Error>
	{
		let length = size_of_val(value);
		if self.offset + length > self.data.len() {
			Err(Error::NoMoreData)
		} else {
			// This is safe because we ensure the amount of data copied in matches the size of T
			// and we guarantee that T is Copy, so memcpy()-safe
			*value = unsafe { mem::transmute_copy(&self.data[self.offset..self.offset + length]) };
			self.offset += length;
			Ok(())
		}
	}

	pub fn read_be_u32(&mut self) -> Result<u32, Error>
	{
		let mut data = [0; 4];
		self.read(&mut data)?;
		Ok(u32::from_be_bytes(data))
	}

	pub fn read_string(&mut self) -> Result<String, Error>
	{
		// Start by finding the extents of the string
		let begin = self.offset;
		if begin >= self.data.len() {
			return Err(Error::NoMoreData);
		}
		let mut end = begin;
		while self.data[end] != 0 {
			end += 1;
			if end >= self.data.len() {
				return Err(Error::NoMoreData);
			}
		}
		// Now select the slice and feed it through the Atari characterset mapper
		let result = self.data[begin..end]
			.iter()
			.map(|&char| ATARI_CHARSET[char as usize])
			.collect();
		// Chew through any remaining NUL characters after the string
		while self.data[end] == 0 {
			end += 1;
			if end > self.data.len() {
				return Err(Error::NoMoreData);
			}
		}
		// Copy the new read offset back for the next time
		self.offset = end;
		Ok(result)
	}

	pub fn data(&self) -> &[u8]
	{
		&self.data
	}

	pub fn data_mut(&mut self) -> &mut [u8]
	{
		&mut self.data
	}

	pub fn offset(&self) -> &usize
	{
		&self.offset
	}
}

impl<'a> Decruncher<'a>
{
	pub fn new(mut file: &File, data: &'a mut Box<[u8]>) -> Result<Self, Error>
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

	/// Decrunch the input data to the output buffer
	pub fn decrunch(mut self)
	{
		// Decrunching starts at the end of the input and works backwards
		self.input_offset = Downcounter::new(self.crunched_data.len());
		self.output_offset = self.decrunched_data.len();
		// Grab our first working data information if possible
		self.working_data = self.crunched_data[self.input_offset.next()] as u16;

		while self.output_offset != 0 {
			if self.read_bit() {
				self.copy_bytes();
			}
			if self.output_offset == 0 {
				break;
			}
			self.unpack_from_offset();
		}

		if self.read_bit() {
			let blocks = if self.read_bit() {
				self.read_bits(15) as u16
			} else {
				0x0f9f
			};
			self.unshuffle(blocks)
		}
	}

	/// Read a single bit from the input
	#[must_use]
	fn read_bit(&mut self) -> bool
	{
		// Extract the bit we need
		let mut result = (self.working_data & 0xff) << 1;
		// Now see if our next-byte condition has been met
		if (result & 0xff) == 0 {
			// If it has, then wiggle things about in the result value to get the next byte
			result >>= 8;
			result += (self.crunched_data[self.input_offset.next()] as u16) << 1;
		}
		// Update the working data area for the next call
		self.working_data &= 0xff00;
		self.working_data |= result & 0xff;
		(result >> 8) != 0
	}

	/// Read `bits` bits from the input
	#[must_use]
	fn read_bits(&mut self, bits: u16) -> u32
	{
		let mut data = self.working_data;
		let mut result: u32 = 0;

		// Loop through for the requested number of bits
		for _ in 0..=bits {
			// Try and extract a bit, and if we're at a next-byte condition, handle that
			data &= 0xff;
			data <<= 1;
			if (data & 0xff) == 0 {
				data >>= 8;
				data += (self.crunched_data[self.input_offset.next()] as u16) << 1;
			}

			// Update the result with the next bit
			result <<= 1;
			result += (data >> 8) as u32;
		}

		// Update the working data area for the next call
		self.working_data &= 0xff00;
		self.working_data |= data & 0xff;
		result
	}

	fn copy_bytes(&mut self)
	{
		// Keep track of how many bytes we've copied
		let mut count = 1;
		// Check if this is a signle byte copy or a multi-byte
		if self.read_bit() {
			// Base case is that the chunk table won't be satisfied
			let mut index = 5;
			for (i, chunk) in CHUNK_TABLE.iter().enumerate() {
				// Read the next block count and check if the result satisfies the chunk mask
				count = self.read_bits(chunk.bits) as u16;
				if (chunk.mask ^ count) != 0 {
					// If it does, note the chunk table index
					index = i;
					break;
				}
			}
			// If the chunk table was satisfied, update the count with the number of extra
			// bytes needed for this chunk, otherwise by 1
			count += if index < 5 {
				EXTRA_BYTES[index]
			} else {
				1
			};
		}

		// Update the offset counters, and copy the chunk from input to output
		self.input_offset -= count;
		self.output_offset -= count as usize;
		self.decrunched_data[self.output_offset..self.output_offset + count as usize]
			.copy_from_slice(&self.crunched_data[self.input_offset.range_for(count)]);
	}

	fn unpack_bytes(&mut self, offset: i16, count: usize)
	{
		let from_offset = self.output_offset.strict_add_signed(offset as isize) + count;
		let to_offset = self.output_offset;
		for index in 1..=count {
			self.decrunched_data[to_offset - index] = self.decrunched_data[from_offset - index];
		}
		self.output_offset -= count;
	}

	fn unpack_from_offset(&mut self)
	{
		// Figure out the table index to use
		let mut index = 4;
		for i in 0..4 {
			// Check if this is the table index to use
			if !self.read_bit() {
				index = i;
				break;
			}
		}

		// Calculate the length adjustment needed
		let bits = LENGTH_TABLE[4 - index] as u8;
		let length_adjustment = if (bits & 0x80) == 0 {
			self.read_bits(bits as u16)
		} else {
			0
		};

		// Figure out how much and from where to unpack
		let length = length_adjustment + (LENGTH_TABLE[9 - index] + 1) as u32;
		let offset = if length > 1 {
			self.offset_from_table(length - 1)
		} else {
			self.offset_from_calculation()
		};
		self.unpack_bytes(offset, (length + 1) as usize)
	}

	fn offset_from_table(&mut self, length: u32) -> i16
	{
		// Figure out the table index to use
		let mut index = 2;
		for i in 0..2 {
			// Check if this is the table index to use
			if !self.read_bit() {
				index = i;
				break;
			}
		}
		// Read the base offset from the file
		let mut offset = self.read_bits(INT8_OFFSETS[2 - index] as u16) as i32;
		// Adjust it properly
		offset += INT16_OFFSETS[4 - index] as i32;
		if offset < 0 {
			offset -= length as i32;
		}
		// and return the result
		offset as i16
	}

	fn offset_from_calculation(&mut self) -> i16
	{
		// Figure out how many bits and what adjustment to use
		let (bits, adjustment) = if self.read_bit() {
			(8, 63)
		} else {
			(5, -1)
		};
		// Read the offset from the input and turn it into an i16
		let offset = self.read_bits(bits) as i16;
		// Apply the adjustment and return the resulting offset
		offset + adjustment
	}

	fn unshuffle(&mut self, blocks: u16)
	{
		let mut offset = self.decrunched_data.len();
		for _ in 0..blocks {
			let mut state = [0; 4];
			// Construct the state for this block
			for index in 0..4 {
				offset -= 2;
				// Grab a big endian u16 chunk of data from the buffer
				let mut chunk = u16::from_be_bytes(
					*self.decrunched_data[offset..offset + 2]
						.as_array::<2>()
						.expect("Slice of 2 bytes converts to array of 2 bytes"),
				);

				// Do the state unshuffle on it to integrate it properly (stripes the bits for this u16 across the state
				// over 4 iterations, 4 bits at a time)
				for iter in 0..4 {
					let shift = ((3 - index) * 4) + iter;
					state[3] |= (chunk & 1) << shift;
					chunk >>= 1;
					state[2] |= (chunk & 1) << shift;
					chunk >>= 1;
					state[1] |= (chunk & 1) << shift;
					chunk >>= 1;
					state[0] |= (chunk & 1) << shift;
					chunk >>= 1;
				}
			}

			// Write the unshuffled state back to the buffer as a series of big endian u16's
			self.decrunched_data[offset..offset + 2].copy_from_slice(&state[0].to_be_bytes());
			self.decrunched_data[offset + 2..offset + 4].copy_from_slice(&state[1].to_be_bytes());
			self.decrunched_data[offset + 4..offset + 6].copy_from_slice(&state[2].to_be_bytes());
			self.decrunched_data[offset + 6..offset + 8].copy_from_slice(&state[3].to_be_bytes());
		}
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

	pub fn range_for(&self, count: u16) -> Range<usize>
	{
		self.0..self.0 + count as usize
	}
}

impl Default for Downcounter
{
	fn default() -> Self
	{
		Self::new(0)
	}
}

impl SubAssign<u16> for Downcounter
{
	fn sub_assign(&mut self, rhs: u16)
	{
		self.0 -= rhs as usize;
	}
}

impl Add<u16> for Downcounter
{
	type Output = Self;

	fn add(self, rhs: u16) -> Self
	{
		Self(self.0 + rhs as usize)
	}
}

impl From<io::Error> for Error
{
	fn from(error: io::Error) -> Self
	{
		Error::Io(error)
	}
}

impl Display for Error
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		write!(fmt, "Error while decrunching file - ")?;
		match self {
			Self::Io(error) => write!(fmt, "IO error: {error}"),
			Self::FileToBig => write!(fmt, "input file too large (> 4MiB)"),
			Self::InvalidPackIceHeader => write!(fmt, "appears to be Pack Ice'd, but invalid header"),
			Self::NoMoreData => write!(fmt, "decrunched data exhausted"),
		}
	}
}
