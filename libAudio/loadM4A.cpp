#include <stdio.h>
#include <malloc.h>
#include <algorithm>

#include <neaacdec.h>
#include <mp4v2/mp4v2.h>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadM4A.cpp
 * @brief The implementation of the M4A/MP4 decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2009-2019
 */

/*!
 * @internal
 * Internal structure for holding the decoding context for a given M4A/MP4 file
 */
struct m4a_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The decoder context handle
	 */
	NeAACDecHandle decoder;
	/*!
	 * @internal
	 * The MP4v2 handle for the MP4 file being read from
	 */
	MP4FileHandle mp4Stream;
	/*!
	 * @internal
	 * The MP4v2 track from which the decoded audio data is being read
	 */
	MP4TrackId track;
	/*!
	 * @internal
	 * @var int nLoops
	 * The number of frames decoded relative to the total number
	 * @var int nCurrLoop
	 * The total number of frames to decode
	 * @var int samplesUsed
	 * The number of samples used so far from the current sample buffer
	 * @var int nSamples
	 * The total number of samples in the current sample buffer
	 */
	uint32_t frameCount, currentFrame;
	uint64_t sampleCount, samplesUsed;
	/*!
	 * @internal
	 * Pointer to the static return result of the call to \c NeAACDecDecode()
	 */
	uint8_t *samples;
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;

	uint8_t playbackBuffer[8192];

	decoderContext_t();
	~decoderContext_t() noexcept;
	void finish() noexcept;
	void aacTrack(fileInfo_t &fileInfo) noexcept;
};

namespace libAudio
{
	namespace m4a
	{
		/*!
		* @internal
		* Internal function used to open the MP4 file for reading
		* @param FileName The name of the file to open
		* @param Mode The \c MP4FileMode in which to open the file. We ensure this has
		*    to be FILEMODE_CREATE for our purposes
		*/
		void *open(const char *fileName, MP4FileMode mode)
		{
			if (mode != FILEMODE_READ)
				return nullptr;
			return fopen(fileName, "rb");
		}

		/*!
		* @internal
		* Internal function used to seek in the MP4 file
		* @param MP4File \c FILE handle for the MP4 file as a void pointer
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
		* @param MP4File \c FILE handle for the MP4 file as a void pointer
		* @param DataOut A typeless buffer to which the read data should be written
		* @param DataOutLen A 64-bit integer giving how much data should be read from the file
		* @param Read A 64-bit integer count returning how much data was actually read
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
		* @param MP4File \c FILE handle for the MP4 file as a void pointer
		* @param DataIn A typeless buffer holding the data to be written, which must also not become modified
		* @param DataInLen A 64-bit integer giving how much data is to be written to the file
		* @param Written A 64-bit integer count returning how much data was actually written
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
		* @param MP4File \c FILE handle for the MP4 file as a void pointer
		*/
		int close(void *file) { return fclose((FILE *)file) != 0; }

		/*!
		* @internal
		* Structure holding pointers to the \c MP4Dec* functions given in this file.
		* Used in the initialising of the MP4v2 file reader as a set of callbacks so
		* as to prevent run-time issues on Windows.
		*/
		MP4FileProvider ioFunctions =
		{
			open,
			seek,
			read,
			write,
			close
		};
	}
}

using namespace libAudio;

m4a_t::m4a_t(fd_t &&fd) noexcept : audioFile_t(audioType_t::m4a, std::move(fd)), ctx(makeUnique<decoderContext_t>()) { }
m4a_t::decoderContext_t::decoderContext_t() : decoder{NeAACDecOpen()}, mp4Stream{nullptr},
	track{MP4_INVALID_TRACK_ID}, frameCount{0}, currentFrame{0}, sampleCount{0}, samplesUsed{0},
	samples{nullptr}, eof{false}, playbackBuffer{} { }

/*!
 * @internal
 * Internal function used to determine the first usable audio track and initialise decoding on it
 * @param ret Our internal decoder structure's pointer named the same as in the only function
 *   which calls this so as to keep name changing and confusion down
 * @return The MP4v2 track ID located for the decoder or -1 on error
 */
