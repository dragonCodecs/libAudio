#include "flac.hxx"

/*!
 * @internal
 * @file saveFLAC.cpp
 * @brief The implementation of the FLAC encoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2019
 */

mode_t normalMode = S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH;

typedef struct _FLAC_Encoder_Context
{
	flac_t inner;

	_FLAC_Encoder_Context(const char *const fileName) : inner(fd_t(fileName, O_RDWR | O_CREAT | O_TRUNC, normalMode), audioModeWrite_t{}) { }
} FLAC_Encoder_Context;

/*!
 * @internal
 * \c f_fwrite() is the internal write callback for FLAC file creation. This prevents
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
FLAC__StreamEncoderWriteStatus f_fwrite(const FLAC__StreamEncoder *, const uint8_t *buffer, size_t bytes, uint32_t, uint32_t, void *ctx)
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
 * \c f_fseek() is the internal seek callback for FLAC file creation. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_enc The encoding context which must not be modified by the function
 * @param offset The offset through the file to which to seek to
 * @param p_FLACFile Our own internal context pointer which holds the file to seek through
 */
FLAC__StreamEncoderSeekStatus f_fseek(const FLAC__StreamEncoder *, uint64_t offset, void *ctx)
{
	const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
	const off_t result = fd.seek(offset, SEEK_SET);
	if (result == -1 || uint64_t(result) != offset)
		return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
	return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

/*!
 * @internal
 * \c f_ftell() is the internal seek callback for FLAC file creation. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_enc The encoding context which must not be modified by the function
 * @param offset The returned offset location into the file
 * @param p_FLACFile Our own internal context pointer which holds the file to get
 *   the write pointer position of
 */
FLAC__StreamEncoderTellStatus f_ftell(const FLAC__StreamEncoder *, uint64_t *offset, void *ctx)
{
	const fd_t &fd = static_cast<audioFile_t *>(ctx)->fd();
	const off_t pos = fd.tell();
	if (pos == -1)
		return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
	*offset = pos;
	return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}

/*!
 * @internal
 * \c f_fmetadata() is the internal metadata callback for FLAC file creation.
 * @param p_enc The encoding context which must not be modified by the function
 * @param p_meta The metadata that is due for processing this callback call
 * @param p_FLACFile Our own internal context pointer
 * @note This is implemented as a no-operation as we are disinterested in processing
 *   the metadata being sent to the file - we just want it written.
 */
void f_fmetadata(const FLAC__StreamEncoder */*p_enc*/, const FLAC__StreamMetadata */*p_meta*/, void */*p_FLACFile*/)
{
}

flac_t::flac_t(fd_t &&fd, audioModeWrite_t) noexcept : audioFile_t{audioType_t::flac, std::move(fd)},
	encoderCtx{makeUnique<encoderContext_t>()} { }
flac_t::encoderContext_t::encoderContext_t() noexcept : streamEncoder{FLAC__stream_encoder_new()}, fileInfo{}
{
	FLAC__stream_encoder_set_compression_level(streamEncoder, 4);
	//FLAC__stream_encoder_set_loose_mid_side_stereo(streamEncoder, true);
}

/*!
 * This function opens the file given by \c FileName for writing and returns a pointer
 * to the context of the opened file which must be used only by FLAC_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *FLAC_OpenW(const char *FileName)
{
	std::unique_ptr<FLAC_Encoder_Context> ret = makeUnique<FLAC_Encoder_Context>(FileName);
	if (!ret || !ret->inner.encoderContext())
		return nullptr;
	return ret.release();
}

/*!
 * This function sets the \c FileInfo structure for a FLAC file being encoded
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file
 * @warning This function must be called before using \c FLAC_WriteBuffer()
 * @bug p_FI must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug \p p_FLACFile must not be \c NULL as no checking on the parameter is done. FIXME!
 */
void FLAC_SetFileInfo(void *p_FLACFile, FileInfo *p_FI)
{
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;
	audioFileInfo(&p_FF->inner, p_FI);
}

void flac_t::fileInfo(const FileInfo &fileInfo)
{
	auto &ctx = *encoderContext();
	FLAC__StreamMetadata_VorbisComment_Entry entry;

	FLAC__stream_encoder_set_channels(ctx.streamEncoder, fileInfo.Channels);
	FLAC__stream_encoder_set_bits_per_sample(ctx.streamEncoder, fileInfo.BitsPerSample);
	FLAC__stream_encoder_set_sample_rate(ctx.streamEncoder, fileInfo.BitRate);

	std::array<FLAC__StreamMetadata *, 2> metadata
	{
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT),
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)
	};
	for (uint32_t i = 0; i < fileInfo.nOtherComments; i++)
	{
		entry.entry = (uint8_t *)fileInfo.OtherComments[i];
		entry.length = strlen(fileInfo.OtherComments[i]);
		FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, true);
	}

	if (fileInfo.Album)
	{
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "Album", fileInfo.Album);
		FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false);
	}
	if (fileInfo.Artist)
	{
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "Artist", fileInfo.Artist);
		FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false);
	}
	if (fileInfo.Title)
	{
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "Title", fileInfo.Title);
		FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false);
	}
	FLAC__stream_encoder_set_metadata(ctx.streamEncoder, metadata.data(), metadata.size());
	ctx.fileInfo = fileInfo;
	FLAC__stream_encoder_init_stream(ctx.streamEncoder, f_fwrite, f_fseek, f_ftell, f_fmetadata, this);
}

/*!
 * This function writes a buffer of audio to a FLAC file opened being encoded
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @attention Will not work unless \c FLAC_SetFileInfo() has been called beforehand
 */
long FLAC_WriteBuffer(void *p_FLACFile, uint8_t *InBuffer, int nInBufferLen)
{
	if (nInBufferLen <= 0)
		return nInBufferLen;
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;
	auto &ctx = *p_FF->inner.encoderContext();
	uint32_t nIBL = (nInBufferLen / (ctx.fileInfo.BitsPerSample / 8)) / ctx.fileInfo.Channels;
	int *IB = (int *)malloc(sizeof(int) * (nInBufferLen / (ctx.fileInfo.BitsPerSample / 8)));

	for (uint32_t i = 0; i < nIBL * ctx.fileInfo.Channels; i++)
		IB[i] = (int)(((short *)InBuffer)[i]);

	FLAC__stream_encoder_process_interleaved(ctx.streamEncoder, IB, nIBL);

	free(IB);
	return nInBufferLen;
}

int64_t flac_t::writeBuffer(const void *const buffer, const uint32_t length)
{
	return 0;
}

bool flac_t::encoderContext_t::finish() noexcept
{
	if (!streamEncoder)
		return true;
	const bool result = !FLAC__stream_encoder_finish(streamEncoder);
	FLAC__stream_encoder_delete(streamEncoder);
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
int FLAC_CloseFileW(void *p_FLACFile)
{
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;
	delete p_FF;
	return 0;
}
