// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2021-2023 Rachel Mant <git@dragonmux.network>
#include <random>
#include <limits>
#include "oggOpus.hxx"

/*!
 * @internal
 * @file saveOggVorbis.cpp
 * @brief The implementation of the Ogg|Opus encoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2021
 */

using substrate::make_unique_nothrow;

namespace libAudio
{
	namespace oggOpus
	{
		int32_t write(void *const filePtr, const uint8_t *buffer, int32_t bufferLen)
		{
			auto *const file{static_cast<const oggOpus_t *>(filePtr)};
			return file->fd().write(buffer, bufferLen) ? 0 : 1;
		}

		int32_t close(void *const) { return 0; }

		constexpr static OpusEncCallbacks callbacks
		{
			write,
			close
		};

		OggOpusComments *copyComments(const fileInfo_t &info) noexcept
		{
			OggOpusComments *tags{ope_comments_create()};
			if (info.title)
				ope_comments_add(tags, "Title", info.title.get());
			if (info.artist)
				ope_comments_add(tags, "Artist", info.artist.get());
			if (info.album)
				ope_comments_add(tags, "Album", info.album.get());
			for (const auto &other : info.other)
				ope_comments_add_string(tags, other.get());
			return tags;
		}
	} // namespace oggOpus
} // namespace libAudio

using namespace libAudio;

oggOpus_t::oggOpus_t(fd_t &&fd, audioModeWrite_t) noexcept :
	audioFile_t{audioType_t::oggOpus, std::move(fd)},
	encoderCtx{make_unique_nothrow<encoderContext_t>()} { }
oggOpus_t::encoderContext_t::encoderContext_t() noexcept : encoder{} { }

oggOpus_t *oggOpus_t::openW(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<oggOpus_t>(fd_t{fileName, O_RDWR | O_CREAT | O_TRUNC, substrate::normalMode},
		audioModeWrite_t{})};
	if (!file || !file->valid())
		return nullptr;
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for writing and returns a pointer
 * to the context of the opened file which must be used only by OggVorbis_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *oggOpusOpenW(const char *fileName) { return oggOpus_t::openW(fileName); }

/*!
 * This function sets the \c FileInfo structure for a Ogg/Vorbis file being encoded
 * @param p_VorbisFile A pointer to a file opened with \c oggOpusOpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file
 * @warning This function must be called before using \c OggVorbis_WriteBuffer()
 */
bool oggOpus_t::fileInfo(const fileInfo_t &info)
{
	auto &ctx = *encoderContext();
	auto *tags{oggOpus::copyComments(info)};
	if (!tags)
		return false;
	int result{};

	ctx.encoder = ope_encoder_create_callbacks(&oggOpus::callbacks, this, tags, info.bitRate,
		info.channels, 0, &result);
	// the above call takes a copy, so we have to clean up
	ope_comments_destroy(tags);
	if (result != OPE_OK)
	{
		if (ctx.encoder)
			ope_encoder_destroy(ctx.encoder);
		ctx.encoder = nullptr;
		return false;
	}
	fileInfo() = info;
	return true;
}

/*!
 * This function writes a buffer of audio to a Ogg/Vorbis file opened being encoded
 * @param p_VorbisFile A pointer to a file opened with \c oggOpusOpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @attention Will not work unless \c OggVorbis_SetFileInfo() has been called beforehand
 */
int64_t oggOpus_t::writeBuffer(const void *const bufferPtr, const int64_t length)
{
	const fileInfo_t &info = fileInfo();
	auto &ctx = *encoderContext();
	if (length <= 0)
		return length;
	else if (info.bitsPerSample != 16)
		return -3; // Opus can't encode non-16-bit sample data.. V_V
	// Convert length into samples per channel
	const auto sampleCount{uint64_t(length) / (uint64_t(info.channels) * (info.bitsPerSample / 8U))};
	const auto *const buffer{static_cast<const int16_t *>(bufferPtr)};
	const auto result{ope_encoder_write(ctx.encoder, buffer, sampleCount)};
	if (result == OPE_OK)
		return length;
	return -1;
}

oggOpus_t::encoderContext_t::~encoderContext_t() noexcept
{
	if (encoder)
	{
		ope_encoder_drain(encoder);
		ope_encoder_destroy(encoder);
	}
}