void m4a_t::decoderContext_t::aacTrack(fileInfo_t &fileInfo) noexcept
{
	/* find AAC track */
	const uint32_t trackCount = MP4GetNumberOfTracks(mp4Stream, nullptr, 0);

	for (uint32_t i = 0; i < trackCount; ++i)
	{
		const MP4TrackId trackID = MP4FindTrackId(mp4Stream, i, nullptr, 0);
		const char *type = MP4GetTrackType(mp4Stream, trackID);

		if (!MP4_IS_AUDIO_TRACK_TYPE(type))
			continue;
		track = trackID;

		uint8_t *buffer = nullptr;
		uint32_t bufferLen = 0;
		MP4GetTrackESConfiguration(mp4Stream, track, &buffer, &bufferLen);

		unsigned long sampleRate = 0;
		unsigned char channels = 0;
		if (NeAACDecInit2(decoder, buffer, bufferLen, &sampleRate, &channels))
			return finish(); // Return having cleaned up, rather than crash
		fileInfo.bitRate = sampleRate;
		fileInfo.channels = channels;
		MP4Free(buffer);

		NeAACDecConfiguration *config = NeAACDecGetCurrentConfiguration(decoder);
		config->outputFormat = FAAD_FMT_16BIT;
		NeAACDecSetConfiguration(decoder, config);

		frameCount = MP4GetTrackNumberOfSamples(mp4Stream, track);
	}
	/* can't decode this */
}

void m4a_t::fetchTags() noexcept
{
	fileInfo_t &info = fileInfo();
	const MP4Tags *tags = MP4TagsAlloc();
	MP4TagsFetch(tags, ctx->mp4Stream);

	info.album = stringDup(tags->album);
	info.artist = stringDup(tags->artist ? tags->artist : tags->albumArtist);
	info.title = stringDup(tags->name);
	if (tags->comments)
		info.other.emplace_back(stringDup(tags->comments));

	info.bitsPerSample = 16;
	const uint32_t timescale = MP4GetTrackTimeScale(ctx->mp4Stream, ctx->track);
	info.totalTime = MP4GetTrackDuration(ctx->mp4Stream, ctx->track) / timescale;
}

