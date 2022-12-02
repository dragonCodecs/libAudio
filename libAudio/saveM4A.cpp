// SPDX-License-Identifier: BSD-3-Clause
#if 0
#ifdef strncasecmp
#undef strncasecmp
#endif
#endif

#include "m4a.hxx"

/*!
 * @internal
 * @file saveM4A.cpp
 * @brief The implementation of the M4A/MP4 encoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2010-2020
 */

namespace libAudio
{
	namespace saveM4A
	{
		/*!
		* @internal
		* Internal function used to open the MP4 file for output and potential readback
		* @param fileName The name of the file to open
		* @param Mode The \c MP4FileMode in which to open the file. We ensure this has
		*    to be FILEMODE_CREATE for our purposes
		*/
		void *openW(const char *fileName, MP4FileMode Mode)
		{
			if (Mode != FILEMODE_CREATE)
				return nullptr;
			return fopen(fileName, "wb+");
		}

		/*!
		* @internal
		* Internal function used to seek in the MP4 file
		* @param file \c FILE handle for the MP4 file as a void pointer
		* @param pos Possition into the file to which to seek to
		*/
		int seek(void *file, int64_t pos)
		{
		#ifdef _WINDOWS
			return (_fseeki64((FILE *)file, pos, SEEK_SET) == 0 ? FALSE : TRUE);
		#elif defined(__arm__) || defined(__aarch64__)
			return fseeko((FILE *)file, pos, SEEK_SET) == 0 ? FALSE : TRUE;
		#else
			return (fseeko64((FILE *)file, pos, SEEK_SET) == 0 ? FALSE : TRUE);
		#endif
		}

		/*!
		* @internal
		* Internal function used to read from the MP4 file
		* @param file \c FILE handle for the MP4 file as a void pointer
		* @param buffer A typeless buffer to which the read data should be written
		* @param bufferLen A 64-bit integer giving how much data should be read from the file
		* @param read A 64-bit integer count returning how much data was actually read
		*/
		int read(void *file, void *buffer, int64_t bufferLen, int64_t *read, int64_t)
		{
			size_t ret = fread(buffer, 1, size_t(bufferLen), (FILE *)file);
			if (ret == 0 && bufferLen != 0)
				return TRUE;
			*read = ret;
			return FALSE;
		}

		/*!
		* @internal
		* Internal function used to write data to the MP4 file
		* @param file \c FILE handle for the MP4 file as a void pointer
		* @param buffer A typeless buffer holding the data to be written, which must also not become modified
		* @param bufferLen A 64-bit integer giving how much data is to be written to the file
		* @param written A 64-bit integer count returning how much data was actually written
		*/
		int write(void *file, const void *buffer, int64_t bufferLen, int64_t *written, int64_t)
		{
			if (fwrite(buffer, 1, size_t(bufferLen), (FILE *)file) != size_t(bufferLen))
				return TRUE;
			*written = bufferLen;
			return FALSE;
		}

		/*!
		* @internal
		* Internal function used to close the MP4 file after I/O is complete
		* @param file \c FILE handle for the MP4 file as a void pointer
		*/
		int close(void *file) { return fclose((FILE *)file) != 0; }

		/*!
		* @internal
		* Structure holding pointers to the \c MP4Enc* functions given in this file.
		* Used in the initialising of the MP4v2 file writer as a set of callbacks so
		* as to prevent run-time issues on Windows.
		*/
		constexpr static MP4FileProvider ioFunctions =
		{
			openW,
			seek,
			read,
			write,
			close
		};
	}
}

using namespace libAudio;

m4a_t::m4a_t(fd_t &&fd, audioModeWrite_t) noexcept :
	audioFile_t(audioType_t::m4a, std::move(fd)), encoderCtx{makeUnique<encoderContext_t>()} { }
m4a_t::encoderContext_t::encoderContext_t() : encoder{nullptr}, mp4Stream{nullptr}, track{MP4_INVALID_TRACK_ID},
	inputSamples{}, outputBytes{}, valid{true} { }

m4a_t *m4a_t::openW(const char *const fileName) noexcept
{
	auto file{makeUnique<m4a_t>(fd_t{fileName, O_RDWR | O_CREAT | O_TRUNC, substrate::normalMode},
		audioModeWrite_t{})};
	if (!file || !file->valid())
		return nullptr;
	auto &ctx = *file->encoderContext();

	ctx.mp4Stream = MP4CreateProvider(fileName, &saveM4A::ioFunctions, MP4_DETAILS_ERROR);// | MP4_DETAILS_WRITE_ALL);

	return file.release();
}

