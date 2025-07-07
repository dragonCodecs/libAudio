// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2009-2023 Rachel Mant <git@dragonmux.network>
#include <algorithm>
#include <mpc/mpcdec.h>

#include <substrate/utility>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadMPC.cpp
 * @brief The implementation of the MPC decoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2010-2020
 */

using substrate::make_unique_nothrow;

struct mpc_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The MPC demultiplexer handle
	 */
	mpc_demux *demuxer;
	/*!
	 * @internal
	 * The MPC stream information handle which holds the metadata
	 * returned by \c mpc_demux_get_info()
	 */
	mpc_streaminfo streamInfo;
	/*!
	 * @internal
	 * The MPC frame handle which holds, among other things, the decoded
	 * MPC data returned by \c mpc_demux_decode()
	 */
	mpc_frame_info frameInfo;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	/*!
	 * @internal
	 * The count of how much of the current decoded data buffer has been used
	 */
	uint32_t samplesUsed;
	/*!
	 * @internal
	 * The MPC callbacks/reader information handle
	 */
	mpc_reader callbacks;
	/*!
	 * @internal
	 * The decoded MPC buffer filled by \c mpc_demux_decode()
	 */
	MPC_SAMPLE_FORMAT buffer[MPC_DECODER_BUFFER_LENGTH];

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
};

namespace libAudio::mpc
{
	/*!
	 * @internal
	 * This function applies a simple conversion algorithm to convert the input
	 * floating point MPC sample to a short for playback
	 * @param sample The floating point sample to convert
	 * @return The converted floating point sample
	 * @bug This function applies no noise shaping or dithering
	 *   So the output is sub-par to what it could be. FIXME!
	 */
	int16_t floatToInt16(MPC_SAMPLE_FORMAT sample)
	{
		using limits = std::numeric_limits<int16_t>;
		if (sample <= -1.0F)
			return limits::min();
		else if (sample >= 1.0F)
			return limits::max();
		return int16_t(sample * limits::max());
	}

	/*!
	 * @internal
	 * \c read() is the internal read callback for MPC file decoding.
	 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
	 * @param reader Structure holding our own internal context pointer which
	 *   holds the file to seek through
	 * @param buffer The buffer to read into
	 * @param bufferLen The number of bytes to read into the buffer
	 * @return The return result of \c fread()
	 */
	int32_t read(mpc_reader *reader, void *buffer, int bufferLen)
	{
		const auto file = static_cast<mpc_t *>(reader->data);
		return int32_t(file->fd().read(buffer, bufferLen, nullptr));
	}

	/*!
	 * @internal
	 * \c seek() is the internal seek callback for MPC file decoding.
	 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
	 * @param reader Structure holding our own internal context pointer which
	 *   holds the file to seek through
	 * @param offset The offset through the file to which to seek to
	 * @return A truth value giving if the seek succeeded or not
	 */
	uint8_t seek(mpc_reader *reader, int offset)
	{
		const auto file = static_cast<mpc_t *>(reader->data);
		return file->fd().seek(offset, SEEK_SET) == offset;
	}

	/*!
	 * @internal
	 * \c tell() is the internal read possition callback for MPC file decoding.
	 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
	 * @param reader Structure holding our own internal context pointer which
	 *   holds the file to seek through
	 * @return An integer giving the read possition of the file in bytes
	 */
	int32_t tell(mpc_reader *reader)
	{
		const auto file = static_cast<mpc_t *>(reader->data);
		return int32_t(file->fd().tell());
	}

	/*!
	 * @internal
	 * \c length() is the internal file length callback for MPC file decoding.
	 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
	 * @param reader Structure holding our own internal context pointer which
	 *   holds the file to seek through
	 * @return An integer giving the length of the file in bytes
	 */
	int32_t length(mpc_reader *reader)
	{
		const auto file = static_cast<mpc_t *>(reader->data);
		return int32_t(file->fd().length());
	}

	/*!
	 * @internal
	 * \c canSeek() is the internal callback for determining if an MPC file being
	 * decoded can be seeked on or not. \n This does two things: \n
	 * - It prevents nasty things from happening on Windows thanks to the run-time mess there
	 * - It uses \c lseek() as a no-operation to determine if we can seek or not.
	 *
	 * @param reader Structure holding our own internal context pointer which
	 *   holds the file to seek through
	 * @return A truth value giving if seeking can work or not
	 */
	uint8_t canSeek(mpc_reader *reader)
	{
		const auto file = static_cast<mpc_t *>(reader->data);
		return file->fd().tell() != -1;
	}

