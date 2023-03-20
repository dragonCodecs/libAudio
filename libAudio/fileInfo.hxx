// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2016-2023 Rachel Mant <git@dragonmux.network>
#ifndef FILE_INFO_HXX
#define FILE_INFO_HXX

#include <cstdint>
#include <memory>
#include <vector>

struct fileInfo_t final
{
	uint64_t totalTime;
	uint32_t bitsPerSample;
	uint32_t bitRate;
	uint8_t channels;
	// int bitStream;
	std::unique_ptr<char []> title;
	std::unique_ptr<char []> artist;
	std::unique_ptr<char []> album;
	std::vector<std::unique_ptr<char []>> other;

	fileInfo_t() noexcept : totalTime{0}, bitsPerSample{0}, bitRate{0},
		channels{0}, title{}, artist{}, album{}, other{} { }
	fileInfo_t(fileInfo_t &&) = default;
	fileInfo_t &operator =(fileInfo_t &&) = default;
	~fileInfo_t() noexcept = default;

	void operator =(const fileInfo_t &info) noexcept
	{
		totalTime = info.totalTime;
		bitsPerSample = info.bitsPerSample;
		bitRate = info.bitRate;
		channels = info.channels;
	}

	fileInfo_t(const fileInfo_t &) = delete;
};

#endif /*FILE_INFO_HXX*/
