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

	std::unique_ptr<char []> _title{};
	std::unique_ptr<char []> _artist{};
	std::unique_ptr<char []> _album{};
	std::vector<std::unique_ptr<char []>> _other{};

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

	[[nodiscard]] const char *title() const noexcept { return _title.get(); }
	[[nodiscard]] std::unique_ptr<char []> &titlePtr() noexcept { return _title; }
	void title(std::unique_ptr<char []> &&title) noexcept { _title = std::move(title); }
	[[nodiscard]] const char *artist() const noexcept { return _artist.get(); }
	[[nodiscard]] std::unique_ptr<char []> &artistPtr() noexcept { return _artist; }
	void artist(std::unique_ptr<char []> &&artist) noexcept { _artist = std::move(artist); }
	[[nodiscard]] const char *album() const noexcept { return _album.get(); }
	[[nodiscard]] std::unique_ptr<char []> &albumPtr() noexcept { return _album; }
	void album(std::unique_ptr<char []> &&album) noexcept { _album = std::move(album); }

	[[nodiscard]] const std::vector<std::unique_ptr<char []>> &other() const noexcept { return _other; }
	[[nodiscard]] size_t otherCommentsCount() const noexcept { return _other.size(); }
	[[nodiscard]] const char *otherComment(size_t index) const noexcept;
	void addOtherComment(std::unique_ptr<char []> &&comment) noexcept
		{ _other.emplace_back(std::move(comment)); }
};

#endif /*FILE_INFO_HXX*/
