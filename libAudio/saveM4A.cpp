#include <stdint.h>
#include <faac.h>
#include <mp4v2/mp4v2.h>

#ifdef strncasecmp
#undef strncasecmp
#endif

#include "libAudio.h"
#include "libAudio_Common.h"

/*!
 * @internal
 * @file saveM4A.cpp
 * @brief The implementation of the M4A/MP4 encoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2019
 */

/*!
 * @internal
 * Internal structure for holding the encoding context for a given M4A/MP4 file
 */
typedef struct _M4A_Enc_Intern
{
	/*!
	 * @internal
	 * The encoder context itself
	 */
	faacEncHandle p_enc;
	/*!
	 * @internal
	 * Holds the count returned by \c faacEncOpen() giving the maximum and
	 *   prefered number of samples to feed \c faacEncEncode() with
	 */
	unsigned long MaxInSamp;
	/*!
	 * @internal
	 * Holds the count returned by \c faacEncOpen() giving the maximum number
	 *   of bytes that \c faacEncEncode() will return in the output buffer
	 */
	unsigned long MaxOutByte;
	/*!
	 * @internal
	 * The MP4v2 handle for the MP4 file being written to
	 */
	MP4FileHandle p_mp4;
	/*!
	 * @internal
	 * The MP4v2 track to which the encoded audio data is being written
	 */
	MP4TrackId track;
	/*!
	 * @internal
	 * A boolean giving whether an encoding error has occured
	 */
	bool err;
	/*!
	 * @internal
	 * A count giving the number of channels being processed. Taken from
	 * the \c FileInfo structure passed into \c M4A_SetFileInfo()
	 */
	int Channels;
} M4A_Enc_Intern;

void *MP4EncOpen(const char *FileName, MP4FileMode Mode);
int MP4EncSeek(void *MP4File, int64_t pos);
int MP4EncRead(void *MP4File, void *DataOut, int64_t DataOutLen, int64_t *Read, int64_t);
int MP4EncWrite(void *MP4File, const void *DataIn, int64_t DataInLen, int64_t *Written, int64_t);
int MP4EncClose(void *MP4File);

/*!
 * @internal
 * Structure holding pointers to the \c MP4Enc* functions given in this file.
 * Used in the initialising of the MP4v2 file writer as a set of callbacks so
 * as to prevent run-time issues on Windows.
 */
MP4FileProvider MP4EncFunctions =
{
	MP4EncOpen,
	MP4EncSeek,
	MP4EncRead,
	MP4EncWrite,
	MP4EncClose
};

/*!
 * @internal
 * Internal function used to open the MP4 file for output and potential readback
 * @param FileName The name of the file to open
 * @param Mode The \c MP4FileMode in which to open the file. We ensure this has
 *    to be FILEMODE_CREATE for our purposes
 */
void *MP4EncOpen(const char *FileName, MP4FileMode Mode)
{
	if (Mode != FILEMODE_CREATE)
		return NULL;
	return fopen(FileName, "wb+");
}

/*!
 * @internal
 * Internal function used to seek in the MP4 file
 * @param MP4File \c FILE handle for the MP4 file as a void pointer
 * @param pos Possition into the file to which to seek to
 */
int MP4EncSeek(void *MP4File, int64_t pos)
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
int MP4EncRead(void *MP4File, void *DataOut, int64_t DataOutLen, int64_t *Read, int64_t)
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
int MP4EncWrite(void *MP4File, const void *DataIn, int64_t DataInLen, int64_t *Written, int64_t)
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
int MP4EncClose(void *MP4File)
{
	return (fclose((FILE *)MP4File) == 0 ? FALSE : TRUE);
}

/*!
 * This function opens the file given by \c FileName for writing and returns a pointer
 * to the context of the opened file which must be used only by M4A_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *M4A_OpenW(const char *FileName)
{
	M4A_Enc_Intern *ret = NULL;

	ret = (M4A_Enc_Intern *)malloc(sizeof(M4A_Enc_Intern));
	if (ret == NULL)
		return ret;

	ret->err = false;
	ret->p_mp4 = MP4CreateProvider(FileName, &MP4EncFunctions, MP4_DETAILS_ERROR);// | MP4_DETAILS_WRITE_ALL);

	return ret;
}

/*!
 * This function sets the \c FileInfo structure for a M4A/MP4 file being encoded
 * @param p_AACFile A pointer to a file opened with \c M4A_OpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file
 * @warning This function must be called before using \c M4A_WriteBuffer()
 * @bug \p p_FI must not be \c NULL as no checking on the parameter is done. FIXME!
 *
 * @bug \p p_AACFile must not be \c NULL as no checking on the parameter is done. FIXME!
 */
