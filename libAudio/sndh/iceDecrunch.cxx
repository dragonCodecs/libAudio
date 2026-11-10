// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2022-2023 Rachel Mant <git@dragonmux.network>
#include <substrate/units>
#include <substrate/span>
#include "iceDecrunch.hxx"
#include "console.hxx"

using substrate::operator ""_MiB;
using substrate::span;

constexpr static std::array<char, 4> magic{{'I', 'C', 'E', '!'}};
constexpr static size_t maxFileLength{4_MiB};

extern "C"
{
	SndhDecruncher *sndh_decruncher_new(size_t unpacked_size);
	void sndh_decruncher_drop(SndhDecruncher *);

	bool sndh_decruncher_depack(SndhDecruncher *, int32_t);
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
	std::array<char, 4> icePackMagic;
	if (!file.read(icePackMagic))
		throw std::exception{};
	const size_t fileLength = file.length();

	// If this is not an ice packed file, just copy the contents into memory having made sure it's not too big.
	if (icePackMagic != magic)
	{
		if (fileLength > maxFileLength)
			throw std::exception{};
		_data = {fileLength};
		std::memcpy(_data.data(), icePackMagic.data(), icePackMagic.size());
		if (!file.read(_data.data() + 4, fileLength - 4))
			throw std::exception{};
		return;
	}

	// Otherwise, read the length information, cross-check that against the expected information, and depack.
	uint32_t packedLength;
	uint32_t unpackedLength;
	if (!file.readBE(packedLength) ||
		!file.readBE(unpackedLength) ||
		packedLength != fileLength ||
		unpackedLength > maxFileLength)
		throw std::exception{};

	decruncher = sndh_decruncher_new(unpackedLength);
	_data = {unpackedLength};
	if (!depack(file))
	{
		_data = {};
		throw std::exception{};
	}
}

sndhDecruncher_t::~sndhDecruncher_t()
{
	if (decruncher)
		sndh_decruncher_drop(decruncher);
}

bool sndhDecruncher_t::depack(const fd_t &file) noexcept
{
	const auto result{sndh_decruncher_depack(decruncher, file)};
	const auto data{sndh_decruncher_data(decruncher)};
	const auto size{sndh_decruncher_length(decruncher)};
	std::memcpy(_data.data(), data, size);
	return result;
}
