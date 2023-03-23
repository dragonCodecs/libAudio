// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2016-2023 Rachel Mant <git@dragonmux.network>
#ifndef FILE_INFO_HXX
#define FILE_INFO_HXX

#include <cstdint>
#include <memory>
#include <vector>

struct fileInfo_t final
{
private:
	uint64_t _totalTime{0};
	uint32_t _bitsPerSample{0};
	uint32_t _bitRate{0};
	uint8_t _channels{0};

public:
	std::unique_ptr<char []> title{};
	std::unique_ptr<char []> artist{};
	std::unique_ptr<char []> album{};
	std::vector<std::unique_ptr<char []>> other{};

public:
	fileInfo_t() noexcept = default;
	fileInfo_t(fileInfo_t &&) = default;
	fileInfo_t &operator =(fileInfo_t &&) = default;
	~fileInfo_t() noexcept = default;

	void operator =(const fileInfo_t &info) noexcept
	{
		if (this == &info)
			return;
		_totalTime = info._totalTime;
		_bitsPerSample = info._bitsPerSample;
		_bitRate = info._bitRate;
		_channels = info._channels;
	}

	fileInfo_t(const fileInfo_t &) = delete;

	[[nodiscard]] uint64_t totalTime() const noexcept { return _totalTime; }
	void totalTime(const uint64_t totalTime) noexcept { _totalTime = totalTime; }
	[[nodiscard]] uint32_t bitsPerSample() const noexcept { return _bitsPerSample; }
	void bitsPerSample(const uint32_t bitsPerSample) noexcept { _bitsPerSample = bitsPerSample; }
	[[nodiscard]] uint32_t bitRate() const noexcept { return _bitRate; }
	void bitRate(const uint32_t bitRate) noexcept { _bitRate = bitRate; }
	[[nodiscard]] uint8_t channels() const noexcept { return _channels; }
	void channels(const uint8_t channels) noexcept { _channels = channels; }
};

#endif /*FILE_INFO_HXX*/
