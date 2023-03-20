// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2009-2023 Rachel Mant <git@dragonmux.network>
#include "flac.hxx"

/*!
 * @internal
 * @file saveFLAC.cpp
 * @brief The implementation of the FLAC encoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2010-2019
 */

namespace libAudio
{
	namespace flac
	{
		/*!
		* @internal
		* \c write() is the internal write callback for FLAC file creation. This prevents
		* nasty things from happening on Windows thanks to the run-time mess there.
		* @param buffer The buffer to write which also must not become modified
		* @param bytes A count holding the number of bytes in \p buffer to write
		* @param ctx Our own internal context pointer which holds the file to write to
		*/
		FLAC__StreamEncoderWriteStatus write(const FLAC__StreamEncoder *, const uint8_t *buffer, size_t bytes, uint32_t, uint32_t, void *ctx)
		{
			const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
			if (bytes > 0)
			{
				const auto result = fd.write(buffer, bytes, nullptr);
				if (result == -1)
					return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
				else if (size_t(result) == bytes)
					return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
			}
			return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
		}

		/*!
		* @internal
		* \c seek() is the internal seek callback for FLAC file creation. This prevents
		* nasty things from happening on Windows thanks to the run-time mess there.
		* @param offset The offset through the file to which to seek to
		* @param ctx Our own internal context pointer which holds the file to seek through
		*/
		FLAC__StreamEncoderSeekStatus seek(const FLAC__StreamEncoder *, uint64_t offset, void *ctx)
		{
			const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
			const auto result = fd.seek(offset, SEEK_SET);
			if (result == -1 || uint64_t(result) != offset)
				return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
			return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
		}

		/*!
		* @internal
		* \c tell() is the internal seek callback for FLAC file creation. This prevents
		* nasty things from happening on Windows thanks to the run-time mess there.
		* @param offset The returned offset location into the file
		* @param ctx Our own internal context pointer which holds the file to get
		*   the write pointer position of
		*/
		FLAC__StreamEncoderTellStatus tell(const FLAC__StreamEncoder *, uint64_t *offset, void *ctx)
		{
			const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
			const auto pos = fd.tell();
			if (pos == -1)
				return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
			*offset = pos;
			return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
		}
	}
}

using namespace libAudio;

flac_t::flac_t(fd_t &&fd, audioModeWrite_t) noexcept : audioFile_t{audioType_t::flac, std::move(fd)},
	encoderCtx{makeUnique<encoderContext_t>()} { }
flac_t::encoderContext_t::encoderContext_t() noexcept : streamEncoder{FLAC__stream_encoder_new()},
	encoderBuffer{}, metadata{}
{
	FLAC__stream_encoder_set_compression_level(streamEncoder, 4);
	//FLAC__stream_encoder_set_loose_mid_side_stereo(streamEncoder, true);
}

flac_t *flac_t::openW(const char *const fileName) noexcept
{
	auto file = makeUnique<flac_t>(fd_t{fileName, O_RDWR | O_CREAT | O_TRUNC, substrate::normalMode},
		audioModeWrite_t{});
	if (!file || !file->valid())
		return nullptr;
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for writing and returns a pointer
 * to the context of the opened file which must be used only by FLAC_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *flacOpenW(const char *fileName) { return flac_t::openW(fileName); }

void writeComment(FLAC__StreamMetadata *metadata, const char *const name, const char *const value)
{
	if (!value)
		return;
	FLAC__StreamMetadata_VorbisComment_Entry entry{};
	FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, name, value);
	FLAC__metadata_object_vorbiscomment_append_comment(metadata, entry, false);
}

void writeComment(FLAC__StreamMetadata *metadata, const char *const name, const std::unique_ptr<char []> &value)
	{ writeComment(metadata, name, value.get()); }

/*!
 * This function sets the \c FileInfo structure for a FLAC file being encoded
 * @param flacFile A pointer to a file opened with \c flacOpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file
 * @warning This function must be called before using \c FLAC_WriteBuffer()
 */
