#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WINDOWS
#include <wavpack.h>
#else
#include <wavpack/wavpack.h>
#endif
#include <string>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadWavPack.cpp
 * @brief The implementation of the WavPack decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2013
 */

struct wavPack_t::decoderContext_t final
{
	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
};

/*!
 * @internal
 * Internal structure for holding the decoding context for a given WavPack file
 */
typedef struct _WavPack_Intern
{
	/*!
	 * @internal
	 * The WavPack callbacks/reader information handle
	 */
	WavpackStreamReader *callbacks;
	/*!
	 * @internal
	 * The decoder context handle
	 */
	WavpackContext *p_dec;
	/*!
	 * @internal
	 * The WavPack file to decode
	 */
	FILE *f_WVP;
	/*!
	 * @internal
	 * The WavPack Corrections file to decode
	 */
	FILE *f_WVPC;
	/*!
	 * @internal
	 * The \c FileInfo for the WavPack file being decoded
	 */
	FileInfo *p_FI;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t buffer[8192];
	/*!
	 * @internal
	 * The internal transfer data buffer used due to how WavPack's decoder
	 * library outputs data
	 */
	int middlebuff[4096];
	/*!
	 * @internal
	 * The playback class instance for the WavPack file
	 */
	playback_t *p_Playback;
	/*!
	 * @internal
	 * The error feedback buffer needed for various WavPack call
	 */
	char err[80];
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;

	wavPack_t inner;
} WavPack_Intern;

/*!
 * @internal
 * \c f_fread_wp() is the internal read callback for WavPack file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file \c FILE handle for the WavPack file as a void pointer
 * @param data The buffer to read into
 * @param size The number of bytes to read into the buffer
 * @return The return result of \c fread()
 */
int f_fread_wp(void *p_file, void *data, int size)
{
	return fread(data, 1, size, (FILE *)p_file);
}

/*!
 * @internal
 * \c f_ftell() is the internal read possition callback for WavPack file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file \c FILE handle for the WavPack file as a void pointer
 * @return An integer giving the read possition of the file in bytes
 */
uint32_t f_ftell(void *p_file)
{
	return ftell((FILE *)p_file);
}

/*!
 * @internal
 * \c f_fseek_abs() is the internal absolute seek callback for WavPack file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file \c FILE handle for the WavPack file as a void pointer
 * @param pos The offset through the file to which to seek to
 * @return A truth value giving if the seek succeeded or not
 */
int f_fseek_abs(void *p_file, uint32_t pos)
{
	return fseek((FILE *)p_file, pos, SEEK_SET);
}

/*!
 * @internal
 * \c f_fseek_rel() is the internal any-place (relative) seek callback for WavPack file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file \c FILE handle for the WavPack file as a void pointer
 * @param pos The offset through the file to which to seek to
 * @param loc The location identifier to seek from
 * @return A truth value giving if the seek succeeded or not
 */
int f_fseek_rel(void *p_file, int pos, int loc)
{
	return fseek((FILE *)p_file, pos, loc);
}

int f_fungetc(void *p_file, int c)
{
	return ungetc(c, (FILE *)p_file);
}

/*!
 * @internal
 * \c f_flen() is the internal file length callback for WavPack file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file \c FILE handle for the WavPack file as a void pointer
 * @return An integer giving the length of the file in bytes
 */
uint32_t f_flen(void *p_file)
{
	struct stat stats;

	fstat(fileno((FILE *)p_file), &stats);
	return stats.st_size;
}

/*!
 * @internal
 * \c f_fcanseek() is the internal callback for determining if a WavPack file being
 * decoded can be seeked on or not. \n This does two things: \n
 * - It prevents nasty things from happening on Windows thanks to the run-time mess there
 * - It uses \c fseek() as a no-operation to determine if we can seek or not.
 *
 * @param p_file \c FILE handle for the WavPack file as a void pointer
 * @return A truth value giving if seeking can work or not
 */
int f_fcanseek(void *p_file)
{
	return (fseek((FILE *)p_file, 0, SEEK_CUR) == 0 ? TRUE : FALSE);
}

wavPack_t::wavPack_t() noexcept : audioFile_t(audioType_t::wavPack, {}), ctx(makeUnique<decoderContext_t>()) { }
wavPack_t::decoderContext_t::decoderContext_t() noexcept { }

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by WavPack_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *WavPack_OpenR(const char *FileName)
{
	FILE *f_WVP = NULL, *f_WVPC = NULL;

	std::unique_ptr<WavPack_Intern> ret = makeUnique<WavPack_Intern>();
	if (!ret)
		return nullptr;

	f_WVP = fopen(FileName, "rb");
	if (f_WVP == NULL)
		return f_WVP;

	f_WVPC = [](std::string fileName) noexcept -> FILE *
	{
		fileName += 'c';
		return fopen(fileName.data(), "rb");
	}(FileName);

	ret->f_WVP = f_WVP;
	ret->f_WVPC = f_WVPC;
	ret->callbacks = (WavpackStreamReader *)malloc(sizeof(WavpackStreamReader));
	ret->callbacks->read_bytes = f_fread_wp;
	ret->callbacks->get_pos = f_ftell;
	ret->callbacks->set_pos_abs = f_fseek_abs;
	ret->callbacks->set_pos_rel = f_fseek_rel;
	ret->callbacks->push_back_byte = f_fungetc;
	ret->callbacks->get_length = f_flen;
	ret->callbacks->can_seek = f_fcanseek;
	ret->callbacks->write_bytes = NULL;
	ret->p_dec = WavpackOpenFileInputEx(ret->callbacks, f_WVP, f_WVPC, ret->err, OPEN_NORMALIZE | OPEN_TAGS, 15);

	return ret.release();
}

