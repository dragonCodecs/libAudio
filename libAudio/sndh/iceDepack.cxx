// SPDX-License-Identifier: BSD-3-Clause
#include <substrate/units>
#include <substrate/span>
#include "iceDepack.hxx"
#include "console.hxx"

using substrate::operator ""_MiB;
using substrate::span;

constexpr static std::array<char, 4> magic{{'I', 'C', 'E', '!'}};
constexpr static size_t maxFileLength{4_MiB};

struct decrunchingError_t : std::exception
{
	const char *what() const noexcept override { return "Error while decrunching and unpacking file"; }
};

struct decruncher_t
{
private:
	const fixedVector_t<uint8_t> crunchedData;
	std::span<uint8_t> decrunchedData;

public:
	decruncher_t(const fd_t &file, span<uint8_t> data) : crunchedData{file.length() - 12}, decrunchedData{data}
	{
		if (!crunchedData.valid() ||
			!file.read(crunchedData.data(), crunchedData.size()))
			throw decrunchingError_t{};
	}
};

sndhDepacker_t::sndhDepacker_t(const fd_t &file)
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

	_data = {unpackedLength};
	if (!depack(file))
	{
		_data = {};
		throw std::exception{};
	}
}

bool sndhDepacker_t::depack(const fd_t &file) noexcept try
{
	decruncher_t decruncher{file, {reinterpret_cast<uint8_t *>(_data.data()), _data.size()}};
	return true;
}
catch (const decrunchingError_t &error)
{
	console.error(error.what());
	return false;
}
