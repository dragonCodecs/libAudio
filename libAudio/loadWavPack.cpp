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
	/*!
	 * @internal
	 * The decoder context handle
	 */
	WavpackContext *decoder;
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;
	/*!
	 * @internal
	 * The WavPack callbacks/reader information handle
	 */
	WavpackStreamReader callbacks;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
	std::unique_ptr<char []> readTag(const char *const tag) noexcept;
};

/*!
 * @internal
 * Internal structure for holding the decoding context for a given WavPack file
 */
typedef struct _WavPack_Intern
{
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
	 * The error feedback buffer needed for various WavPack call
	 */
	char err[80];

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
wavPack_t::decoderContext_t::decoderContext_t() noexcept : decoder{nullptr}, eof{false},
	callbacks{f_fread_wp, f_ftell, f_fseek_abs, f_fseek_rel, f_fungetc, f_flen, f_fcanseek, nullptr} { }

std::unique_ptr<char []> wavPack_t::decoderContext_t::readTag(const char *const tag) noexcept
{
	const uint32_t length = WavpackGetTagItem(decoder, tag, nullptr, 0);
	if (length)
	{
		auto result = makeUnique<char []>(length + 1);
		if (!result)
			return nullptr;
		WavpackGetTagItem(decoder, tag, result.get(), length + 1);
		return result;
	}
	return nullptr;
}

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
	if (!ret || !ret->inner.context())
		return nullptr;
	auto &ctx = *ret->inner.context();
	fileInfo_t &info = ret->inner.fileInfo();

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
	ctx.decoder = WavpackOpenFileInputEx(&ctx.callbacks, f_WVP, f_WVPC, ret->err, OPEN_NORMALIZE | OPEN_TAGS, 15);

	info.channels = WavpackGetNumChannels(ctx.decoder);
	info.bitsPerSample = WavpackGetBitsPerSample(ctx.decoder);
	info.bitRate = WavpackGetSampleRate(ctx.decoder);
	info.album = ctx.readTag("album");
	info.artist = ctx.readTag("artist");
	info.title = ctx.readTag("title");

	if (ExternalPlayback == 0)
		ret->inner.player(makeUnique<playback_t>(ret.get(), WavPack_FillBuffer, ret->buffer, 8192, info));

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
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;
	return audioFileInfo(&p_WF->inner);
}

wavPack_t::decoderContext_t::~decoderContext_t() noexcept
	{ WavpackCloseFile(decoder); }

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

	if (p_WF->f_WVPC != NULL)
		fclose(p_WF->f_WVPC);

	ret = fclose(p_WF->f_WVP);

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
	uint32_t offset = 0;
	const fileInfo_t &info = p_WF->inner.fileInfo();
	auto &ctx = *p_WF->inner.context();

	if (ctx.eof)
		return -2;
	while (offset < nOutBufferLen && !ctx.eof)
	{
		short *out = (short *)p_WF->buffer;
		int j = WavpackUnpackSamples(ctx.decoder, p_WF->middlebuff,
			(4096 / info.channels)) * info.channels;

		if (j == 0)
			ctx.eof = true;

		for (int i = 0; i < j; )
		{
			for (uint8_t k = 0; k < info.channels; k++)
				out[i + k] = (short)p_WF->middlebuff[i + k];

			i += info.channels;
		}

		memcpy(OutBuffer + offset, out, j * 2);
		offset += j * 2;
	}

	return offset;
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
	p_WF->inner.play();
}

void WavPack_Pause(void *p_WVPFile)
{
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;
	p_WF->inner.pause();
}

void WavPack_Stop(void *p_WVPFile)
{
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;
	p_WF->inner.stop();
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