	constexpr static std::array<char, 3> mpcMagic{{'M', 'P', 'C'}};
	constexpr static std::array<char, 3> mpPlusMagic{{'M', 'P', '+'}};
} // namespace libAudio::mpc

using namespace libAudio;

mpc_t::mpc_t(fd_t &&fd) noexcept : audioFile_t{audioType_t::musePack, std::move(fd)},
	decoderCtx{make_unique_nothrow<decoderContext_t>()} { }
mpc_t::decoderContext_t::decoderContext_t() noexcept : demuxer{nullptr}, streamInfo{}, frameInfo{}, playbackBuffer{},
	samplesUsed{0}, callbacks{mpc::read, mpc::seek, mpc::tell, mpc::length, mpc::canSeek, nullptr} { }

/*!
 * Constructs a mpc_t using the file given by \c fileName for reading and playback
 * and returns a pointer to the context of the opened file
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
mpc_t *mpc_t::openR(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<mpc_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isMPC(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	ctx.callbacks.data = file.get();
	ctx.demuxer = mpc_demux_init(&ctx.callbacks);
	ctx.frameInfo.buffer = ctx.buffer;
	mpc_demux_get_info(ctx.demuxer, &ctx.streamInfo);

	info.bitsPerSample(ctx.streamInfo.bitrate == 0U ? 16U : static_cast<uint8_t>(ctx.streamInfo.bitrate));
	info.bitRate(ctx.streamInfo.sample_freq);
	info.channels(static_cast<uint8_t>(ctx.streamInfo.channels));
	info.totalTime(ctx.streamInfo.samples / info.bitRate());

	if (!ExternalPlayback)
		file->player(make_unique_nothrow<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192U, info));
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by MPC_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *mpcOpenR(const char *fileName) { return mpc_t::openR(fileName); }

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param bufferPtr A pointer to the buffer to be filled
 * @param length An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 */
int64_t mpc_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	const auto buffer = static_cast<uint8_t *>(bufferPtr);
	uint32_t offset = 0;
	const fileInfo_t &info = fileInfo();
	auto &ctx = *context();

	while (offset < length)
	{
		if (ctx.samplesUsed == 0)
		{
			if (mpc_demux_decode(ctx.demuxer, &ctx.frameInfo) != 0 || ctx.frameInfo.bits == -1)
				return -2;
		}

		int16_t *playbackBuffer = reinterpret_cast<int16_t *>(ctx.playbackBuffer + offset);
		uint32_t count = 0;
		const uint32_t sampleOffset = ctx.samplesUsed * info.channels();
		for (uint32_t index = 0, i = ctx.samplesUsed; i < ctx.frameInfo.samples; ++i)
		{
			playbackBuffer[index] = mpc::floatToInt16(ctx.frameInfo.buffer[sampleOffset + index]);
			++index;
			if (info.channels() == 2)
			{
				playbackBuffer[index] = mpc::floatToInt16(ctx.frameInfo.buffer[sampleOffset + index]);
				++index;
			}
			count += sizeof(int16_t) * info.channels();

			if ((offset + count) >= length)
			{
				ctx.samplesUsed = ++i;
				break;
			}
		}
		if ((offset + count) < length)
			ctx.samplesUsed = 0;

		if (buffer != ctx.playbackBuffer)
			memcpy(buffer + offset, playbackBuffer, count);
		offset += count;
	}

	return offset;
}

mpc_t::decoderContext_t::~decoderContext_t() noexcept
	{ mpc_demux_exit(demuxer); }

/*!
 * Checks the file given by \p fileName for whether it is an MPC
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an MPC file or not
 */
bool isMPC(const char *fileName) { return mpc_t::isMPC(fileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a MPC
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a MPC file or not
 */
bool mpc_t::isMPC(const int32_t fd) noexcept
{
	std::array<char, 3> mpcMagic{};
	return
		fd != -1 &&
		static_cast<size_t>(read(fd, mpcMagic.data(), mpcMagic.size())) == mpcMagic.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		(mpcMagic == libAudio::mpc::mpPlusMagic || mpcMagic == libAudio::mpc::mpcMagic);
}

/*!
 * Checks the file given by \p fileName for whether it is a MPC
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a MPC file or not
 */
bool mpc_t::isMPC(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	return file.valid() && isMPC(file);
}
