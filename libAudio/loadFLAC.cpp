// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2009-2023 Rachel Mant <git@dragonmux.network>
#include <array>

#include "flac.hxx"
#include "string.hxx"
#include "oggCommon.hxx"

/*!
 * @internal
 * @file loadFLAC.cpp
 * @brief The implementation of the FLAC decoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2009-2020
 */

using substrate::make_unique_nothrow;

namespace libAudio::flac
{
	/*!
	 * @internal
	 * \c read() is the internal read callback for FLAC file decoding. This prevents
	 * nasty things from happening on Windows thanks to the run-time mess there.
	 * @param buffer The buffer to read into
	 * @param bytes The number of bytes to read into the buffer, given as a pointer
	 * @param ctx Pointer to our internal context for the given FLAC file
	 * @return A status indicating if we had success or not
	 */
	FLAC__StreamDecoderReadStatus read(const FLAC__StreamDecoder *, uint8_t *buffer, size_t *bytes, void *ctx)
	{
		const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
		if (*bytes > 0)
		{
			const bool result = fd.read(buffer, *bytes, *bytes);
			if (!result && !fd.isEOF())
				return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
			if (!result)
				return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}

		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}

	/*!
	 * @internal
	 * \c seek() is the internal seek callback for FLAC file decoding. This prevents
	 * nasty things from happening on Windows thanks to the run-time mess there.
	 * @param offset A 64-bit unsigned integer giving the number of bytes from the beginning
	 *   of the file to seek through
	 * @param ctx Pointer to our internal context for the given FLAC file
	 * @return A status indicating if the seek worked or not
	 */
	FLAC__StreamDecoderSeekStatus seek(const FLAC__StreamDecoder *, uint64_t offset, void *ctx)
	{
		const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
		const auto result = fd.seek(offset, SEEK_SET);
		if (result == -1 || uint64_t(result) != offset)
			return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
	}

	/*!
	 * @internal
	 * \c tell() is the internal read possition callback for FLAC file decoding. This prevents
	 * nasty things from happening on Windows thanks to the run-time mess there.
	 * @param offset A 64-bit unsigned integer returning the number of bytes from the beginning
	 *   of the file at which the read possition is currently at
	 * @param ctx Pointer to our internal context for the given FLAC file
	 * @return A status indicating if we were able to determine the position or not
	 */
	FLAC__StreamDecoderTellStatus tell(const FLAC__StreamDecoder *, uint64_t *offset, void *ctx)
	{
		const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
		const auto pos = fd.tell();
		if (pos == -1)
			return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
		*offset = uint64_t(pos);
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}

	/*!
	 * @internal
	 * \c length() is the internal file length callback for FLAC file decoding. This prevents
	 * nasty things from happening on Windows thanks to the run-time mess there.
	 * @param len A 64-bit unsigned integer returning the length of the file in bytes
	 * @param ctx Pointer to our internal context for the given FLAC file
	 * @return A status indicating if we were able to determine the length or not
	 */
	FLAC__StreamDecoderLengthStatus length(const FLAC__StreamDecoder *, uint64_t *len, void *ctx)
	{
		const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
		const auto length = fd.length();
		if (length == -1)
			return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
		*len = uint64_t(length);
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}

	/*!
	 * @internal
	 * \c eof() is the internal end-of-file callback for FLAC file decoding. This prevents
	 * nasty things from happening on Windows thanks to the run-time mess there.
	 * @param ctx Pointer to our internal context for the given FLAC file
	 * @return A status indicating whether we have reached the end of the file or not
	 */
	int eof(const FLAC__StreamDecoder *, void *ctx)
	{
		const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
		return fd.isEOF() ? 1 : 0;
	}