/*!
 * This function gets the \c FileInfo structure for an opened WavPack file
 * @param p_WVPFile A pointer to a file opened with \c WavPack_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c WavPack_Play() or \c WavPack_FillBuffer()
 * @bug \p p_WVPFile must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *WavPack_GetFileInfo(void *p_WVPFile)
{
	FileInfo *ret = NULL;
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	p_WF->p_FI = ret;
	ret->Channels = WavpackGetNumChannels(p_WF->p_dec);
	ret->BitsPerSample = WavpackGetBitsPerSample(p_WF->p_dec);
	ret->BitRate = WavpackGetSampleRate(p_WF->p_dec);

	{
		int i = WavpackGetTagItem(p_WF->p_dec, "album", NULL, 0) + 1;
		if (i > 1)
		{
			ret->Album = (char *)malloc(i);
			WavpackGetTagItem(p_WF->p_dec, "album", (char *)ret->Album, i);
		}
	}
	{
		int i = WavpackGetTagItem(p_WF->p_dec, "artist", NULL, 0) + 1;
		if (i > 1)
		{
			ret->Artist = (char *)malloc(i);
			WavpackGetTagItem(p_WF->p_dec, "artist", (char *)ret->Artist, i);
		}
	}
	{
		int i = WavpackGetTagItem(p_WF->p_dec, "title", NULL, 0) + 1;
		if (i > 1)
		{
			ret->Title = (char *)malloc(i);
			WavpackGetTagItem(p_WF->p_dec, "title", (char *)ret->Title, i);
		}
	}

	if (ExternalPlayback == 0)
		p_WF->p_Playback = new playback_t(p_WVPFile, WavPack_FillBuffer, p_WF->buffer, 8192, ret);

	return ret;
}

wavPack_t::decoderContext_t::~decoderContext_t() noexcept { }

/*!
 * Closes an opened audio file
 * @param p_WVPFile A pointer to a file opened with \c WavPack_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_WVPFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_WVPFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int WavPack_CloseFileR(void *p_WVPFile)
{
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;
	int ret = 0;

	delete p_WF->p_Playback;

	WavpackCloseFile(p_WF->p_dec);

	if (p_WF->f_WVPC != NULL)
		fclose(p_WF->f_WVPC);

	ret = fclose(p_WF->f_WVP);

	free(p_WF->callbacks);
	delete p_WF;

	return ret;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_WVPFile A pointer to a file opened with \c WavPack_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_WVPFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long WavPack_FillBuffer(void *p_WVPFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;
	uint8_t *OBuff = OutBuffer;
	if (p_WF->eof == true)
		return -2;

	while (OBuff - OutBuffer < nOutBufferLen && p_WF->eof == false)
	{
		short *out = (short *)p_WF->buffer;
		int j = WavpackUnpackSamples(p_WF->p_dec, p_WF->middlebuff, (4096 / p_WF->p_FI->Channels)) * p_WF->p_FI->Channels;

		if (j == 0)
			p_WF->eof = true;

		for (int i = 0; i < j; )
		{
			for (uint32_t k = 0; k < p_WF->p_FI->Channels; k++)
				out[i + k] = (short)p_WF->middlebuff[i + k];

			i += p_WF->p_FI->Channels;
		}

		memcpy(OBuff, out, j * 2);
		OBuff += j * 2;
	}

	return (OBuff - OutBuffer);
}

int64_t wavPack_t::fillBuffer(void *const buffer, const uint32_t length) { return -1; }

/*!
 * Plays an opened WavPack file using OpenAL on the default audio device
 * @param p_WVPFile A pointer to a file opened with \c WavPack_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c WavPack_OpenR() used to open the file at \p p_WVPFile,
 * this function will do nothing.
 * @bug \p p_WVPFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_WVPFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void WavPack_Play(void *p_WVPFile)
{
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;

	p_WF->p_Playback->play();
}

void WavPack_Pause(void *p_WVPFile)
{
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;

	p_WF->p_Playback->pause();
}

void WavPack_Stop(void *p_WVPFile)
{
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;

	p_WF->p_Playback->stop();
}

/*!
 * Checks the file given by \p FileName for whether it is an WavPack
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an WavPack file or not
 */
bool Is_WavPack(const char *FileName) { return wavPack_t::isWavPack(FileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a MP3
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a MP3 file or not
 */
bool wavPack_t::isWavPack(const int32_t fd) noexcept
{
	char wavPackSig[4];
	if (fd == -1 ||
		read(fd, wavPackSig, 4) != 4 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		strncmp(wavPackSig, "wvpk", 4) != 0)
		return false;
	return true;
}

/*!
 * Checks the file given by \p fileName for whether it is a MP3
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a MP3 file or not
 */
bool wavPack_t::isWavPack(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isWavPack(file);
}

/*!
 * @internal
 * This structure controls decoding WavPack files when using the high-level API on them
 */
API_Functions WavPackDecoder =
{
	WavPack_OpenR,
	WavPack_GetFileInfo,
	WavPack_FillBuffer,
	WavPack_CloseFileR,
	WavPack_Play,
	WavPack_Pause,
	WavPack_Stop
};