/*!
 * This function opens the file given by \c fileName for writing and returns a pointer
 * to the context of the opened file which must be used only by M4A_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *m4aOpenW(const char *fileName) { return m4a_t::openW(fileName); }

/*!
 * This function sets the \c FileInfo structure for a M4A/MP4 file being encoded
 * @param aacFile A pointer to a file opened with \c m4aOpenW()
 * @param info A \c fileInfo_t pointer containing various metadata about an opened file
 * @warning This function must be called before using \c M4A_WriteBuffer()
 * @bug \p p_FI must not be \c nullptr as no checking on the parameter is done. FIXME!
 *
 * @bug \p aacFile must not be \c nullptr as no checking on the parameter is done. FIXME!
 */
bool m4a_t::fileInfo(const fileInfo_t &info)
{
	auto &ctx = *encoderContext();
	const MP4Tags *tags = MP4TagsAlloc();
	if (!tags || info.bitsPerSample != 16)
		return false;

	ctx.encoder = faacEncOpen(info.bitRate, info.channels, &ctx.inputSamples, &ctx.outputBytes);
	if (!ctx.encoder)
		return false;
	ctx.buffer = makeUnique<uint8_t []>(ctx.outputBytes);
	if (!ctx.buffer)
		return ctx.valid = false;

	MP4SetTimeScale(ctx.mp4Stream, info.bitRate);
	ctx.track = MP4AddAudioTrack(ctx.mp4Stream, info.bitRate, 1024);
	//p_AF->MaxInSamp / info->channels, MP4_MPEG4_AUDIO_TYPE);
	MP4SetAudioProfileLevel(ctx.mp4Stream, 0x0F);

	if (info.album)
		MP4TagsSetAlbum(tags, info.album.get());
	if (info.artist)
		MP4TagsSetArtist(tags, info.artist.get());
	if (info.title)
		MP4TagsSetName(tags, info.title.get());
	MP4TagsSetEncodingTool(tags, "libAudio " libAudioVersion);
	MP4TagsStore(tags, ctx.mp4Stream);
	MP4TagsFree(tags);

	auto config = faacEncGetCurrentConfiguration(ctx.encoder);
	if (!config)
		return ctx.valid = false;
	config->inputFormat = FAAC_INPUT_16BIT;
	config->mpegVersion = MPEG4;
	config->bitRate = 128000;
//	config->bitRate = 64000;
	config->outputFormat = 0;
	config->useLfe = config->useTns = config->allowMidside = 0;
	config->aacObjectType = LOW;//MAIN;
	config->bandWidth = 0;
	config->quantqual = 100;
	if (!faacEncSetConfiguration(ctx.encoder, config))
		return ctx.valid = false;

	uint8_t *ascBuffer = nullptr;
	unsigned long ascLength = 0;
	if (faacEncGetDecoderSpecificInfo(ctx.encoder, &ascBuffer, &ascLength))
		return ctx.valid = false;
	MP4SetTrackESConfiguration(ctx.mp4Stream, ctx.track, ascBuffer, ascLength);
	free(ascBuffer);

	fileInfo() = info;
	return true;
}

/*!
 * This function writes a buffer of audio to a M4A/MP4 file opened being encoded
 * @param aacFile A pointer to a file opened with \c m4aOpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @attention Will not work unless \c M4A_SetFileInfo() has been called beforehand
 */
int64_t m4a_t::writeBuffer(const void *const bufferPtr, const int64_t rawLength)
{
	uint32_t offset = 0;
	auto &ctx = *encoderContext();
	auto buffer = static_cast<const char *>(bufferPtr);
	if (!ctx.encoder)
		return -3;
	else if (!ctx.valid)
		return -4;
	else if (rawLength == -2 || rawLength <= 0)
	{
		int32_t sampleCount{};
		do
		{
			sampleCount = faacEncEncode(ctx.encoder, nullptr, 0, ctx.buffer.get(), ctx.outputBytes);
			if (sampleCount > 0)
				MP4WriteSample(ctx.mp4Stream, ctx.track, ctx.buffer.get(), sampleCount);
		}
		while (sampleCount > 0);
		return -2;
	}

	const uint32_t length = uint32_t(rawLength);
	while (offset < length)
	{
		const uint32_t samplesMax = std::min((length - offset) >> 1U, uint32_t(ctx.inputSamples));
		int sampleCount = faacEncEncode(ctx.encoder,
			const_cast<int32_t *>(reinterpret_cast<const int32_t *>(buffer + offset)),
			samplesMax, ctx.buffer.get(), ctx.outputBytes);
		if (!sampleCount)
			continue;
		else if (sampleCount < 0)
			break;
		MP4WriteSample(ctx.mp4Stream, ctx.track, ctx.buffer.get(), ctx.outputBytes);
		offset += samplesMax << 1U;
	}
	return offset;
}

m4a_t::encoderContext_t::~encoderContext_t() noexcept
{
	MP4Close(mp4Stream);
	if (encoder)
		faacEncClose(encoder);
}
