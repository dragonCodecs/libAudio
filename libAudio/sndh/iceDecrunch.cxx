// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2022-2023 Rachel Mant <git@dragonmux.network>
#include "iceDecrunch.hxx"

extern "C"
{
	uint8_t *sndh_decruncher_data(SndhDecruncher *);
	size_t sndh_decruncher_length(SndhDecruncher *);
	size_t *sndh_decruncher_offset(SndhDecruncher *);
}

sndhDecruncher_t::sndhDecruncher_t(SndhDecruncher *decruncher) : _decruncher{decruncher}
{
	_data = {sndh_decruncher_data(_decruncher), sndh_decruncher_length(_decruncher)};
	_offset = sndh_decruncher_offset(_decruncher);
}
