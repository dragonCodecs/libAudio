// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2022-2023 Rachel Mant <git@dragonmux.network>
#include "iceDecrunch.hxx"
#include "loader.hxx"

extern "C"
{
	size_t sndh_decruncher_length(const SndhDecruncher *);
	void sndh_decruncher_seek_start(SndhDecruncher *);
	bool sndh_decruncher_read(SndhDecruncher *, slice_t<uint8_t>);
}

size_t SndhDecruncher::length() const
{
	return sndh_decruncher_length(this);
}

bool SndhDecruncher::seek_start()
{
	sndh_decruncher_seek_start(this);
	return true;
}

bool SndhDecruncher::read(substrate::span<uint8_t> span)
{
	slice_t<uint8_t> slice{span.data(), span.size()};
	return sndh_decruncher_read(this, slice);
}
