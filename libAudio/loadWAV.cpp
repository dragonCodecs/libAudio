#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <windows.h>
#else
#endif

#include "libAudio.h"
#include "libAudio_Common.h"

/*!
 * @internal
 * @file loadWAV.cpp
 * The implementation of the WAV decoder API
 * @author Richard Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2011
 */

/*!
 * @internal
 * Internal structure for holding the decoding context for a given WAV file
 */
typedef struct _WAV_Intern
{
	/*!
	 * @internal
	 * The WAV file to decode
	 */
	FILE *f_WAV;
	/*!
	 * @internal
	 * The internal count of how long the file is in bytes
	 * as read from the data chunk
	 */
	int fLen;
	/*!
	 * @internal
	 * The compression flags read from the WAV file
	 */
	short compression;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	BYTE buffer[8192];
	/*!
	 * @internal
	 * The playback class instance for the WAV file
	 */
	Playback *p_Playback;
	/*!
	 * @internal
	 * The \c FileInfo for the WAV file being decoded
	 */
	FileInfo *p_FI;
	/*!
	 * @internal
	 * The byte possition that the final byte of the file should be at in the file,
	 * calculated fresh from fLen and an \c ftell() call just after reading the data
	 * for fLen
	 */
	int DataEnd;
	/*!
	 * @internal
	 * A flag indicating if this WAV's data is floating point
	 */
	bool usesFloat;
} WAV_Intern;

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by WAV_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *WAV_OpenR(const char *FileName)
{
	WAV_Intern *ret = NULL;
	char tmp[4];

	FILE *f_WAV = fopen(FileName, "rb");
	if (f_WAV == NULL)
		return ret;

	ret = (WAV_Intern *)malloc(sizeof(WAV_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(WAV_Intern));
	ret->f_WAV = f_WAV;

	fread(tmp, 4, 1, f_WAV);
	if (strncmp(tmp, "RIFF", 4) != 0)
		return NULL;

	/*{
		int fLen;
		struct stat Stats;
		fstat(fileno(f_WAV), &Stats);
		fLen = Stats.st_size;
		fread(&ret->fLen, 4, 1, f_WAV);
		if (ret->fLen != fLen - 8)
			return NULL;
	}*/
	fread(&ret->fLen, 4, 1, f_WAV);
	fread(tmp, 4, 1, f_WAV);
	if (strncmp(tmp, "WAVE", 4) != 0)
		return NULL;

	return ret;
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_WAVFile A pointer to a file opened with \c WAV_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c WAV_Play() or \c WAV_FillBuffer()
 * @bug \p p_WAVFile must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *WAV_GetFileInfo(void *p_WAVFile)
{
	WAV_Intern *p_WF = (WAV_Intern *)p_WAVFile;
	FILE *f_WAV = p_WF->f_WAV;
	FileInfo *ret = NULL;
	char tmp[4];
	int fmtLen;
	int i;

	fread(tmp, 4, 1, f_WAV);
	while (strncmp(tmp, "fmt ", 4) != 0)
	{
		int len;
		fread(&len, 4, 1, f_WAV);
		fseek(f_WAV, len, SEEK_CUR);
		fread(tmp, 4, 1, f_WAV);
	}

	fread(&fmtLen, 4, 1, f_WAV);
	if (fmtLen < 16) // Must be at least 16, probably 18.
		return ret;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	memset(ret, 0x00, sizeof(FileInfo));
	p_WF->p_FI = ret;

	fread(&p_WF->compression, 2, 1, f_WAV);
	p_WF->compression == 3 ? p_WF->usesFloat = true : p_WF->usesFloat = false;
	fread(&ret->Channels, 2, 1, f_WAV);
	fread(&ret->BitRate, 4, 1, f_WAV);
	fread(tmp, 4, 1, f_WAV);
	fread(tmp, 2, 1, f_WAV);
	fread(&ret->BitsPerSample, 2, 1, f_WAV);
	fmtLen -= 16;
	// Currently we do not care if the file has extra data, we're only looking to work with PCM.
	for (i = 0; i < fmtLen; i++)
		fread(tmp, 1, 1, f_WAV);

	fread(tmp, 4, 1, f_WAV);
	while (strncmp(tmp, "data", 4) != 0)
	{
		int len;
		fread(&len, 4, 1, f_WAV);
		fseek(f_WAV, len, SEEK_CUR);
		fread(tmp, 4, 1, f_WAV);
	}
	fread(&p_WF->DataEnd, 4, 1, f_WAV);
	p_WF->DataEnd += ftell(f_WAV);
	ret->TotalTime = (float)p_WF->DataEnd;
	ret->TotalTime /= ret->Channels;
	ret->TotalTime /= (ret->BitsPerSample / 8);

	if (ExternalPlayback == 0)
		p_WF->p_Playback = new Playback(ret, WAV_FillBuffer, p_WF->buffer, 8192, p_WAVFile);

	return ret;
}

/*!
 * Closes an opened audio file
 * @param p_WAVFile A pointer to a file opened with \c WAV_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_WAVFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_WAVFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int WAV_CloseFileR(void *p_WAVFile)
{
	WAV_Intern *p_WF = (WAV_Intern *)p_WAVFile;
	FILE *f_WAV = p_WF->f_WAV;
	int ret;

	delete p_WF->p_Playback;

	ret = fclose(f_WAV);
	free(p_WF);
	return ret;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_WAVFile A pointer to a file opened with \c WAV_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_WAVFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long WAV_FillBuffer(void *p_WAVFile, BYTE *OutBuffer, int nOutBufferLen)
{
	WAV_Intern *p_WF = (WAV_Intern *)p_WAVFile;
	FILE *f_WAV = p_WF->f_WAV;
	int ret = 0;

	if (feof(f_WAV) != FALSE || ftell(f_WAV) >= p_WF->DataEnd)
		return -2;
	// 16-bit short reader
	if (p_WF->usesFloat == false && p_WF->p_FI->BitsPerSample == 16)
	{
		for (int i = 0; i < nOutBufferLen && ftell(f_WAV) < p_WF->DataEnd; i += 2)
			ret += fread(OutBuffer + ret, 1, 2, f_WAV);
	}
	// 8-bit char reader
	else if (p_WF->usesFloat == false && p_WF->p_FI->BitsPerSample == 8)
	{
		for (int i = 0; i < nOutBufferLen && ftell(f_WAV) < p_WF->DataEnd; i += 2)
		{
			short in;
			fread(&in, 1, 1, f_WAV);
			// scale the values up to 16-bit
			*((short *)(OutBuffer + ret)) = in * 257;
			ret += 2;
		}
	}
	// 32-bit float reader
	else if (p_WF->usesFloat == true && p_WF->p_FI->BitsPerSample == 32)
	{
		for (int i = 0; i < nOutBufferLen && ftell(f_WAV) < p_WF->DataEnd; i += 2)
		{
			float in;
			fread(&in, 4, 1, f_WAV);
			if (in > -2.0F && in < 2.0F)
				*((short *)(OutBuffer + ret)) = (short)(in * 32767.5F);
			else
				*((short *)(OutBuffer + ret)) = (short)((in / ((float)((int)in))) * 65535.0F);
			ret += 2;
		}
	}
	// 32-bit int reader
	else if (p_WF->usesFloat == false && p_WF->p_FI->BitsPerSample == 32)
	{
		for (int i = 0; i < nOutBufferLen && ftell(f_WAV) < p_WF->DataEnd; i += 2)
		{
			int in;
			fread(&in, 4, 1, f_WAV);
			*((short *)(OutBuffer + ret)) = (short)(in / 65535);
		}
	}
	// 24-bit int reader
	else if (p_WF->usesFloat == false && p_WF->p_FI->BitsPerSample == 24)
	{
		for (int i = 0; i < nOutBufferLen && ftell(f_WAV) < p_WF->DataEnd; i += 2)
		{
			int in;
			fread(&in, 3, 1, f_WAV);
			*((short *)(OutBuffer + ret)) = (short)(((float)in / 65536.0F) * 256.0F);
			ret += 2;
		}
	}
	// 24-bit float reader
	else if (p_WF->usesFloat == true && p_WF->p_FI->BitsPerSample == 24)
	{
		for (int i = 0; i < nOutBufferLen && ftell(f_WAV) < p_WF->DataEnd; i += 2)
		{
			float in;
			fread(&in, 3, 1, f_WAV);
			*((short *)(OutBuffer + ret)) = (short)(in * 65535.0F);
			ret += 2;
		}
	}
	else
		return -1;

	return ret;
}

/*!
 * Plays an opened WAV file using OpenAL on the default audio device
 * @param p_WAVFile A pointer to a file opened with \c WAV_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c WAV_OpenR() used to open the file at \p p_WAVFile,
 * this function will do nothing.
 * @bug \p p_WAVFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_WAVFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void WAV_Play(void *p_WAVFile)
{
	WAV_Intern *p_WF = (WAV_Intern *)p_WAVFile;

	p_WF->p_Playback->Play();
}

/*!
 * Checks the file given by \p FileName for whether it is an WAV
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an WAV file or not
 */
bool Is_WAV(const char *FileName)
{
	FILE *f_WAV = fopen(FileName, "rb");
	char RIFFSig[4];
	char WAVESig[4];
	if (f_WAV == NULL)
		return false;

	fread(RIFFSig, 4, 1, f_WAV);
	fseek(f_WAV, 4, SEEK_CUR);
	fread(WAVESig, 4, 1, f_WAV);
	fclose(f_WAV);

	if (strncmp(RIFFSig, "RIFF", 4) != 0 ||
		strncmp(WAVESig, "WAVE", 4) != 0)
		return false;
	else
		return true;
}

/*!
 * @internal
 * This structure controls decoding WAV files when using the high-level API on them
 */
API_Functions WAVDecoder =
{
	WAV_OpenR,
	WAV_GetFileInfo,
	WAV_FillBuffer,
	WAV_CloseFileR,
	WAV_Play
};
