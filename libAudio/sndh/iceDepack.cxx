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

struct byteBlock_t
{
	uint16_t bits;
	uint16_t mask;
};

constexpr static auto chunkTable
{
	substrate::make_array<byteBlock_t>({
		{1U, 0x0003U},
		{1U, 0x0003U},
		{2U, 0x0007U},
		{7U, 0x00ffU},
		{14U, 0x7fffU}
	})
};

constexpr static auto extraBytes
	{substrate::make_array<uint16_t>({2U, 5U, 8U, 15U, 270U})};

constexpr static auto lengthTable
	{substrate::make_array<int8_t>({9, 1, 0, -1, -1, 8, 4, 2, 1, 0})};
constexpr static auto int8Offsets
	{substrate::make_array<int8_t>({11, 4, 7, 0, 1, 31, -1, -1, 0, 31})};
constexpr static auto int16Offsets
	{substrate::make_array<int16_t>({2820, 1792, 287, -1, 31})};

struct decruncher_t
{
private:
	const fixedVector_t<uint8_t> crunchedData;
	std::span<uint8_t> decrunchedData;

	size_t inputOffset{};
	size_t outputOffset{};
	uint16_t workingData{};

	uint16_t unpackMask;

public:
	decruncher_t(const fd_t &file, span<uint8_t> data) : crunchedData{file.length() - 12}, decrunchedData{data}
	{
		if (!crunchedData.valid() ||
			!file.read(crunchedData.data(), crunchedData.size()))
			throw decrunchingError_t{};
	}

	void decrunch()
	{
		inputOffset = crunchedData.size();
		outputOffset = decrunchedData.size();
		workingData = crunchedData[--inputOffset];

		unpackMask = uint16_t(outputOffset);

		decrunchBytes();
	}

private:
	[[nodiscard]] bool getBit()
	{
		uint16_t result = (workingData & 0xffU) << 1U;
		if (!(result & 0xffU))
		{
			result >>= 8U;
			result += (crunchedData[--inputOffset] << 1U);
		}
		workingData &= 0xff00U;
		workingData |= result & 0xffU;
		return result >> 8U;
	}

	[[nodiscard]] uint32_t getBits(uint16_t bits)
	{
		uint32_t data = workingData;
		uint32_t result = 0;

		for (uint16_t bit = 0; bit <= bits; ++bit)
		{
			data &= 0xffU;
			data <<= 1U;

			if (!uint8_t(data))
			{
				const uint8_t byte{crunchedData[--inputOffset]};
				data >>= 8U;
				data += byte << 1U;
			}
			result += result;
			result += data >> 8U;
		}

		workingData &= 0xff00U;
		workingData |= data & 0xffU;
		return result;
	}

	void decrunchBytes()
	{
	}

	void copyBytes()
	{
		uint32_t count = 1;
		if (getBit())
		{
			size_t i = 0;
			for (; i < 5; ++i)
			{
				const auto chunk = chunkTable[i];
				count = getBits(chunk.bits);
				unpackMask = chunk.mask;
				if ((chunk.mask ^ count) & 0xffffU)
					break;
			}
			count += i < 5 ? extraBytes[i] : 1U;
		}

		outputOffset -= count;
		inputOffset -= count;
		std::memcpy(decrunchedData.data() + outputOffset, crunchedData.data() + inputOffset, count);
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
	decruncher.decrunch();
	return true;
}
catch (const decrunchingError_t &error)
{
	console.error(error.what());
	return false;
}
