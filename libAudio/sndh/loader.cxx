// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2023 Rachel Mant <git@dragonmux.network>
#include <cstring>
#include <string>
#include <type_traits>
#include "loader.hxx"
#include "atariASCII.hxx"
#include "../conversions.hxx"
#include "../string.hxx"

using namespace libAudio::conversions;

constexpr std::array<char, 4> typeHeader{'S', 'N', 'D', 'H'};
constexpr std::array<char, 4> typeTitle{'T', 'I', 'T', 'L'};
constexpr std::array<char, 4> typeComposer{'C', 'O', 'M', 'M'};
constexpr std::array<char, 4> typeRipper{'R', 'I', 'P', 'P'};
constexpr std::array<char, 4> typeConverter{'C', 'O', 'N', 'V'};
constexpr std::array<char, 4> typeTuneNames{'#', '!', 'S', 'N'};
constexpr std::array<char, 2> typeTuneCount{'#', '#'};
constexpr std::array<char, 2> typeTimerA{'T', 'A'};
constexpr std::array<char, 2> typeTimerB{'T', 'B'};
constexpr std::array<char, 2> typeTimerC{'T', 'C'};
constexpr std::array<char, 2> typeTimerD{'T', 'D'};
constexpr std::array<char, 2> typeTimerVBL{'!', 'V'};
constexpr std::array<char, 4> typeYear{'Y', 'E', 'A', 'R'};
constexpr std::array<char, 4> typeTime{'T', 'I', 'M', 'E'};
constexpr std::array<char, 4> typeEnd{'H', 'D', 'N', 'S'};

template<typename T, size_t sizeA, size_t sizeB> std::enable_if_t<sizeB < sizeA, bool>
	operator ==(const std::array<T, sizeA> &a, const std::array<T, sizeB> &b) noexcept
	{ return std::equal(b.begin(), b.end(), a.begin()); }

template<typename T, size_t sizeA, size_t sizeB> std::enable_if_t<sizeA < sizeB, bool>
	operator ==(const std::array<T, sizeA> &a, const std::array<T, sizeB> &b) noexcept
	{ return std::equal(a.begin(), a.end(), b.begin()); }

sndhLoader_t::sndhLoader_t(const fd_t &file) : _data{file}, _entryPoints{}, _metadata{}
{
	std::array<char, 4> magic{};
	if (!_data.readBE(_entryPoints.init) ||
		!_data.readBE(_entryPoints.exit) ||
		!_data.readBE(_entryPoints.play) ||
		!_data.read(magic) ||
		magic != typeHeader ||
		!readMeta())
		throw std::exception{};
}

std::string readString(sndhDecruncher_t &file)
{
	std::string result{};
	uint8_t value{255U};
	while (value != 0U)
	{
		if (!file.read(value))
			throw std::exception{};
		result += atariChars[value];
	}
	// If there are trailing NUL's after the string, chew through them
	while (file.peak() == '\0')
	{
		if (!file.read(value))
			throw std::exception{};
	}
	return result;
}

void readString(sndhDecruncher_t &file, std::unique_ptr<char []> &dst)
{
	const std::string result{readString(file)};
	copyComment(dst, result.data());
}

uint16_t readFrequency(sndhDecruncher_t &file, const char *const prefix)
{
	if (!prefix[0] || !isNumber(prefix[0]))
		throw std::exception{};
	uint16_t result{uint8_t(prefix[0] - '0')};
	if (!prefix[1])
		return result;
	else if (!isNumber(prefix[1]))
		throw std::exception{};
	result *= 10;
	result += prefix[1] - '0';

	char value{-1};
	while (value != 0)
	{
		if (!file.read(value) || (value && !isNumber(value)))
			throw std::exception{};
		else if (value)
		{
			result *= 10;
			result += value - '0';
		}
	}
	return result;
}

bool sndhLoader_t::readMeta()
{
	std::array<char, 4U> tagType{};
	while (tagType != typeEnd)
	{
		if (!_data.read(tagType))
			return false;
		else if (tagType == typeTitle)
			readString(_data, _metadata.title);
		else if (tagType == typeComposer)
			readString(_data, _metadata.artist);
		else if (tagType == typeRipper)
			readString(_data, _metadata.ripper);
		else if (tagType == typeConverter)
			readString(_data, _metadata.converter);
		else if (tagType == typeTuneCount)
		{
			auto count = readString(_data);
			count.insert(0U, tagType.data() + 2, 2U);
			_metadata.tuneCount = toInt_t<uint8_t>{count.c_str()}.fromInt();
			_metadata.tuneTimes = {_metadata.tuneCount};
			_metadata.tuneNames = {_metadata.tuneCount};
		}
		else if (tagType == typeTuneNames)
		{
			if (!_metadata.tuneTimes.valid())
				return false;
			for (auto &tuneName: _metadata.tuneNames)
				readString(_data, tuneName);
		}
		else if (tagType == typeTimerA || tagType == typeTimerB || tagType == typeTimerC ||
			tagType == typeTimerD || tagType == typeTimerVBL)
		{
			_metadata.timer = tagType[1];
			_metadata.timerFrequency = readFrequency(_data, tagType.data() + 2);
		}
		else if (tagType == typeYear)
		{
			std::string year{readString(_data)};
			_metadata.year = toInt_t<uint32_t>{year.data()}.fromInt();
		}
		else if (tagType == typeTime)
		{
			if (!_metadata.tuneTimes.valid())
				return false;
			for (auto &time : _metadata.tuneTimes)
			{
				if (!_data.readBE(time))
					return false;
			}
		}
		// If none of the above tags matched, we're in a SNDH v1.1 file and came to the end of the header
		else if (tagType != typeEnd)
			// Put the non-header data back and get out of here
			return _data.seekRel(-4);
	}
	return true;
}

// Defer copying the SNDH data to the Atari ST's RAM to the emulation itself
bool sndhLoader_t::copyToRAM(atariSTe_t &emulator) noexcept
	{ return emulator.copyToRAM(_data); }
