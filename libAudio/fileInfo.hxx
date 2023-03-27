// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2016-2023 Rachel Mant <git@dragonmux.network>
#ifndef FILE_INFO_HXX
#define FILE_INFO_HXX

#include <cstdint>
#include <memory>
#include <vector>
#include "libAudio.h"

struct libAUDIO_CLS_API fileInfo_t final
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
	fileInfo_t() noexcept;
	fileInfo_t(fileInfo_t &&) noexcept;
	fileInfo_t &operator =(fileInfo_t &&) noexcept;
	~fileInfo_t() noexcept;

	void operator =(const fileInfo_t &info) noexcept;

	fileInfo_t(const fileInfo_t &) = delete;

	[[nodiscard]] uint64_t totalTime() const noexcept;
	void totalTime(uint64_t totalTime) noexcept;
	[[nodiscard]] uint32_t bitsPerSample() const noexcept;
	void bitsPerSample(uint32_t bitsPerSample) noexcept;
	[[nodiscard]] uint32_t bitRate() const noexcept;
	void bitRate(uint32_t bitRate) noexcept;
	[[nodiscard]] uint8_t channels() const noexcept;
	void channels(uint8_t channels) noexcept;

	[[nodiscard]] const char *title() const noexcept;
	[[nodiscard]] std::unique_ptr<char []> &titlePtr() noexcept;
	void title(std::unique_ptr<char []> &&title) noexcept;
	[[nodiscard]] const char *artist() const noexcept;
	[[nodiscard]] std::unique_ptr<char []> &artistPtr() noexcept;
	void artist(std::unique_ptr<char []> &&artist) noexcept;
	[[nodiscard]] const char *album() const noexcept;
	[[nodiscard]] std::unique_ptr<char []> &albumPtr() noexcept;
	void album(std::unique_ptr<char []> &&album) noexcept;

	[[nodiscard]] const std::vector<std::unique_ptr<char []>> &other() const noexcept;
	[[nodiscard]] size_t otherCommentsCount() const noexcept;
	[[nodiscard]] const char *otherComment(size_t index) const noexcept;
	void addOtherComment(std::unique_ptr<char []> &&comment) noexcept;
};

#endif /*FILE_INFO_HXX*/
