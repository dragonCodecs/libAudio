#include <sys/stat.h>
#include <sys/types.h>
#include <OptimFROG/OptimFROG.h>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadOptimFROG.cpp
 * @brief The implementation of the OptimFROG decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2009-2013
 */

/*!
 * @internal
 * Internal structure for holding the decoding context for a given OptimFROG file
 */
typedef struct _OFROG_Intern
{
	/*!
	 * @internal
	 * The OptimFROG file to decode
	 */
	FILE *f_OFG;
	/*!
	 * @internal
	 * The OptimFROG callbacks information handle
	 */
	ReadInterface *callbacks;
	/*!
	 * @internal
	 * The \c FileInfo for the OptimFROG file being decoded
	 */
	FileInfo *p_FI;
	/*!
	 * @internal
	 * The decoder context handle
	 */
	void *p_dec;
	/*!
	 * @internal
	 * The playback class instance for the AAC file
	 */
	playback_t *p_Playback;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	BYTE buffer[8192];
} OFROG_Intern;

/*!
 * @internal
 * \c __fclose() is the internal close callback for OptimFROG file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param Inst \c FILE handle for the OptimFROG file as a void pointer
 * @return A truth value indicating if closing the file succeeded or not
 */
uint8_t __fclose(void *Inst)
{
	return (fclose((FILE *)Inst) == 0 ? TRUE : FALSE);
}

/*!
 * @internal
 * \c __fread() is the internal read callback for OptimFROG file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param Inst \c FILE handle for the OptimFROG file as a void pointer
 * @param dest The buffer to read into
 * @param count The number of bytes to read into the buffer
 * @return The return result of \c fread()
 */
int __fread(void *Inst, void *dest, UINT count)
{
	int ret = fread(dest, 1, count, (FILE *)Inst);
	int err = ferror((FILE *)Inst);

	if (err == 0)
		return ret;

	return -1;
}

/*!
 * @internal
 * \c __feof() is the internal end-of-file callback for OptimFROG file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param Inst \c FILE handle for the OptimFROG file as a void pointer
 * @return A truth value indicating if we have reached the end of the file or not
 */
uint8_t __feof(void *Inst)
{
	return (feof((FILE *)Inst) == 0 ? FALSE : TRUE);
}

/*!
 * @internal
 * \c __fseekable() is the internal callback for determining if an OptimFROG file being
 * decoded can be seeked on or not. \n This does two things: \n
 * - It prevents nasty things from happening on Windows thanks to the run-time mess there
 * - It uses \c fseek() as a no-operation to determine if we can seek or not.
 *
 * @param Inst \c FILE handle for the OptimFROG file as a void pointer
 * @return A truth value giving if seeking can work or not
 */
uint8_t __fseekable(void *Inst)
{
	return (fseek((FILE *)Inst, 0, SEEK_CUR) == 0 ? TRUE : FALSE);
}

/*!
 * @internal
 * \c __flen() is the internal file length callback for OptimFROG file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param Inst \c FILE handle for the OptimFROG file as a void pointer
 * @return A 64-bit integer giving the length of the file in bytes
 */
__int64 __flen(void *Inst)
{
	struct stat stats;

	fstat(fileno((FILE *)Inst), &stats);
	return stats.st_size;
}

/*!
 * @internal
 * \c __ftell() is the internal read possition callback for OptimFROG file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param Inst \c FILE handle for the OptimFROG file as a void pointer
 * @return A 64-bit integerAn integer giving the read possition of the file in bytes
 */
__int64 __ftell(void *Inst)
{
	return ftell((FILE *)Inst);
}

/*!
 * @internal
 * \c __fseek() is the internal seek callback for OptimFROG file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param Inst \c FILE handle for the OptimFROG file as a void pointer
 * @param pos The offset through the file to which to seek to
 * @return A truth value giving if the seek succeeded or not
 */