bool M4A_SetFileInfo(void *p_AACFile, const fileInfo_t *const info)
{
	const MP4Tags *p_Tags;
	faacEncConfigurationPtr p_conf;
	uint8_t *ASC;
	unsigned long lenASC;
	M4A_Enc_Intern *p_AF = (M4A_Enc_Intern *)p_AACFile;

	p_Tags = MP4TagsAlloc();
	p_AF->Channels = info->channels;
	p_AF->p_enc = faacEncOpen(info->bitRate, info->channels, &p_AF->MaxInSamp, &p_AF->MaxOutByte);
	if (p_AF->p_enc == NULL)
	{
		p_AF->err = true;
		return false;
	}
	MP4SetTimeScale(p_AF->p_mp4, info->bitRate);
	p_AF->track = MP4AddAudioTrack(p_AF->p_mp4, info->bitRate, 1024);//p_AF->MaxInSamp / info->channels, MP4_MPEG4_AUDIO_TYPE);
	MP4SetAudioProfileLevel(p_AF->p_mp4, 0x0F);

	if (info->album)
		MP4TagsSetAlbum(p_Tags, info->album.get());
	if (info->artist)
		MP4TagsSetArtist(p_Tags, info->artist.get());
	if (info->title)
		MP4TagsSetName(p_Tags, info->title.get());

	MP4TagsSetEncodingTool(p_Tags, "libAudio " libAudioVersion);
	MP4TagsStore(p_Tags, p_AF->p_mp4);
	MP4TagsFree(p_Tags);

	p_conf = faacEncGetCurrentConfiguration(p_AF->p_enc);
	if (p_conf == NULL)
	{
		p_AF->err = true;
		return false;
	}
	p_conf->inputFormat = FAAC_INPUT_16BIT;
	p_conf->mpegVersion = MPEG4;
	p_conf->bitRate = 128000;
//	p_conf->bitRate = 64000;
	p_conf->outputFormat = 0;
	p_conf->useLfe = p_conf->useTns = p_conf->allowMidside = 0;
	p_conf->aacObjectType = LOW;//MAIN;
	p_conf->bandWidth = 0;
	p_conf->quantqual = 100;
	if (faacEncSetConfiguration(p_AF->p_enc, p_conf) == 0)
	{
		p_AF->err = true;
		return false;
	}
	if (faacEncGetDecoderSpecificInfo(p_AF->p_enc, &ASC, &lenASC) != 0)
	{
		p_AF->err = true;
		return false;
	}

	MP4SetTrackESConfiguration(p_AF->p_mp4, p_AF->track, ASC, lenASC);

	free(ASC);
	return true;
}

/*!
 * This function writes a buffer of audio to a M4A/MP4 file opened being encoded
 * @param p_AACFile A pointer to a file opened with \c M4A_OpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @attention Will not work unless \c M4A_SetFileInfo() has been called beforehand
 */
long M4A_WriteBuffer(void *p_AACFile, uint8_t *InBuffer, int nInBufferLen)
{
	M4A_Enc_Intern *p_AF = (M4A_Enc_Intern *)p_AACFile;
	int nOB, j = 0;
	uint8_t *OB = NULL;

	if (p_AF->p_enc == NULL)
		return -3;
	if (p_AF->err == true)
		return -4;

	if (nInBufferLen == -2)
	{
		OB = (uint8_t *)malloc(p_AF->MaxOutByte);

		nOB = faacEncEncode(p_AF->p_enc, NULL, 0, OB, p_AF->MaxOutByte);
		if (nOB > 0)
			MP4WriteSample(p_AF->p_mp4, p_AF->track, OB, nOB);
		nOB = faacEncEncode(p_AF->p_enc, NULL, 0, OB, p_AF->MaxOutByte);
		if (nOB > 0)
			MP4WriteSample(p_AF->p_mp4, p_AF->track, OB, nOB);
		nOB = faacEncEncode(p_AF->p_enc, NULL, 0, OB, p_AF->MaxOutByte);
		if (nOB > 0)
			MP4WriteSample(p_AF->p_mp4, p_AF->track, OB, nOB);
		nOB = faacEncEncode(p_AF->p_enc, NULL, 0, OB, p_AF->MaxOutByte);
		if (nOB > 0)
			MP4WriteSample(p_AF->p_mp4, p_AF->track, OB, nOB);

		free(OB);
		return -2;
	}

	OB = (uint8_t *)malloc(p_AF->MaxOutByte);
	while (j < nInBufferLen)
	{
		if (j + ((int)p_AF->MaxInSamp * 2) > nInBufferLen)
		{
			nOB = faacEncEncode(p_AF->p_enc, (int *)(InBuffer + j), (nInBufferLen - j) / 2, OB, p_AF->MaxOutByte);
			j = nInBufferLen;
		}
		else
		{
			nOB = faacEncEncode(p_AF->p_enc, (int *)(InBuffer + j), p_AF->MaxInSamp, OB, p_AF->MaxOutByte);
			j += p_AF->MaxInSamp * 2;
		}

		if (nOB < 0)
			break;
		if (nOB == 0)
			continue;

		MP4WriteSample(p_AF->p_mp4, p_AF->track, OB, nOB);
	}
	free(OB);

	return nInBufferLen;
}

/*!
 * Closes an open M4A/MP4 file
 * @param p_AACFile A pointer to a file opened with \c M4A_OpenW()
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_AACFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 */
int M4A_CloseFileW(void *p_AACFile)
{
	M4A_Enc_Intern *p_AF = (M4A_Enc_Intern *)p_AACFile;

	if (p_AF == NULL)
		return 0;
	MP4Close(p_AF->p_mp4);
	faacEncClose(p_AF->p_enc);

	free(p_AF);
	return 0;
}
