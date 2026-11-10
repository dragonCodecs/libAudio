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
	decruncher = sndh_decruncher_new(file);
	if (decruncher == nullptr)
		throw decrunchingError_t{};
	_data = {sndh_decruncher_data(decruncher), sndh_decruncher_length(decruncher)};
}

sndhDecruncher_t::~sndhDecruncher_t()
{
	if (decruncher)
		sndh_decruncher_drop(decruncher);
}
