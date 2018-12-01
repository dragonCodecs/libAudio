#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <malloc.h>

#include <FLAC/all.h>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadFLAC.cpp
 * @brief The implementation of the FLAC decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2009-2013
 */

/*!
 * @internal
 * Internal structure for holding the decoding context for a given FLAC file
 */
struct flac_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The decoder context handle
	 */
	FLAC__StreamDecoder *streamDecoder;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	std::unique_ptr<uint8_t []> buffer;
	uint32_t bufferLen;
	uint8_t playbackBuffer[16384];
	/*!
	 * @internal
	 * The amount to shift the sample data by to convert it
	 */
	uint8_t sampleShift;
	/*!
	 * @internal
	 * The count of the number of bytes left to process
	 * (also thinkable as the number of bytes left to read)
	 */
	uint32_t bytesRemain;
	uint32_t bytesAvail;
	/*!
	 * @internal
	 * The playback class instance for the FLAC file
	 */
	std::unique_ptr<Playback> player;

	decoderContext_t();
	~decoderContext_t() noexcept;
	bool finish() noexcept;
	FLAC__StreamDecoderState nextFrame() noexcept;
};

/*!
 * @internal
 * \c f_fread() is the internal read callback for FLAC file decoding. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_dec The decoder context to read for, which must not become modified
 * @param Buffer The buffer to read into
 * @param bytes The number of bytes to read into the buffer, given as a pointer
 * @param p_FF Pointer to our internal context for the given FLAC file
 * @return A status indicating if we had success or not
 */