m4a_t *m4a_t::openR(const char *const fileName) noexcept
{
	std::unique_ptr<m4a_t> m4aFile(makeUnique<m4a_t>(fd_t(fileName, O_RDONLY | O_NOCTTY)));
	if (!m4aFile || !m4aFile->valid() || !isM4A(m4aFile->_fd))
		return nullptr;
	return m4aFile.release();
}

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by M4A_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *M4A_OpenR(const char *FileName)
{
	std::unique_ptr<m4a_t> file(m4a_t::openR(FileName));
	if (!file)
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	ctx.mp4Stream = MP4ReadProvider(FileName, 0, &m4a::ioFunctions);
	ctx.aacTrack(info);
	if (!ctx.decoder)
		return nullptr;
	file->fetchTags();

	if (ExternalPlayback == 0)
		file->player(makeUnique<playback_t>(file.get(), M4A_FillBuffer, ctx.playbackBuffer, 8192, info));

	return file.release();
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_M4AFile A pointer to a file opened with \c M4A_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c nullptr
 * @warning This function must be called before using \c M4A_Play() or \c M4A_FillBuffer()
 * @bug \p p_M4AFile must not be nullptr as no checking on the parameter is done. FIXME!
 */
FileInfo *M4A_GetFileInfo(void *p_M4AFile)
	{ return audioFileInfo(p_M4AFile); }

void m4a_t::decoderContext_t::finish() noexcept
{
	NeAACDecClose(decoder);
	decoder = nullptr;
	MP4Close(mp4Stream);
	mp4Stream = nullptr;
}

m4a_t::decoderContext_t::~decoderContext_t() noexcept { finish(); }

/*!
 * Closes an opened audio file
 * @param p_M4AFile A pointer to a file opened with \c M4A_OpenR()
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_M4AFile after using
 * this function - please either set it to \c nullptr or be extra carefull
 * to destroy it via scope
 * @bug \p p_M4AFile must not be nullptr as no checking on the parameter is done. FIXME!
 */
int M4A_CloseFileR(void *p_M4AFile) { return audioCloseFileR(p_M4AFile); }

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_M4AFile A pointer to a file opened with \c M4A_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_M4AFile must not be nullptr as no checking on the parameter is done. FIXME!
 */
long M4A_FillBuffer(void *p_M4AFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(p_M4AFile, OutBuffer, nOutBufferLen); }

int64_t m4a_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	auto &ctx = *context();
	auto buffer = reinterpret_cast<uint8_t *const>(bufferPtr);
	auto *OBuf = buffer;

	while ((OBuf - buffer) < length && !ctx.eof)
	{
		uint32_t nUsed;
		if (ctx.samplesUsed == ctx.sampleCount)
		{
			if (ctx.currentFrame < ctx.frameCount)
			{
				NeAACDecFrameInfo FI;
				uint8_t *Buff = nullptr;
				uint32_t nBuff = 0;
				++ctx.currentFrame;
				if (!MP4ReadSample(ctx.mp4Stream, ctx.track, ctx.currentFrame, &Buff, &nBuff))
				{
					ctx.eof = true;
					return -2;
				}
				ctx.samples = (uint8_t *)NeAACDecDecode(ctx.decoder, &FI, Buff, nBuff);
				MP4Free(Buff);

				ctx.sampleCount = FI.samples * FI.channels;
				ctx.samplesUsed = 0;
				if (FI.error != 0)
				{
					printf("Error: %s\n", NeAACDecGetErrorMessage(FI.error));
					ctx.sampleCount = 0;
					continue;
				}
			}
			else if (ctx.currentFrame == ctx.frameCount)
				return -1;
		}

		nUsed = std::min<uint64_t>(ctx.sampleCount - ctx.samplesUsed, length - (OBuf - buffer));
		memcpy(OBuf, ctx.samples + ctx.samplesUsed, nUsed);
		OBuf += nUsed;
		ctx.samplesUsed += nUsed;
	}

	return OBuf - buffer;
}

/*!
 * Plays an opened audio file using OpenAL on the default audio device
 * @param p_M4AFile A pointer to a file opened with \c M4A_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 *   the call to \c M4A_OpenR() used to open the file at \p p_M4AFile,
 *   this function will do nothing.
 * @bug \p p_M4AFile must not be nullptr as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_M4AFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void M4A_Play(void *p_M4AFile) { audioPlay(p_M4AFile); }
void M4A_Pause(void *p_M4AFile) { audioPause(p_M4AFile); }
void M4A_Stop(void *p_M4AFile) { audioStop(p_M4AFile); }

// Standard "ftyp" Atom for a MOV based MP4 AAC file:
// 00 00 00 20 66 74 79 70 4D 34 41 20
// .  .  .     f  t  y  p  M  4  A

/*!
 * Checks the file given by \p FileName for whether it is an MP4/M4A
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an MP4/M4A file or not
 */
bool Is_M4A(const char *FileName) { return m4a_t::isM4A(FileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a MP4/M4A
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a MP4/M4A file or not
 */
bool m4a_t::isM4A(const int fd) noexcept
{
	char length[4], typeSig[4], fileType[4];
	if (fd == -1 ||
		read(fd, length, 4) != 4 ||
		read(fd, typeSig, 4) != 4 ||
		read(fd, fileType, 4) != 4 ||
		strncmp(typeSig, "ftyp", 4) != 0 ||
		(strncmp(fileType, "M4A ", 4) != 0 &&
		strncmp(fileType, "mp42", 4) != 0))
		return false;
	return true;
}

/*!
 * Checks the file given by \p fileName for whether it is a MP4/M4A
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a MP4/M4A file or not
 */
bool m4a_t::isM4A(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isM4A(file);
}

/*!
 * @internal
 * This structure controls decoding MP4/M4A files when using the high-level API on them
 */
API_Functions M4ADecoder =
{
	M4A_OpenR,
	audioFileInfo,
	audioFillBuffer,
	audioCloseFileR,
	audioPlay,
	audioPause,
	audioStop
};
