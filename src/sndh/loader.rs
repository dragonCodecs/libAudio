// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::fmt::{self, Display};

use crate::sndh::ice_decrunch::{self, SndhDecruncher};

#[repr(C)]
pub struct SndhEntryPoints
{
	init: u32,
	exit: u32,
	play: u32,
}

pub enum Error
{
	Decrunch(ice_decrunch::Error),
}

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
		}
	}
}