bool flac_t::fileInfo(const fileInfo_t &info)
{
	auto &ctx = *encoderContext();
	FLAC__StreamMetadata_VorbisComment_Entry entry;

	FLAC__stream_encoder_set_channels(ctx.streamEncoder, info.channels);
	FLAC__stream_encoder_set_bits_per_sample(ctx.streamEncoder, info.bitsPerSample);
	FLAC__stream_encoder_set_sample_rate(ctx.streamEncoder, info.bitRate);

	ctx.metadata = {
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT),
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)
	};
	for (const auto &comment : info.other)
	{
		entry.entry = reinterpret_cast<uint8_t *>(comment.get());
		entry.length = uint32_t(strlen(comment.get()));
		FLAC__metadata_object_vorbiscomment_append_comment(ctx.metadata[0], entry, true);
	}

	writeComment(ctx.metadata[0], "Album", info.album);
	writeComment(ctx.metadata[0], "Artist", info.artist);
	writeComment(ctx.metadata[0], "Title", info.title);
	FLAC__stream_encoder_set_metadata(ctx.streamEncoder, ctx.metadata.data(), uint32_t(ctx.metadata.size()));
	FLAC__stream_encoder_init_stream(ctx.streamEncoder, flac::write, flac::seek,
		flac::tell, nullptr, this);
	fileInfo() = info;
	return true;
}

void flac_t::encoderContext_t::fillFrame(const int8_t *const buffer, const uint32_t samples) noexcept
{
	for (uint32_t i = 0; i < samples; ++i)
		encoderBuffer[i] = buffer[i];
}

void flac_t::encoderContext_t::fillFrame(const int16_t *const buffer, const uint32_t samples) noexcept
{
	for (uint32_t i = 0; i < samples; ++i)
		encoderBuffer[i] = buffer[i];
}

/*!
 * This function writes a buffer of audio to a FLAC file opened being encoded
 * @param flacFile A pointer to a file opened with \c flacOpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @attention Will not work unless \c FLAC_SetFileInfo() has been called beforehand
 */
int64_t flac_t::writeBuffer(const void *const bufferPtr, const int64_t rawLength)
{
	const auto buffer = static_cast<const char *>(bufferPtr);
	uint32_t offset = 0;
	auto &ctx = *encoderContext();
	const fileInfo_t &info = fileInfo();

	if (rawLength <= 0 || FLAC__stream_encoder_get_state(ctx.streamEncoder) != FLAC__STREAM_ENCODER_OK)
		return -2;
	const uint32_t length = uint32_t(rawLength);
	while (offset < length)
	{
		const uint32_t samplesMax = (length - offset) / sizeof(int16_t);
		const uint32_t sampleCount = std::min(uint32_t(ctx.encoderBuffer.size()), samplesMax);
		const auto samples = buffer + offset;
		const uint8_t bytesPerSample = info.bitsPerSample / 8;

		if (info.bitsPerSample == 8)
			ctx.fillFrame(reinterpret_cast<const int8_t *>(samples), sampleCount);
		else
			ctx.fillFrame(reinterpret_cast<const int16_t *>(samples), sampleCount);
		offset += sampleCount * bytesPerSample;

		if (!FLAC__stream_encoder_process_interleaved(ctx.streamEncoder,
			ctx.encoderBuffer.data(), sampleCount / info.channels))
			break;
	}

	return offset;
}

bool flac_t::encoderContext_t::finish() noexcept
{
	if (!streamEncoder)
		return true;
	const bool result = !FLAC__stream_encoder_finish(streamEncoder);
	FLAC__stream_encoder_delete(streamEncoder);
	for (auto data : metadata)
		FLAC__metadata_object_delete(data);
	streamEncoder = nullptr;
	return result;
}

flac_t::encoderContext_t::~encoderContext_t() noexcept { finish(); }
