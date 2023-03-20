// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2009-2023 Rachel Mant <git@dragonmux.network>
#include <map>
#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file saveAudio.cpp
 * @brief The implementation of the master encoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2010-2020
 */

const std::map<uint32_t, fileOpenW_t> writers
{
	{AUDIO_OGG_VORBIS, oggVorbisOpenW},
	{AUDIO_OGG_OPUS, oggOpusOpenW},
	{AUDIO_FLAC, flacOpenW},
	{AUDIO_MP4, m4aOpenW},
	{AUDIO_MP3, mp3OpenW},
};

/*!
 * This function opens the file given by \c fileName for writing and returns a pointer
 * to the context of the opened file which must be used only by Audio_* functions
 * @param fileName The name of the file to open
 * @param audioType One of the AUDIO_* constants describing what codec to use for the file
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 * @note Currently only Ogg/Vorbis, FLAC and MP4 are supported. Other formats will be added
 * in following releases of the library.
 */
void *audioOpenW(const char *fileName, uint32_t audioType)
{
	const auto writer{writers.find(audioType)};
	if (writer == writers.end())
		return nullptr;
	return writer->second(fileName);
}

/*!
 * This function sets the \c fileInfo_t structure for an opened file
 * @param audioFile A pointer to a file opened with \c audioOpenW()
 * @param fileInfo A \c fileInfo_t pointer containing various metadata about an opened file
 * @warning This function must be called before using \c audioWriteBuffer()
 */
bool audioSetFileInfo(void *audioFile, const fileInfo_t *const fileInfo)
{
	auto *const file{static_cast<audioFile_t *>(audioFile)};
	if (!file)
		return false;
	return file->fileInfo(*fileInfo);
}

bool audioFileInfo(void *audioFile, const fileInfo_t *const fileInfo)
{
	auto *const file{static_cast<audioFile_t *>(audioFile)};
	if (file && fileInfo)
		return file->fileInfo(*fileInfo);
	return false;
}

bool audioFile_t::fileInfo(const fileInfo_t &) { return false; }

/*!
 * This function writes a buffer of audio to an opened file
 * @param audioFile A pointer to a file opened with \c audioOpenW()
 * @param buffer The buffer of audio to write
 * @param length An integer giving how long the buffer to write is
 * @warning May not work unless \c audioSetFileInfo() has been called beforehand
 */
int64_t audioWriteBuffer(void *audioFile, const void *const buffer, int64_t length)
{
	auto *const file{static_cast<audioFile_t *>(audioFile)};
	if (!file)
		return 0;
	return file->writeBuffer(buffer, length);
}

int64_t audioFile_t::writeBuffer(const void *const, const int64_t) { return 0; }