FLAC__StreamDecoderReadStatus f_fread(const FLAC__StreamDecoder *, uint8_t *Buffer, size_t *bytes, void *ctx)
{
	const fd_t &fd = reinterpret_cast<audioFile_t *>(ctx)->fd();
	if (*bytes > 0)
	{
		bool result = fd.read(Buffer, *bytes, *bytes);
		if (!result && !fd.isEOF())
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		else if (!result)
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		else
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}

	return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

/*!
 * @internal
 * \c f_fseek() is the internal seek callback for FLAC file decoding. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_dec The decoder context to seek for, which must not become modified
 * @param amount A 64-bit unsigned integer giving the number of bytes from the beginning
 *   of the file to seek through
 * @param p_FF Pointer to our internal context for the given FLAC file
 * @return A status indicating if the seek worked or not
 */
FLAC__StreamDecoderSeekStatus f_fseek(const FLAC__StreamDecoder *, uint64_t amount, void *ctx)
{
	const fd_t &fd = reinterpret_cast<audioFile_t *>(ctx)->fd();
	if (lseek(fd, amount, SEEK_SET) < 0)
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

/*!
 * @internal
 * \c f_ftell() is the internal read possition callback for FLAC file decoding. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_dec The decoder context to get the read position for, which must not become modified
 * @param offset A 64-bit unsigned integer returning the number of bytes from the beginning
 *   of the file at which the read possition is currently at
 * @param p_FF Pointer to our internal context for the given FLAC file
 * @return A status indicating if we were able to determine the position or not
 */
FLAC__StreamDecoderTellStatus f_ftell(const FLAC__StreamDecoder *, uint64_t *offset, void *ctx)
{
	const fd_t &fd = reinterpret_cast<audioFile_t *>(ctx)->fd();
	const off_t pos = lseek(fd, 0, SEEK_CUR);
	if (pos < 0)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	*offset = uint64_t(pos);
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

/*!
 * @internal
 * \c f_flen() is the internal file length callback for FLAC file decoding. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_dec The decoder context to get the file length for, which must not become modified
 * @param len A 64-bit unsigned integer returning the length of the file in bytes
 * @param p_FF Pointer to our internal context for the given FLAC file
 * @return A status indicating if we were able to determine the length or not
 */
FLAC__StreamDecoderLengthStatus f_flen(const FLAC__StreamDecoder *, uint64_t *len, void *ctx)
{
	const fd_t &fd = reinterpret_cast<audioFile_t *>(ctx)->fd();
	struct stat fileStat;
	if (fstat(fd, &fileStat) != 0)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;

	*len = fileStat.st_size;
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

/*!
 * @internal
 * \c f_feof() is the internal end-of-file callback for FLAC file decoding. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_dec The decoder context to get the EOF flag for, which must not become modified
 * @param p_FF Pointer to our internal context for the given FLAC file
 * @return A status indicating whether we have reached the end of the file or not
 */
int f_feof(const FLAC__StreamDecoder *, void *ctx)
{
	const fd_t &fd = reinterpret_cast<audioFile_t *>(ctx)->fd();
	return fd.isEOF() ? 1 : 0;
}

/*!
 * @internal
 * \c f_data() is the internal data callback for FLAC file decoding.
 * @param p_dec The decoder context to process data for, which must not become modified
 * @param p_frame The headers for the current frame of decoded FLAC audio
 * @param buffers The 32-bit audio buffers decoded for the current \p p_frame
 * @param p_FLACFile Pointer to our internal context for the given FLAC file
 * @return A constant status indicating that it's safe to continue reading the file
 */
FLAC__StreamDecoderWriteStatus f_data(const FLAC__StreamDecoder *, const FLAC__Frame *p_frame, const int * const buffers[], void *audioFile)
{
	const flac_t &file = *reinterpret_cast<flac_t *>(audioFile);
	auto &ctx = *file.context();
	short *PCM = reinterpret_cast<short *>(ctx.buffer.get());
	const uint8_t channels = file.fileInfo().channels;
	const uint8_t sampleShift = ctx.sampleShift;
	uint32_t len = p_frame->header.blocksize;
	if (len > (ctx.bufferLen / channels))
		len = ctx.bufferLen / channels;

	for (uint32_t i = 0; i < len; i++)
	{
		for (uint8_t j = 0; j < channels; j++)
			PCM[(i * channels) + j] = (short)(buffers[j][i] >> sampleShift);
	}
	ctx.bytesAvail = len * channels * (file.fileInfo().bitsPerSample / 8);
	ctx.bytesRemain = ctx.bytesAvail;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

uint32_t stringsLength(const char *const part) noexcept { return strlen(part); }
template<typename... Args> uint32_t stringsLength(const char *const part, Args &&...args) noexcept
	{ return strlen(part) + stringsLength(args...); }

std::unique_ptr<char []> stringsConcat(std::unique_ptr<char []> &&dst, const uint32_t offset, const char *const part) noexcept
{
	if (!dst)
		return nullptr;
	strcpy(dst.get() + offset, part);
	return std::move(dst);
}

template<typename... Args> std::unique_ptr<char []> stringsConcat(std::unique_ptr<char []> &&dst, const uint32_t offset,
	const char *const part, Args &&...args) noexcept
{
	if (!dst)
		return nullptr;
	strcpy(dst.get() + offset, part);
	return stringsConcat(std::move(dst), offset + strlen(part), args...);
}

template<typename... Args> std::unique_ptr<char []> stringConcat(const char *const part, Args &&...args) noexcept
	{ return stringsConcat(makeUnique<char []>(stringsLength(part, args...) + 1), 0, part, args...); }

void copyComment(std::unique_ptr<char []> &dst, const char *const src) noexcept
	{ dst = !dst ? stringConcat(src) : stringConcat(dst.get(), " / ", src); }

/*!
 * @internal
 * \c f_metadata() is the internal metadata callback for FLAC file decoding.
 * @param p_dec The decoder context to process metadata for, which must not become modified
 * @param p_metadata The item of metadata to process
 * @param p_FLACFile Pointer to our internal context for the given FLAC file
 */
void f_metadata(const FLAC__StreamDecoder *, const FLAC__StreamMetadata *p_metadata, void *audioFile)
{
	flac_t &file = *reinterpret_cast<flac_t *>(audioFile);
	auto &ctx = *file.context();
	fileInfo_t &info = file.fileInfo();

	if (p_metadata->type >= FLAC__METADATA_TYPE_UNDEFINED)
		return;

	switch (p_metadata->type)
	{
		case FLAC__METADATA_TYPE_STREAMINFO:
		{
			const FLAC__StreamMetadata_StreamInfo &streamInfo = p_metadata->data.stream_info;
			info.channels = streamInfo.channels;
			info.bitRate = streamInfo.sample_rate;
			info.bitsPerSample = streamInfo.bits_per_sample;
			if (info.bitsPerSample == 24)
			{
				info.bitsPerSample = 16;
				ctx.sampleShift = 8;
			}
			else
				ctx.sampleShift = 0;
			ctx.bufferLen = streamInfo.channels * streamInfo.max_blocksize;
			ctx.buffer = makeUnique<uint8_t []>(ctx.bufferLen * (streamInfo.bits_per_sample / 8));
			info.totalTime = streamInfo.total_samples / streamInfo.sample_rate;
			if (!ExternalPlayback && ctx.buffer != nullptr)
				file.player(makeUnique<Playback>(info, audioFillBuffer, ctx.playbackBuffer, 16384, audioFile));
			break;
		}
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
		{
			const FLAC__StreamMetadata_VorbisComment &comments = p_metadata->data.vorbis_comment;
			for (uint32_t i = 0; i < comments.num_comments; ++i)
			{
				auto comment = reinterpret_cast<const char *const>(comments.comments[i].entry);
				if (strncasecmp(comment, "title=", 6) == 0)
					copyComment(info.title, comment + 6);
				else if (strncasecmp(comment, "artist=", 7) == 0)
					copyComment(info.artist, comment + 7);
				else if (strncasecmp(comment, "album=", 6) == 0)
					copyComment(info.album, comment + 6);
#ifndef __arm__
				else
				{
					std::unique_ptr<char []> other;
					copyComment(other, comment);
					info.other.emplace_back(std::move(other));
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
 * \c f_metadata() is the internal error callback for FLAC file decoding.
 * @param p_dec The decoder context to process an error for, which must not become modified
 * @param errStat The error that has occured
 * @param p_FLACFile Pointer to our internal context for the given FLAC file
 * @note Implemented as a no-operation due to how the rest of the decoder is structured
 */
void f_error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *) noexcept { }

flac_t::flac_t(fd_t &&fd) noexcept : audioFile_t(audioType_t::flac, std::move(fd)), ctx(makeUnique<decoderContext_t>()) { }
flac_t::decoderContext_t::decoderContext_t() : streamDecoder(FLAC__stream_decoder_new()) { }

flac_t *flac_t::openR(const char *const fileName) noexcept
{
	std::unique_ptr<flac_t> flacFile(makeUnique<flac_t>(fd_t(fileName, O_RDONLY | O_NOCTTY)));
	if (!flacFile || !flacFile->valid() || !isFLAC(flacFile->_fd))
		return nullptr;

	lseek(flacFile->_fd, 0, SEEK_SET);
	return flacFile.release();
}

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by FLAC_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *FLAC_OpenR(const char *FileName)
{
	std::array<char, 4> sig;

	std::unique_ptr<flac_t> file(flac_t::openR(FileName));
	if (!file)
		return nullptr;
	const fd_t &fd = file->fd();
	FLAC__StreamDecoder *const p_dec = file->context()->streamDecoder;

	FLAC__stream_decoder_set_metadata_ignore_all(p_dec);
	FLAC__stream_decoder_set_metadata_respond(p_dec, FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__stream_decoder_set_metadata_respond(p_dec, FLAC__METADATA_TYPE_VORBIS_COMMENT);

	if (!fd.read(sig))
		return nullptr;
	lseek(fd, 0, SEEK_SET);
	if (strncmp(sig.data(), "OggS", 4) == 0)
		FLAC__stream_decoder_init_ogg_stream(p_dec, f_fread, f_fseek, f_ftell, f_flen, f_feof, f_data, f_metadata, f_error, file.get());
	else
		FLAC__stream_decoder_init_stream(p_dec, f_fread, f_fseek, f_ftell, f_flen, f_feof, f_data, f_metadata, f_error, file.get());
	FLAC__stream_decoder_process_until_end_of_metadata(p_dec);

	return file.release();
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c nullptr
 */
FileInfo *FLAC_GetFileInfo(void *p_FLACFile)
	{ return audioFileInfo(p_FLACFile); }

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

/*!
 * Closes an opened audio file
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenR()
 * @return an integer indicating success or failure relative to whether the
 * FLAC encoder was able to properly finish encoding
 * @warning Do not use the pointer given by \p p_FLACFile after using
 * this function - please either set it to \c nullptr or be extra carefull
 * to destroy it via scope
 */
int FLAC_CloseFileR(void *p_FLACFile) { return audioCloseFileR(p_FLACFile); }

FLAC__StreamDecoderState flac_t::decoderContext_t::nextFrame() noexcept
{
	FLAC__stream_decoder_process_single(streamDecoder);
	return FLAC__stream_decoder_get_state(streamDecoder);
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_FLACFile must not be nullptr as no checking on the parameter is done. FIXME!
 */
long FLAC_FillBuffer(void *p_FLACFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(p_FLACFile, OutBuffer, nOutBufferLen); }

int64_t flac_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	auto buffer = reinterpret_cast<char *const>(bufferPtr);
	uint32_t filled = 0;
	while (filled < length)
	{
		if (ctx->bytesRemain == 0)
		{
			const FLAC__StreamDecoderState state = ctx->nextFrame();
			if (state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED)
			{
				ctx->bytesRemain = 0;
				if (filled == 0)
					return -2;
				break;
			}
		}
		uint32_t len = ctx->bytesRemain;
		if (len > (length - filled))
			len = length - filled;
		memcpy(buffer + filled, ctx->buffer.get() + (ctx->bytesAvail - ctx->bytesRemain), len);
		filled += len;
		ctx->bytesRemain -= len;
	}
	return filled;
}

/*!
 * Plays an opened audio file using OpenAL on the default audio device
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c FLAC_OpenR() used to open the file at \p p_FLACFile,
 * this function will do nothing.
 */
void FLAC_Play(void *p_FLACFile) { audioPlay(p_FLACFile); }
void FLAC_Pause(void *p_FLACFile) { audioPause(p_FLACFile); }
void FLAC_Stop(void *p_FLACFile) { audioStop(p_FLACFile); }

/*!
 * Checks the file given by \p FileName for whether it is an FLAC
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an FLAC file or not
 */
bool Is_FLAC(const char *FileName) { return flac_t::isFLAC(FileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a FLAC
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a FLAC file or not
 */
bool flac_t::isFLAC(const int fd) noexcept
{
	char flacSig[4];
	if (fd == -1 ||
		read(fd, flacSig, 4) != 4 ||
		(strncmp(flacSig, "fLaC", 4) != 0 &&
		strncmp(flacSig, "OggS", 4) != 0))
		return false;
	return true;
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
	if (!file.valid())
		return false;
	return isFLAC(file);
}

/*!
 * @internal
 * This structure controls decoding FLAC files when using the high-level API on them
 */
API_Functions FLACDecoder =
{
	FLAC_OpenR,
	audioFileInfo,
	audioFillBuffer,
	audioCloseFileR,
	audioPlay,
	audioPause,
	audioStop
};
