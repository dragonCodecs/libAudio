// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Rachel Mant <git@dragonmux.network>
#include "libAudio.h"
#include "fileInfo.hxx"

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
