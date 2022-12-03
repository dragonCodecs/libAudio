// SPDX-License-Identifier: BSD-3-Clause
#include <substrate/units>
#include "iceDepack.hxx"

using substrate::operator ""_MiB;

constexpr static std::array<char, 4> magic{{'I', 'C', 'E', '!'}};
constexpr static size_t maxFileLength{4_MiB};

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
		_data = substrate::make_unique<char []>(fileLength);
		std::memcpy(_data.get(), magic.data(), magic.size());
		if (!file.read(_data.get() + 4, fileLength - 4))
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

	_data = substrate::make_unique<char []>(unpackedLength);
	if (!depack(file))
	{
		_data.reset();
		throw std::exception{};
	}
}

bool sndhDepacker_t::depack(const fd_t &file) noexcept
{
}
