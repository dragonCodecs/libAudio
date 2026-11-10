// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2022-2023 Rachel Mant <git@dragonmux.network>
#include "iceDecrunch.hxx"
#include "console.hxx"

extern "C"
{
	SndhDecruncher *sndh_decruncher_new(int32_t fd);
	void sndh_decruncher_drop(SndhDecruncher *);

	uint8_t *sndh_decruncher_data(SndhDecruncher *);
	size_t sndh_decruncher_length(SndhDecruncher *);
	size_t *sndh_decruncher_offset(SndhDecruncher *);

	void console_error(const char *message)
	{
		console.error(message);
	}
}

struct decrunchingError_t : std::exception
{
	const char *what() const noexcept override { return "Error while decrunching file"; }
};

sndhDecruncher_t::sndhDecruncher_t(const fd_t &file)
{
	_decruncher = sndh_decruncher_new(file);
	if (_decruncher == nullptr)
		throw decrunchingError_t{};
	_data = {sndh_decruncher_data(_decruncher), sndh_decruncher_length(_decruncher)};
	_offset = sndh_decruncher_offset(_decruncher);
}

sndhDecruncher_t::~sndhDecruncher_t()
{
	if (_decruncher)
		sndh_decruncher_drop(_decruncher);
}