	/*!
	 * @internal
	 * \c data() is the internal data callback for FLAC file decoding.
	 * @param frame The headers for the current frame of decoded FLAC audio
	 * @param buffers The 32-bit audio buffers decoded for the current \p frame
	 * @param audioFile Pointer to our internal context for the given FLAC file
	 * @return A constant status indicating that it's safe to continue reading the file
	 */
	FLAC__StreamDecoderWriteStatus data(const FLAC__StreamDecoder *, const FLAC__Frame *frame, const int *const buffers[], void *audioFile)
	{
		const flac_t &file = *static_cast<flac_t *>(audioFile);
		auto &ctx = *file.decoderContext();
		int16_t *PCM = reinterpret_cast<int16_t *>(ctx.buffer.get());
		const uint8_t channels = file.fileInfo().channels();
		if (!channels)
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		const uint8_t sampleShift = ctx.sampleShift;
		uint32_t len = frame->header.blocksize;
		if (len > (ctx.bufferLen / channels))
			len = ctx.bufferLen / channels;

		for (uint32_t i = 0; i < len; i++)
		{
			for (uint8_t j = 0; j < channels; j++)
				PCM[(i * channels) + j] = int16_t(buffers[j][i] >> sampleShift);
		}
		ctx.bytesAvail = len * channels * (file.fileInfo().bitsPerSample() / 8U);
		ctx.bytesRemain = ctx.bytesAvail;

		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

	/*!
	 * @internal
	 * \c metadata() is the internal metadata callback for FLAC file decoding.
	 * @param metadata The item of metadata to process
	 * @param audioFile Pointer to our internal context for the given FLAC file
	 */
	void metadata(const FLAC__StreamDecoder *, const FLAC__StreamMetadata *metadata, void *audioFile)
	{
		flac_t &file = *static_cast<flac_t *>(audioFile);
		auto &ctx = *file.decoderContext();
		fileInfo_t &info = file.fileInfo();

		if (metadata->type >= FLAC__METADATA_TYPE_UNDEFINED)
			return;

		switch (metadata->type)
		{
			case FLAC__METADATA_TYPE_STREAMINFO:
			{
				const FLAC__StreamMetadata_StreamInfo &streamInfo = metadata->data.stream_info;
				info.channels(static_cast<uint8_t>(streamInfo.channels));
				info.bitRate(streamInfo.sample_rate);
				info.bitsPerSample(static_cast<uint8_t>(streamInfo.bits_per_sample));
				if (info.bitsPerSample() == 24U)
				{
					info.bitsPerSample(16U);
					ctx.sampleShift = 8;
				}
				else
					ctx.sampleShift = 0;
				ctx.bufferLen = streamInfo.channels * streamInfo.max_blocksize;
				ctx.buffer = make_unique_nothrow<uint8_t []>(ctx.bufferLen * (streamInfo.bits_per_sample / 8));
				info.totalTime(streamInfo.total_samples / streamInfo.sample_rate);
				if (!ExternalPlayback && ctx.buffer != nullptr)
					file.player(make_unique_nothrow<playback_t>(audioFile, audioFillBuffer, ctx.playbackBuffer, 16384U, info));
				break;
			}
			case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			{
				const FLAC__StreamMetadata_VorbisComment &comments = metadata->data.vorbis_comment;
				for (uint32_t i = 0; i < comments.num_comments; ++i)
				{
					const auto comment = reinterpret_cast<const char *>(comments.comments[i].entry);
					if (strncasecmp(comment, "title=", 6) == 0)
						copyComment(info.titlePtr(), comment + 6);
					else if (strncasecmp(comment, "artist=", 7) == 0)
						copyComment(info.artistPtr(), comment + 7);
					else if (strncasecmp(comment, "album=", 6) == 0)
						copyComment(info.albumPtr(), comment + 6);
#ifndef __arm__
					else
					{
						std::unique_ptr<char []> other;
						copyComment(other, comment);
						info.addOtherComment(std::move(other));
					}
#endif
				}
				break;
			}
			default:
			{
				printf("Unused metadata block read\n");
				break;
			}
		}
	}

	/*!
	 * @internal
	 * \c error() is the internal error callback for FLAC file decoding.
	 * @note Implemented as a no-operation due to how the rest of the decoder is structured
	 */
	void error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *) noexcept { }

	constexpr static std::array<char, 4> flacMagic{{'f', 'L', 'a', 'C'}};
	constexpr static std::array<char, 4> oggMagic{{'O', 'g', 'g', 'S'}};
} // namespace libAudio::flac

using namespace libAudio;

flac_t::flac_t(fd_t &&fd, audioModeRead_t) noexcept : audioFile_t{audioType_t::flac, std::move(fd)},
	decoderCtx{make_unique_nothrow<decoderContext_t>()} { }
