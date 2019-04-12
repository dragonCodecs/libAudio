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
 * @date 2009-2013
 */

/*!
 * @internal
 * Internal structure for holding the decoding context for a given M4A/MP4 file
 */
struct m4a_t::decoderContext_t final
{
	NeAACDecHandle p_dec;
	MP4FileHandle p_MP4;
	MP4TrackId nTrack;
	uint8_t playbackBuffer[8192];

	std::unique_ptr<playback_t> player;
};

typedef struct _M4A_Intern
{
	/*!
	 * @internal
	 * The decoder context handle
	 */
	NeAACDecHandle p_dec;
	/*!
	 * @internal
	 * The MP4v2 handle for the MP4 file being read from
	 */
	MP4FileHandle p_MP4;
	/*!
	 * @internal
	 * The MP4v2 track from which the decoded audio data is being read
	 */
	MP4TrackId nTrack;
	/*!
	 * @internal
	 * The count returned as a parameter of \c NeAACDecInit2() holding
	 * the sample rate of the MP4 audio stream to decode
	 */
	uint32_t ActualSampleRate;
	/*!
	 * @internal
	 * The count returned as a parameter of \c NeAACDecInit2() holding
	 * the number of channels encoded in the MP4 audio stream to decode
	 */
	uint8_t ActualChannels;
	/*!
	 * @internal
	 * The \c FileInfo for the AAC file being decoded
	 */
	FileInfo *p_FI;
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
	int nLoops, nCurrLoop, samplesUsed, nSamples;
	/*!
	 * @internal
	 * Pointer to the static return result of the call to \c NeAACDecDecode()
	 */
	uint8_t *p_Samples;
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;
	/*!
	 * @internal
	 * The MP4v2 tags structure allocated and filled for the metadata
	 * in the MP4 file
	 */
	const MP4Tags *p_Tags;

	m4a_t::decoderContext_t inner;
} M4A_Intern;

void *MP4DecOpen(const char *FileName, MP4FileMode Mode);
int MP4DecSeek(void *MP4File, int64_t pos);
int MP4DecRead(void *MP4File, void *DataOut, int64_t DataOutLen, int64_t *Read, int64_t);
int MP4DecWrite(void *MP4File, const void *DataIn, int64_t DataInLen, int64_t *Written, int64_t);
int MP4DecClose(void *MP4File);

/*!
 * @internal
 * Structure holding pointers to the \c MP4Dec* functions given in this file.
 * Used in the initialising of the MP4v2 file reader as a set of callbacks so
 * as to prevent run-time issues on Windows.
 */
MP4FileProvider MP4DecFunctions =
{
	MP4DecOpen,
	MP4DecSeek,
	MP4DecRead,
	MP4DecWrite,
	MP4DecClose
};

/*!
 * @internal
 * Internal function used to open the MP4 file for reading
 * @param FileName The name of the file to open
 * @param Mode The \c MP4FileMode in which to open the file. We ensure this has
 *    to be FILEMODE_CREATE for our purposes
 */
void *MP4DecOpen(const char *FileName, MP4FileMode Mode)
{
	if (Mode != FILEMODE_READ)
		return NULL;
	return fopen(FileName, "rb");
}

/*!
 * @internal
 * Internal function used to seek in the MP4 file
 * @param MP4File \c FILE handle for the MP4 file as a void pointer
 * @param pos Possition into the file to which to seek to
 */
