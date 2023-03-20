// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2022-2023 Rachel Mant <git@dragonmux.network>
#include "mp3.hxx"

/*!
 * @internal
 * @file saveMP3.cxx
 * @brief The implementation of the MP3 encoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2022
 */

mp3_t::mp3_t(fd_t &&fd, audioModeWrite_t) noexcept : audioFile_t{audioType_t::mp3, std::move(fd)},
	encoderCtx{makeUnique<encoderContext_t>()} { }
mp3_t::encoderContext_t::encoderContext_t() noexcept : encoder{lame_init()}, lameFrameOffset{}
{
	lame_set_brate(encoder, 320);
}

mp3_t *mp3_t::openW(const char *const fileName) noexcept
{
	auto file{makeUnique<mp3_t>(fd_t{fileName, O_RDWR | O_CREAT | O_TRUNC, substrate::normalMode},
		audioModeWrite_t{})};
	if (!file || !file->valid())
		return nullptr;
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for writing and returns a pointer
 * to the context of the opened file which must be used only by MP3_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *mp3OpenW(const char *fileName) { return mp3_t::openW(fileName); }

/*!
 * This function sets the \c fileInfo_t structure for a MP3 file being encoded
 * @param info A \c fileInfo_t pointer containing various metadata about an opened file
 * @warning This function must be called before using \c mp3_t::writeBuffer() or \c audioWriteBuffer()
 */
bool mp3_t::fileInfo(const fileInfo_t &info)
{
	const auto &file{fd()};
	auto &ctx = *encoderContext();
	lame_set_num_channels(ctx.encoder, info.channels);
	lame_set_in_samplerate(ctx.encoder, static_cast<int>(info.bitRate));
	if (info.channels == 1)
		lame_set_mode(ctx.encoder, MPEG_mode::MONO);
	else
		lame_set_mode(ctx.encoder, MPEG_mode::JOINT_STEREO);

	id3tag_init(ctx.encoder);
	if (info.title)
		id3tag_set_title(ctx.encoder, info.title.get());
	if (info.artist)
		id3tag_set_artist(ctx.encoder, info.artist.get());
	if (info.album)
		id3tag_set_album(ctx.encoder, info.album.get());
	for (const auto &comment : info.other)
		id3tag_set_comment(ctx.encoder, comment.get());
	id3tag_add_v2(ctx.encoder);
	lame_init_params(ctx.encoder);
	const auto id3v2Length{lame_get_id3v2_tag(ctx.encoder, nullptr, 0)};
	auto id3v2Data{substrate::make_unique<unsigned char []>(id3v2Length)};
	lame_get_id3v2_tag(ctx.encoder, id3v2Data.get(), id3v2Length);
	if (!file.write(id3v2Data, id3v2Length))
		return false;
	ctx.lameFrameOffset = static_cast<substrate::off_t>(id3v2Length);
	fileInfo() = info;
	return true;
}

/*!
 * This function writes a buffer of audio to a MP3 file opened being encoded
 * @param bufferPtr The buffer of audio to write
 * @param length An int64_t giving how long the buffer to write is
 * @attention Will not work unless \c mp3_t::fileInfo() or \c audioFileInfo() has been called beforehand
 */
int64_t mp3_t::writeBuffer(const void *const bufferPtr, const int64_t length)
{
	const fd_t &file{fd()};
	const fileInfo_t &info = fileInfo();
	auto &ctx = *encoderContext();
	if (length <= 0)
	{
		int32_t result{};
		std::array<unsigned char, 7200> encoderBuffer{};
		do
		{
			result = lame_encode_flush(ctx.encoder, encoderBuffer.data(), encoderBuffer.size());
			if (result > 0 && !file.write(encoderBuffer.data(), result))
				break;
		}
		while (result > 0);
		result = lame_get_lametag_frame(ctx.encoder, encoderBuffer.data(), encoderBuffer.size());
		if (file.seek(ctx.lameFrameOffset, SEEK_SET) == ctx.lameFrameOffset)
			[[maybe_unused]] const auto _{file.write(encoderBuffer.data(), result)};
		return length;
	}
	else if (info.bitsPerSample != 16)
		return -3; // LAME can't encode non-16-bit sample data.. V_V
	const auto sampleCount{uint64_t(length) / (uint64_t(info.channels) * (info.bitsPerSample / 8U))};
	const auto *const sampleBuffer{static_cast<const short *>(bufferPtr)};
	const auto encodedLength{sampleCount + (sampleCount / 4U) + 7200U};
	auto encoderBuffer{substrate::make_unique<unsigned char []>(encodedLength)};
	const auto result{lame_encode_buffer_interleaved(ctx.encoder, const_cast<short *>(sampleBuffer), sampleCount,
		encoderBuffer.get(), encodedLength)};
	if (result >= 0)
	{
		if (!file.write(encoderBuffer, result))
			return -1;
	}
	else
		return result;
	return length;
}

mp3_t::encoderContext_t::~encoderContext_t() noexcept
{
	lame_close(encoder);
}