uint8_t __fseek(void *Inst, __int64 pos)
{
	return (fseek((FILE *)Inst, (long)pos, SEEK_SET) == 0 ? TRUE : FALSE);
}

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by OptimFROG_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *OptimFROG_OpenR(const char *FileName)
{
	OFROG_Intern *ret;
	FILE *f_OFG;

	ret = (OFROG_Intern *)malloc(sizeof(OFROG_Intern));
	if (ret == NULL)
		return ret;

	f_OFG = fopen(FileName, "rb");
	if (f_OFG == NULL)
		return f_OFG;

	ret->f_OFG = f_OFG;
	ret->p_dec = OptimFROG_createInstance();

	ret->callbacks = (ReadInterface *)malloc(sizeof(ReadInterface));
	ret->callbacks->close = __fclose;
	ret->callbacks->read = __fread;
	ret->callbacks->eof = __feof;
	ret->callbacks->seekable = __fseekable;
	ret->callbacks->length = __flen;
	ret->callbacks->getPos = __ftell;
	ret->callbacks->seek = __fseek;

	OptimFROG_openExt(ret->p_dec, ret->callbacks, ret->f_OFG, TRUE);

	return ret;
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_OFGFile A pointer to a file opened with \c OptimFROG_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c OptimFROG_Play() or \c OptimFROG_FillBuffer()
 * @bug \p p_OFGFile must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *OptimFROG_GetFileInfo(void *p_OFGFile)
{
	FileInfo *ret = NULL;
	OFROG_Intern *p_OF = (OFROG_Intern *)p_OFGFile;
	OptimFROG_Info *p_OFI = (OptimFROG_Info *)malloc(sizeof(OptimFROG_Info));
	OptimFROG_Tags *p_OFT = (OptimFROG_Tags *)malloc(sizeof(OptimFROG_Tags));

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	OptimFROG_getInfo(p_OF->p_dec, p_OFI);
	ret->Channels = p_OFI->channels;
	ret->BitRate = p_OFI->samplerate;
	ret->BitsPerSample = p_OFI->bitspersample;
	ret->TotalTime = p_OFI->length_ms / 1000;
	p_OF->p_FI = ret;

	/*OptimFROG_getTags(p_OF->p_dec, p_OFT);
	for (UINT i = 0; i < p_OFT->keyCount; i++)
	{
		if (p_OFT->keys[i] == "title")
		{
			ret->Title = strdup(p_OFT->values[i]);
		}
	}
	OptimFROG_freeTags(p_OFT);*/

	free(p_OFI);
	free(p_OFT);

	if (ExternalPlayback == 0)
		p_OF->p_Playback = new playback_t(p_OFGFile, OptimFROG_FillBuffer, p_OF->buffer, 8192, ret);

	return ret;
}

/*!
 * Closes an opened audio file
 * @param p_OFGFile A pointer to a file opened with \c OptimFROG_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_MPCFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_OFGFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int OptimFROG_CloseFileR(void *p_OFGFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)p_OFGFile;
	int ret = 0;

	delete p_OF->p_Playback;

	OptimFROG_close(p_OF->p_dec);
	ret = fclose(p_OF->f_OFG);

	free(p_OF->callbacks);

	return ret;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_OFGFile A pointer to a file opened with \c OptimFROG_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_OFGFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long OptimFROG_FillBuffer(void *p_OFGFile, BYTE *OutBuffer, int nOutBufferLen)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)p_OFGFile;
	BYTE *OBuff = OutBuffer;
	static bool eof = false;
	if (eof == true)
		return -2;

	while (OBuff - OutBuffer < nOutBufferLen && eof == false)
	{
		short *out = (short *)p_OF->buffer;
		int nSamples = (8192 / p_OF->p_FI->Channels) / (p_OF->p_FI->BitsPerSample / 8);
		int ret = OptimFROG_read(p_OF->p_dec, OBuff, nSamples, TRUE);
		if (ret < nSamples)
			eof = true;

		memcpy(OBuff, out, 8192);
		OBuff += 8192;
	}

	return (OBuff - OutBuffer);
}

/*!
 * Plays an opened OptimFROG file using OpenAL on the default audio device
 * @param p_OFGFile A pointer to a file opened with \c OptimFROG_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c OptimFROG_OpenR() used to open the file at \p p_OFGFile,
 * this function will do nothing.
 * @bug \p p_OFGFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_OFGFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void OptimFROG_Play(void *p_OFGFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)p_OFGFile;

	p_OF->p_Playback->play();
}

void OptimFROG_Pause(void *p_OFGFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)p_OFGFile;

	p_OF->p_Playback->pause();
}

void OptimFROG_Stop(void *p_OFGFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)p_OFGFile;

	p_OF->p_Playback->stop();
}

/*!
 * Checks the file given by \p FileName for whether it is an OptimFROG
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an OptimFROG file or not
 */
bool Is_OptimFROG(const char *FileName)
{
	FILE *f_OFG = fopen(FileName, "rb");
	char OFGSig[4];

	if (f_OFG == NULL)
		return false;

	fread(OFGSig, 4, 1, f_OFG);
	fclose(f_OFG);

	if (strncmp(OFGSig, "OFR ", 4) != 0)
		return false;

	return true;
}

/*!
 * @internal
 * This structure controls decoding OptimFROG files when using the high-level API on them
 */
API_Functions OptimFROGDecoder =
{
	OptimFROG_OpenR,
	nullptr,
	OptimFROG_GetFileInfo,
	nullptr,
	OptimFROG_FillBuffer,
	nullptr,
	OptimFROG_CloseFileR,
	nullptr,
	OptimFROG_Play,
	OptimFROG_Pause,
	OptimFROG_Stop
};
