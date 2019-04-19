#include "flac.hxx"

/*!
 * @internal
 * @file saveFLAC.cpp
 * @brief The implementation of the FLAC encoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
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
		* @param p_enc The encoding context which must not be modified by the function
		* @param buffer The buffer to write which also must not become modified
		* @param nBytes A count holding the number of bytes in \p buffer to write
		* @param nSamp A count holding the number of samples encoded in buffer,
		*   or 0 to indicate metadata is being written
		* @param nCurrFrame If \p nSamp is non-zero, this olds the number of the current
		*   frame being encoded
		* @param p_FLACFile Our own internal context pointer which holds the file to write to
		*/
		FLAC__StreamEncoderWriteStatus write(const FLAC__StreamEncoder *, const uint8_t *buffer, size_t bytes, uint32_t, uint32_t, void *ctx)
		{
			const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
			if (bytes > 0)
			{
				const ssize_t result = fd.write(buffer, bytes);
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
		* @param p_enc The encoding context which must not be modified by the function
		* @param offset The offset through the file to which to seek to
		* @param p_FLACFile Our own internal context pointer which holds the file to seek through
		*/
		FLAC__StreamEncoderSeekStatus seek(const FLAC__StreamEncoder *, uint64_t offset, void *ctx)
		{
			const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
			const off_t result = fd.seek(offset, SEEK_SET);
			if (result == -1 || uint64_t(result) != offset)
				return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
			return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
		}

		/*!
		* @internal
		* \c tell() is the internal seek callback for FLAC file creation. This prevents
		* nasty things from happening on Windows thanks to the run-time mess there.
		* @param p_enc The encoding context which must not be modified by the function
		* @param offset The returned offset location into the file
		* @param p_FLACFile Our own internal context pointer which holds the file to get
		*   the write pointer position of
		*/
		FLAC__StreamEncoderTellStatus tell(const FLAC__StreamEncoder *, uint64_t *offset, void *ctx)
		{
			const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
			const off_t pos = fd.tell();
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
	auto flacFile = makeUnique<flac_t>(fd_t(fileName, O_RDWR | O_CREAT | O_TRUNC, normalMode),
		audioModeWrite_t{});
	if (!flacFile || !flacFile->valid())
		return nullptr;
	return flacFile.release();
}

/*!
 * This function opens the file given by \c FileName for writing and returns a pointer
 * to the context of the opened file which must be used only by FLAC_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *FLAC_OpenW(const char *FileName) { return flac_t::openW(FileName); }

/*!
 * This function sets the \c FileInfo structure for a FLAC file being encoded
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file
 * @warning This function must be called before using \c FLAC_WriteBuffer()
 * @bug p_FI must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug \p p_FLACFile must not be \c NULL as no checking on the parameter is done. FIXME!
 */
bool FLAC_SetFileInfo(void *p_FLACFile, FileInfo *p_FI) { return audioFileInfo(p_FLACFile, p_FI); }

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

bool flac_t::fileInfo(const FileInfo &fileInfo)
{
	auto &ctx = *encoderContext();
	fileInfo_t &info = this->fileInfo();
	FLAC__StreamMetadata_VorbisComment_Entry entry;

	FLAC__stream_encoder_set_channels(ctx.streamEncoder, fileInfo.Channels);
	FLAC__stream_encoder_set_bits_per_sample(ctx.streamEncoder, fileInfo.BitsPerSample);
	FLAC__stream_encoder_set_sample_rate(ctx.streamEncoder, fileInfo.BitRate);

	ctx.metadata = {
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT),
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)
	};
	for (uint32_t i = 0; i < fileInfo.nOtherComments; i++)
	{
		entry.entry = (uint8_t *)fileInfo.OtherComments[i];
		entry.length = strlen(fileInfo.OtherComments[i]);
		FLAC__metadata_object_vorbiscomment_append_comment(ctx.metadata[0], entry, true);
	}

	writeComment(ctx.metadata[0], "Album", fileInfo.Album);
	writeComment(ctx.metadata[0], "Artist", fileInfo.Artist);
	writeComment(ctx.metadata[0], "Title", fileInfo.Title);
	FLAC__stream_encoder_set_metadata(ctx.streamEncoder, ctx.metadata.data(), ctx.metadata.size());
	FLAC__stream_encoder_init_stream(ctx.streamEncoder, flac::write, flac::seek,
		flac::tell, nullptr, this);

	info.totalTime = fileInfo.TotalTime;
	info.bitsPerSample = fileInfo.BitsPerSample;
	info.bitRate = fileInfo.BitRate;
	info.channels = fileInfo.Channels;
	return true;
}

/*!
 * This function writes a buffer of audio to a FLAC file opened being encoded
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @attention Will not work unless \c FLAC_SetFileInfo() has been called beforehand
 */
long FLAC_WriteBuffer(void *p_FLACFile, uint8_t *InBuffer, int nInBufferLen)
	{ return audioWriteBuffer(p_FLACFile, InBuffer, nInBufferLen); }

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

int64_t flac_t::writeBuffer(const void *const bufferPtr, const uint32_t length)
{
	const auto buffer = static_cast<const char *>(bufferPtr);
	uint32_t offset = 0;
	auto &ctx = *encoderContext();
	const fileInfo_t &info = fileInfo();

	if (FLAC__stream_encoder_get_state(ctx.streamEncoder) != FLAC__STREAM_ENCODER_OK)
		return -2;
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

/*!
 * Closes an open FLAC file
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenW()
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_FLACFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 */
int FLAC_CloseFileW(void *p_FLACFile) { return audioCloseFile(p_FLACFile); }
