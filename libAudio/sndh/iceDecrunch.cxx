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

struct decrunchingError_t : std::exception
{
	const char *what() const noexcept override { return "Error while decrunching file"; }
};

struct byteBlock_t
{
	uint8_t bits;
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

public:
	decruncher_t(const fd_t &file, span<uint8_t> data) : crunchedData
		{
			[&]()
			{
				// Check the file length to make sure it's at least 12 bytes to satisfy the alloc analyser
				const auto length{file.length()};
				if (length < 12)
					throw std::exception{};
				return size_t(length) - 12U;
			}()
		}, decrunchedData{data}
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

		while (outputOffset)
		{
			if (getBit())
				copyBytes();
			if (!outputOffset)
				break;
			unpackFromOffset();
		}

		if (getBit())
			unshuffle(getBit() ? static_cast<uint16_t>(getBits(15U)) : 0x0f9fU);
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
		uint16_t data = workingData;
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
			result <<= 1U;
			result += data >> 8U;
		}

		workingData &= 0xff00U;
		workingData |= data & 0xffU;
		return result;
	}

	void copyBytes()
	{
		uint16_t count = 1;
		if (getBit())
		{
			size_t i = 0;
			for (; i < 5; ++i)
			{
				const auto chunk = chunkTable[i];
				count = static_cast<uint16_t>(getBits(chunk.bits));
				if ((chunk.mask ^ count) & 0xffffU)
					break;
			}
			count += i < 5 ? extraBytes[i] : 1U;
		}

		outputOffset -= count;
		inputOffset -= count;
		std::memcpy(decrunchedData.data() + outputOffset, crunchedData.data() + inputOffset, count);
	}

	void unpackBytes(int32_t offset, const uint32_t count)
	{
		const size_t fromOffset = outputOffset + offset + count;
		const size_t toOffset = outputOffset;
		for (uint32_t i = 1; i <= count; ++i)
			decrunchedData[toOffset - i] = decrunchedData[fromOffset - i];
		outputOffset -= count;
	}

	void unpackFromOffset()
	{
		size_t i = 0;
		for (; i < 4; ++i)
		{
			if (!getBit())
				break;
		}

		uint32_t lengthAdjustment = 0;
		const uint8_t bits = lengthTable[4 - i];
		if (!(bits & 0x80U))
			lengthAdjustment = getBits(bits);

		uint32_t length = lengthTable[9 - i] + lengthAdjustment + 1U;
		const int16_t offset = length > 1 ? offsetFromTable(length - 1U) : offsetFromCalculation();
		unpackBytes(offset, length + 1);
	}

	int16_t offsetFromTable(const uint32_t length)
	{
		size_t i = 0;
		for (; i < 2; ++i)
		{
			if (!getBit())
				break;
		}
		int32_t offset = getBits(int16_t{int8Offsets[2 - i]});
		offset += int16Offsets[4 - i];
		if (offset < 0)
			offset -= length;
		return static_cast<int16_t>(offset);
	}

	int16_t offsetFromCalculation()
	{
		uint8_t bits = 5U;
		int8_t adjustment = -1;
		if (getBit())
		{
			bits = 8U;
			adjustment = 63;
		}
		int32_t offset = getBits(bits);
		return static_cast<int16_t>(offset) + adjustment;
	}

	void unshuffle(uint16_t blocks)
	{
		auto *data = decrunchedData.data() + decrunchedData.size();
		for (uint16_t i = 0; i < blocks; ++i)
		{
			std::array<uint16_t, 4> state{};
			for (size_t j = 0; j < 4; ++j)
			{
				data -= 2;
				auto chunk{uint16_t((data[0] << 8U) | data[1])};

				for (size_t k = 0; k < 4; ++k)
				{
					const size_t shift = ((3U - j) * 4U) + k;
					state[3] |= (chunk & 1U) << shift;
					chunk >>= 1U;
					state[2] |= (chunk & 1U) << shift;
					chunk >>= 1U;
					state[1] |= (chunk & 1U) << shift;
					chunk >>= 1U;
					state[0] |= (chunk & 1U) << shift;
					chunk >>= 1U;
				}
			}

			data[0] = state[0] >> 8U;
			data[1] = uint8_t(state[0]);
			data[2] = state[1] >> 8U;
			data[3] = uint8_t(state[1]);
			data[4] = state[2] >> 8U;
			data[5] = uint8_t(state[2]);
			data[6] = state[3] >> 8U;
			data[7] = uint8_t(state[3]);
		}
	}
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

	_data = {unpackedLength};
	if (!depack(file))
	{
		_data = {};
		throw std::exception{};
	}
}

bool sndhDecruncher_t::depack(const fd_t &file) noexcept try
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
