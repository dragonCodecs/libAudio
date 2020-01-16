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
	ReadInterface callbacks;
	/*!
	 * @internal
	 * The \c FileInfo for the OptimFROG file being decoded
	 */
	fileInfo_t info;
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
	uint8_t buffer[8192];
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
	return (fclose((FILE *)Inst) == 0 ? 1 : 0);
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
int __fread(void *Inst, void *dest, uint32_t count)
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
	return (feof((FILE *)Inst) == 0 ? 1 : 0);
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
	return (fseek((FILE *)Inst, 0, SEEK_CUR) == 0 ? 1 : 0);
}

/*!
 * @internal
 * \c __flen() is the internal file length callback for OptimFROG file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param Inst \c FILE handle for the OptimFROG file as a void pointer
 * @return A 64-bit integer giving the length of the file in bytes
 */
int64_t __flen(void *Inst)
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
int64_t __ftell(void *Inst)
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
uint8_t __fseek(void *Inst, int64_t pos)
{
	return (fseek((FILE *)Inst, (long)pos, SEEK_SET) == 0 ? 1 : 0);
}

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by OptimFROG_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *optimFROGOpenR(const char *fileName)
{
	std::unique_ptr<OFROG_Intern> ret = makeUnique<OFROG_Intern>();
	if (!ret)
		return nullptr;
	fileInfo_t &info = ret->info;

	ret->f_OFG = fopen(fileName, "rb");
	if (!ret->f_OFG)
		return nullptr;
	ret->p_dec = OptimFROG_createInstance();

	ret->callbacks = {};
	ret->callbacks.close = __fclose;
	ret->callbacks.read = __fread;
	ret->callbacks.eof = __feof;
	ret->callbacks.seekable = __fseekable;
	ret->callbacks.length = __flen;
	ret->callbacks.getPos = __ftell;
	ret->callbacks.seek = __fseek;
	OptimFROG_openExt(ret->p_dec, &ret->callbacks, ret->f_OFG, true);

	OptimFROG_Info ofgInfo;
	OptimFROG_getInfo(ret->p_dec, &ofgInfo);
	info.channels = ofgInfo.channels;
	info.bitRate = ofgInfo.samplerate;
	info.bitsPerSample = ofgInfo.bitspersample;
	info.totalTime = ofgInfo.length_ms / 1000;

	return ret.release();
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param ofrgFile A pointer to a file opened with \c optimFROGOpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c OptimFROG_Play() or \c OptimFROG_FillBuffer()
 * @bug \p ofrgFile must not be NULL as no checking on the parameter is done. FIXME!
 */
const fileInfo_t *OptimFROG_GetFileInfo(void *ofrgFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)ofrgFile;
	const fileInfo_t &info = p_OF->info;
#if 0
	OptimFROG_Tags *p_OFT = (OptimFROG_Tags *)malloc(sizeof(OptimFROG_Tags));

	/*OptimFROG_getTags(p_OF->p_dec, p_OFT);
	for (UINT i = 0; i < p_OFT->keyCount; i++)
	{
		if (p_OFT->keys[i] == "title")
		{
			ret->Title = strdup(p_OFT->values[i]);
		}
	}
	OptimFROG_freeTags(p_OFT);*/

	free(p_OFT);
#endif

	if (ExternalPlayback == 0)
		p_OF->p_Playback = new playback_t(ofrgFile, OptimFROG_FillBuffer, p_OF->buffer, 8192, info);

	return &p_OF->info;
}

/*!
 * Closes an opened audio file
 * @param ofrgFile A pointer to a file opened with \c optimFROGOpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_MPCFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p ofrgFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int OptimFROG_CloseFileR(void *ofrgFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)ofrgFile;
	int ret = 0;

	delete p_OF->p_Playback;

	OptimFROG_close(p_OF->p_dec);
	ret = fclose(p_OF->f_OFG);

	delete p_OF;
	return ret;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param ofrgFile A pointer to a file opened with \c optimFROGOpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p ofrgFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long OptimFROG_FillBuffer(void *ofrgFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)ofrgFile;
	uint8_t *OBuff = OutBuffer;
	static bool eof = false;
	if (eof == true)
		return -2;

	while (OBuff - OutBuffer < nOutBufferLen && eof == false)
	{
		short *out = (short *)p_OF->buffer;
		int nSamples = (8192 / p_OF->info.channels) / (p_OF->info.bitsPerSample / 8);
		int ret = OptimFROG_read(p_OF->p_dec, OBuff, nSamples, true);
		if (ret < nSamples)
			eof = true;

		memcpy(OBuff, out, 8192);
		OBuff += 8192;
	}

	return (OBuff - OutBuffer);
}

/*!
 * Plays an opened OptimFROG file using OpenAL on the default audio device
 * @param ofrgFile A pointer to a file opened with \c optimFROGOpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c optimFROGOpenR() used to open the file at \p ofrgFile,
 * this function will do nothing.
 * @bug \p ofrgFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p ofrgFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void OptimFROG_Play(void *ofrgFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)ofrgFile;

	p_OF->p_Playback->play();
}

void OptimFROG_Pause(void *ofrgFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)ofrgFile;

	p_OF->p_Playback->pause();
}

void OptimFROG_Stop(void *ofrgFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)ofrgFile;

	p_OF->p_Playback->stop();
}

/*!
 * Checks the file given by \p fileName for whether it is an OptimFROG
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an OptimFROG file or not
 */
bool isOptimFROG(const char *fileName)
{
	FILE *f_OFG = fopen(fileName, "rb");
	char OFGSig[4];

	if (f_OFG == NULL)
		return false;

	fread(OFGSig, 4, 1, f_OFG);
	fclose(f_OFG);

	if (memcmp(OFGSig, "OFR ", 4) != 0)
		return false;

	return true;
}

/*!
 * @internal
 * This structure controls decoding OptimFROG files when using the high-level API on them
 */
API_Functions OptimFROGDecoder =
{
	optimFROGOpenR,
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