flac_t::decoderContext_t::decoderContext_t() noexcept : streamDecoder{FLAC__stream_decoder_new()},
	buffer{}, bufferLen{0}, playbackBuffer{}, sampleShift{0}, bytesRemain{0}, bytesAvail{0} { }

/*!
 * Constructs a flac_t using the file given by \c fileName for reading and playback
 * and returns a pointer to the context of the opened file
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
flac_t *flac_t::openR(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<flac_t>(fd_t{fileName, O_RDONLY | O_NOCTTY}, audioModeRead_t{})};
	if (!file || !file->valid() || !isFLAC(file->_fd))
		return nullptr;
	const fd_t &fd = file->fd();
	auto &ctx = *file->decoderContext();

	FLAC__stream_decoder_set_metadata_ignore_all(ctx.streamDecoder);
	FLAC__stream_decoder_set_metadata_respond(ctx.streamDecoder, FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__stream_decoder_set_metadata_respond(ctx.streamDecoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);

	std::array<char, 4> sig{};
	if (!fd.read(sig) || fd.seek(0, SEEK_SET) != 0)
		return nullptr;

	if (memcmp(sig.data(), "OggS", sig.size()) == 0)
		FLAC__stream_decoder_init_ogg_stream(ctx.streamDecoder, flac::read, flac::seek,
			flac::tell, flac::length, flac::eof, flac::data, flac::metadata, flac::error,
			file.get());
	else
		FLAC__stream_decoder_init_stream(ctx.streamDecoder, flac::read, flac::seek, flac::tell,
			flac::length, flac::eof, flac::data, flac::metadata, flac::error, file.get());

	FLAC__stream_decoder_process_until_end_of_metadata(ctx.streamDecoder);
	if (!ctx.buffer) // This indicates that no StreamInfo block was present, rendering the file unplayable
		return nullptr;
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by FLAC_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *flacOpenR(const char *fileName) { return flac_t::openR(fileName); }

bool flac_t::decoderContext_t::finish() noexcept
{
	if (!streamDecoder)
		return true;
	const bool result = !FLAC__stream_decoder_finish(streamDecoder);
	FLAC__stream_decoder_delete(streamDecoder);
	streamDecoder = nullptr;
	return result;
}

flac_t::decoderContext_t::~decoderContext_t() noexcept { finish(); }

FLAC__StreamDecoderState flac_t::decoderContext_t::nextFrame() noexcept
{
	FLAC__stream_decoder_process_single(streamDecoder);
	return FLAC__stream_decoder_get_state(streamDecoder);
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param bufferPtr A pointer to the buffer to be filled
 * @param length An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 */
int64_t flac_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	auto *const buffer = static_cast<char *>(bufferPtr);
	uint32_t filled = 0;
	auto &ctx = *decoderContext();
	while (filled < length)
	{
		if (ctx.bytesRemain == 0)
		{
			const FLAC__StreamDecoderState state = ctx.nextFrame();
			if (state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED)
			{
				ctx.bytesRemain = 0;
				if (filled == 0)
					return -2;
				break;
			}
		}
		uint32_t len = ctx.bytesRemain;
		if (len > (length - filled))
			len = length - filled;
		memcpy(buffer + filled, ctx.buffer.get() + (ctx.bytesAvail - ctx.bytesRemain), len);
		filled += len;
		ctx.bytesRemain -= len;
	}
	return filled;
}

/*!
 * Checks the file given by \p fileName for whether it is an FLAC
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an FLAC file or not
 */
bool isFLAC(const char *fileName) { return flac_t::isFLAC(fileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a FLAC
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a FLAC file or not
 */
bool flac_t::isFLAC(const int32_t fd) noexcept
{
	std::array<char, 4> flacMagic{};
	if (fd == -1 ||
		static_cast<size_t>(read(fd, flacMagic.data(), flacMagic.size())) != flacMagic.size() ||
		lseek(fd, 0, SEEK_SET) != 0)
		return false;
	if (flacMagic == libAudio::flac::oggMagic)
	{
		ogg_packet header;
		return isOgg(fd, header) && ::isFLAC(header);
	}
	return flacMagic == libAudio::flac::flacMagic;
}

/*!
 * Checks the file given by \p fileName for whether it is a FLAC
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a FLAC file or not
 */
bool flac_t::isFLAC(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	return file.valid() && isFLAC(file);
}
