// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Rachel Mant <git@dragonmux.network>
#include "libAudio.h"
#include "fileInfo.hxx"

fileInfo_t::fileInfo_t() noexcept = default;
fileInfo_t::fileInfo_t(fileInfo_t &&) noexcept = default;
fileInfo_t &fileInfo_t::operator =(fileInfo_t &&) noexcept = default;
fileInfo_t::~fileInfo_t() noexcept = default;

void fileInfo_t::operator =(const fileInfo_t &info) noexcept
{
	if (this == &info)
		return;
	_totalTime = info._totalTime;
	_bitsPerSample = info._bitsPerSample;
	_bitRate = info._bitRate;
	_channels = info._channels;
}

uint64_t fileInfo_t::totalTime() const noexcept
	{ return _totalTime; }
void fileInfo_t::totalTime(const uint64_t totalTime) noexcept
	{ _totalTime = totalTime; }
uint8_t fileInfo_t::bitsPerSample() const noexcept
	{ return _bitsPerSample; }
void fileInfo_t::bitsPerSample(const uint8_t bitsPerSample) noexcept
	{ _bitsPerSample = bitsPerSample; }
uint32_t fileInfo_t::bitRate() const noexcept
	{ return _bitRate; }
void fileInfo_t::bitRate(const uint32_t bitRate) noexcept
	{ _bitRate = bitRate; }
uint8_t fileInfo_t::channels() const noexcept
	{ return _channels; }
void fileInfo_t::channels(const uint8_t channels) noexcept
	{ _channels = channels; }

const char *fileInfo_t::title() const noexcept
	{ return _title.get(); }
std::unique_ptr<char []> &fileInfo_t::titlePtr() noexcept
	{ return _title; }
void fileInfo_t::title(std::unique_ptr<char []> &&title) noexcept
	{ _title = std::move(title); }
const char *fileInfo_t::artist() const noexcept
	{ return _artist.get(); }
std::unique_ptr<char []> &fileInfo_t::artistPtr() noexcept
	{ return _artist; }
void fileInfo_t::artist(std::unique_ptr<char []> &&artist) noexcept
	{ _artist = std::move(artist); }
const char *fileInfo_t::album() const noexcept
	{ return _album.get(); }
std::unique_ptr<char []> &fileInfo_t::albumPtr() noexcept
	{ return _album; }
void fileInfo_t::album(std::unique_ptr<char []> &&album) noexcept
	{ _album = std::move(album); }

const std::vector<std::unique_ptr<char []>> &fileInfo_t::other() const noexcept
	{ return _other; }
size_t fileInfo_t::otherCommentsCount() const noexcept
	{ return _other.size(); }
void fileInfo_t::addOtherComment(std::unique_ptr<char []> &&comment) noexcept
	{ _other.emplace_back(std::move(comment)); }

const char *fileInfo_t::otherComment(size_t index) const noexcept
{
	if (index >= _other.size())
		return nullptr;
	return _other[index].get();
}

uint64_t audioFileTotalTime(const fileInfo_t *const fileInfo)
{
	if (!fileInfo)
		return 0U;
	return fileInfo->totalTime();
}

uint32_t audioFileBitsPerSample(const fileInfo_t *const fileInfo)
{
	if (!fileInfo)
		return 0U;
	return fileInfo->bitsPerSample();
}

uint32_t audioFileBitRate(const fileInfo_t *const fileInfo)
{
	if (!fileInfo)
		return 0U;
	return fileInfo->bitRate();
}

uint8_t audioFileChannels(const fileInfo_t *const fileInfo)
{
	if (!fileInfo)
		return 0U;
	return fileInfo->channels();
}

const char *audioFileTitle(const fileInfo_t *const fileInfo)
{
	if (!fileInfo)
		return nullptr;
	return fileInfo->title();
}

const char *audioFileArtist(const fileInfo_t *const fileInfo)
{
	if (!fileInfo)
		return nullptr;
	return fileInfo->artist();
}

const char *audioFileAlbum(const fileInfo_t *const fileInfo)
{
	if (!fileInfo)
		return nullptr;
	return fileInfo->album();
}

size_t audioFileOtherCommentsCount(const fileInfo_t *fileInfo)
{
	if (!fileInfo)
		return 0;
	return fileInfo->otherCommentsCount();
}

const char *audioFileOtherComment(const fileInfo_t *fileInfo, size_t index)
{
	if (!fileInfo)
		return nullptr;
	return fileInfo->otherComment(index);
}