int MP4DecSeek(void *MP4File, int64_t pos)
{
#ifdef _WINDOWS
	return (_fseeki64((FILE *)MP4File, pos, SEEK_SET) == 0 ? FALSE : TRUE);
#elif defined(__arm__) || defined(__aarch64__)
	return fseeko((FILE *)MP4File, pos, SEEK_SET) == 0 ? FALSE : TRUE;
#else
	return (fseeko64((FILE *)MP4File, pos, SEEK_SET) == 0 ? FALSE : TRUE);
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
int MP4DecRead(void *MP4File, void *DataOut, int64_t DataOutLen, int64_t *Read, int64_t)
{
	int ret = fread(DataOut, 1, (size_t)DataOutLen, (FILE *)MP4File);
	if (ret <= 0 && DataOutLen != 0)
		return TRUE;
	*Read = ret;
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
int MP4DecWrite(void *MP4File, const void *DataIn, int64_t DataInLen, int64_t *Written, int64_t)
{
	if (fwrite(DataIn, 1, (size_t)DataInLen, (FILE *)MP4File) != (size_t)DataInLen)
		return TRUE;
	*Written = DataInLen;
	return FALSE;
}

/*!
 * @internal
 * Internal function used to close the MP4 file after I/O is complete
 * @param MP4File \c FILE handle for the MP4 file as a void pointer
 */
int MP4DecClose(void *MP4File)
{
	return (fclose((FILE *)MP4File) == 0 ? FALSE : TRUE);
}

/*!
 * @internal
 * Internal function used to determine the first usable audio track and initialise decoding on it
 * @param ret Our internal decoder structure's pointer named the same as in the only function
 *   which calls this so as to keep name changing and confusion down
 * @return The MP4v2 track ID located for the decoder or -1 on error
 */
MP4TrackId GetAACTrack(M4A_Intern *ret)
{
	/* find AAC track */
	int i, numTracks = MP4GetNumberOfTracks(ret->p_MP4, NULL, 0);

	for (i = 0; i < numTracks; i++)
	{
		NeAACDecConfiguration *ADC;
		uint8_t *Buff = NULL;
		uint32_t BuffLen = 0;
		MP4TrackId Track = MP4FindTrackId(ret->p_MP4, i, NULL, 0);
		const char *TrackType = MP4GetTrackType(ret->p_MP4, Track);

		if (!MP4_IS_AUDIO_TRACK_TYPE(TrackType))
			continue;

		MP4GetTrackESConfiguration(ret->p_MP4, Track, &Buff, &BuffLen);

		NeAACDecInit2(ret->p_dec, Buff, BuffLen, (unsigned long *)&ret->ActualSampleRate, &ret->ActualChannels);
		ADC = NeAACDecGetCurrentConfiguration(ret->p_dec);
		ADC->outputFormat = FAAD_FMT_16BIT;
		NeAACDecSetConfiguration(ret->p_dec, ADC);
		free(Buff);
		return Track;
	}

	/* can't decode this */
	return -1;
}

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by M4A_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *M4A_OpenR(const char *FileName)
{
	M4A_Intern *ret = new (std::nothrow) M4A_Intern();
	if (ret == NULL)
		return ret;

	ret->eof = false;
	ret->p_dec = NeAACDecOpen();
	ret->p_MP4 = MP4ReadProvider(FileName, 0, &MP4DecFunctions);
	ret->nTrack = GetAACTrack(ret);
	ret->p_Tags = MP4TagsAlloc();

	return ret;
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_M4AFile A pointer to a file opened with \c M4A_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c M4A_Play() or \c M4A_FillBuffer()
 * @bug \p p_M4AFile must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *M4A_GetFileInfo(void *p_M4AFile)
{
	const MP4Tags *p_Tags;
	uint32_t timescale;
	const char *value;
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;
	FileInfo *ret = NULL;

	p_MF->p_FI = ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	ret->BitRate = p_MF->ActualSampleRate;
	ret->Channels = p_MF->ActualChannels;
	MP4TagsFetch(p_MF->p_Tags, p_MF->p_MP4);
	p_Tags = p_MF->p_Tags;

	ret->Album = p_Tags->album;
	ret->Artist = (p_Tags->artist == NULL ? p_Tags->albumArtist : p_Tags->artist);
	ret->Title = p_Tags->name;
	ret->BitsPerSample = 16;
	value = p_Tags->comments;
	if (value == NULL)
		ret->nOtherComments = 0;
	else
	{
		ret->nOtherComments = 1;
		ret->OtherComments.push_back(value);
	}
	timescale = MP4GetTrackTimeScale(p_MF->p_MP4, p_MF->nTrack);
	ret->TotalTime = MP4GetTrackDuration(p_MF->p_MP4, p_MF->nTrack) / timescale;
	p_MF->nLoops = MP4GetTrackNumberOfSamples(p_MF->p_MP4, p_MF->nTrack);
	p_MF->nCurrLoop = 0;

	if (ExternalPlayback == 0)
		p_MF->inner.player.reset(new playback_t(p_M4AFile, M4A_FillBuffer, p_MF->inner.playbackBuffer, 8192, ret));

	return ret;
}

/*!
 * Closes an opened audio file
 * @param p_M4AFile A pointer to a file opened with \c M4A_OpenR()
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_M4AFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_M4AFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int M4A_CloseFileR(void *p_M4AFile)
{
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;

	NeAACDecClose(p_MF->p_dec);
	MP4Close(p_MF->p_MP4);
	MP4TagsFree(p_MF->p_Tags);

	delete p_MF;
	return 0;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_M4AFile A pointer to a file opened with \c M4A_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_M4AFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long M4A_FillBuffer(void *p_M4AFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;
	uint8_t *OBuf = OutBuffer;

	while ((OBuf - OutBuffer) < nOutBufferLen && p_MF->eof == false)
	{
		uint32_t nUsed;
		if (p_MF->samplesUsed == p_MF->nSamples)
		{
			if (p_MF->nCurrLoop < p_MF->nLoops)
			{
				NeAACDecFrameInfo FI;
				uint8_t *Buff = NULL;
				uint32_t nBuff = 0;
				p_MF->nCurrLoop++;
				if (MP4ReadSample(p_MF->p_MP4, p_MF->nTrack, p_MF->nCurrLoop, &Buff, &nBuff) == false)
				{
					p_MF->eof = true;
					return -2;
				}
				p_MF->p_Samples = (uint8_t *)NeAACDecDecode(p_MF->p_dec, &FI, Buff, nBuff);
				free(Buff);

				p_MF->nSamples = FI.samples * FI.channels;
				p_MF->samplesUsed = 0;
				if (FI.error != 0)
				{
					printf("Error: %s\n", NeAACDecGetErrorMessage(FI.error));
					p_MF->nSamples = 0;
					continue;
				}
			}
			else if (p_MF->nCurrLoop == p_MF->nLoops)
				return -1;

		}

		nUsed = std::min<int64_t>(p_MF->nSamples - p_MF->samplesUsed, nOutBufferLen - (OBuf - OutBuffer));
		memcpy(OBuf, p_MF->p_Samples + p_MF->samplesUsed, nUsed);
		OBuf += nUsed;
		p_MF->samplesUsed += nUsed;
	}

	return OBuf - OutBuffer;
}

/*!
 * Plays an opened audio file using OpenAL on the default audio device
 * @param p_M4AFile A pointer to a file opened with \c M4A_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 *   the call to \c M4A_OpenR() used to open the file at \p p_M4AFile,
 *   this function will do nothing.
 * @bug \p p_M4AFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_M4AFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void M4A_Play(void *p_M4AFile)
{
	if (!p_M4AFile)
		return;
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;
	p_MF->inner.player->play();
}

void M4A_Pause(void *p_M4AFile)
{
	if (!p_M4AFile)
		return;
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;
	p_MF->inner.player->pause();
}

void M4A_Stop(void *p_M4AFile)
{
	if (!p_M4AFile)
		return;
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;
	p_MF->inner.player->stop();
}

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
	M4A_GetFileInfo,
	M4A_FillBuffer,
	M4A_CloseFileR,
	M4A_Play,
	M4A_Pause,
	M4A_Stop
};
