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
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	/*!
	 * @internal
	 * The internal transfer data buffer used due to how WavPack's decoder
	 * library outputs data
	 */
	std::array<int32_t, 1024> decodeBuffer; // This allocates 1 page for the buffer.
	/*!
	 * @internal
	 * @var int samplesUsed
	 * The number of samples used so far from the current sample buffer
	 * @var int sampleCount
	 * The total number of samples in the current sample buffer
	 */
	uint32_t sampleCount, samplesUsed;
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;
	/*!
	 * @internal
	 * The WavPack Corrections file to decode
	 */
	fd_t wvcFileFD;
	/*!
	 * @internal
	 * The WavPack callbacks/reader information handle
	 */
	WavpackStreamReader callbacks;

	decoderContext_t(std::string fileName) noexcept;
	~decoderContext_t() noexcept;
	std::unique_ptr<char []> readTag(const char *const tag) noexcept;
	void nextFrame(const uint8_t channels) noexcept;
	void *wvcFile() noexcept WARN_UNUSED { return wvcFileFD.valid() ? &wvcFileFD : nullptr; }
	static fd_t wvcFile(std::string &fileName) noexcept;
};

/*!
 * @internal
 * Internal structure for holding the decoding context for a given WavPack file
 */
typedef struct _WavPack_Intern
{
	/*!
	 * @internal
	 * The error feedback buffer needed for various WavPack call
	 */
	char err[80];

	wavPack_t inner;

	_WavPack_Intern(fd_t &&fd, const char *const fileName) : inner(std::move(fd), fileName) { }
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
int32_t f_fread_wp(void *filePtr, void *buffer, int32_t length)
{
	const fd_t &file = *static_cast<fd_t *>(filePtr);
	return file.read(buffer, length, nullptr);
}

/*!
 * @internal
 * \c f_ftell() is the internal read possition callback for WavPack file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file \c FILE handle for the WavPack file as a void pointer
 * @return An integer giving the read possition of the file in bytes
 */
uint32_t f_ftell(void *filePtr)
{
	const fd_t &file = *static_cast<fd_t *>(filePtr);
	return file.tell();
}

/*!
 * @internal
 * \c f_fseek_abs() is the internal absolute seek callback for WavPack file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file \c FILE handle for the WavPack file as a void pointer
 * @param pos The offset through the file to which to seek to
 * @return A truth value giving if the seek succeeded or not
 */
int f_fseek_abs(void *filePtr, uint32_t offset)
{
	const fd_t &file = *static_cast<fd_t *>(filePtr);
	return file.seek(offset, SEEK_SET);
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
int f_fseek_rel(void *filePtr, int32_t offset, int mode)
{
	const fd_t &file = *static_cast<fd_t *>(filePtr);
	return file.seek(offset, mode);
}

int f_fungetc(void *filePtr, int)
{
	const fd_t &file = *static_cast<fd_t *>(filePtr);
	return file.seek(-1, SEEK_CUR);
}

/*!
 * @internal
 * \c f_flen() is the internal file length callback for WavPack file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file \c FILE handle for the WavPack file as a void pointer
 * @return An integer giving the length of the file in bytes
 */
uint32_t f_flen(void *filePtr)
{
	const fd_t &file = *static_cast<fd_t *>(filePtr);
	return file.length();
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
int f_fcanseek(void *filePtr)
{
	const fd_t &file = *static_cast<fd_t *>(filePtr);
	return file.tell() != -1;
}

wavPack_t::wavPack_t(fd_t &&fd, const char *const fileName) noexcept : audioFile_t(audioType_t::wavPack, std::move(fd)),
	ctx(makeUnique<decoderContext_t>(fileName)) { }
wavPack_t::decoderContext_t::decoderContext_t(std::string fileName) noexcept : decoder{nullptr}, playbackBuffer{},
	decodeBuffer{}, sampleCount{0}, samplesUsed{0}, eof{false}, wvcFileFD{wvcFile(fileName)},
	callbacks{f_fread_wp, f_ftell, f_fseek_abs, f_fseek_rel, f_fungetc, f_flen, f_fcanseek, nullptr} { }

fd_t wavPack_t::decoderContext_t::wvcFile(std::string &fileName) noexcept
{
	fileName += 'c';
	return {fileName.data(), O_RDONLY | O_NOCTTY};
}

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
	std::unique_ptr<WavPack_Intern> ret = makeUnique<WavPack_Intern>(fd_t(FileName, O_RDONLY | O_NOCTTY), FileName);
	if (!ret || !ret->inner.context())
		return nullptr;
	fd_t &fileDesc = const_cast<fd_t &>(ret->inner.fd());
	auto &ctx = *ret->inner.context();
	fileInfo_t &info = ret->inner.fileInfo();

	ctx.decoder = WavpackOpenFileInputEx(&ctx.callbacks, &fileDesc,
		ctx.wvcFile(), ret->err, OPEN_NORMALIZE | OPEN_TAGS, 15);

	info.channels = WavpackGetNumChannels(ctx.decoder);
	info.bitsPerSample = WavpackGetBitsPerSample(ctx.decoder);
	info.bitRate = WavpackGetSampleRate(ctx.decoder);
	info.album = ctx.readTag("album");
	info.artist = ctx.readTag("artist");
	info.title = ctx.readTag("title");

	if (ExternalPlayback == 0)
		ret->inner.player(makeUnique<playback_t>(&ret->inner, audioFillBuffer, ctx.playbackBuffer, 8192, info));

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
	delete p_WF;
	return 0;
}

void wavPack_t::decoderContext_t::nextFrame(const uint8_t channels) noexcept
{
	sampleCount = WavpackUnpackSamples(decoder, decodeBuffer.data(),
		decodeBuffer.size() / channels) * channels;
	samplesUsed = 0;
	eof = sampleCount == 0;
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
	return audioFillBuffer(&p_WF->inner, OutBuffer, nOutBufferLen);
}

int64_t wavPack_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	const auto buffer = static_cast<uint8_t *>(bufferPtr);
	uint32_t offset = 0;
	const fileInfo_t &info = fileInfo();
	auto &ctx = *context();

	if (ctx.eof)
		return -2;
	while (offset < length && !ctx.eof)
	{
		if (ctx.samplesUsed == ctx.sampleCount)
			ctx.nextFrame(info.channels);

		auto playbackBuffer = reinterpret_cast<int16_t *>(buffer + offset);
		uint32_t count = ctx.samplesUsed * info.channels, index = 0;
		for (uint32_t i = ctx.samplesUsed; i < ctx.sampleCount; i += info.channels)
		{
			for (uint8_t channel = 0; channel < info.channels; ++channel)
				playbackBuffer[index++] = int16_t(ctx.decodeBuffer[count++]);
		}
		ctx.samplesUsed += index;
		offset += index * sizeof(int16_t);
	}

	return offset;
}

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
