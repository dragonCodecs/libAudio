#include <opus/opus.h>

#include "libAudio.h"
#include "libAudio.hxx"
#include "oggCommon.hxx"

/*!
 * @internal
 * @file loadOggOpus.cxx
 * @brief The implementation of the Ogg|Opus decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2019
 */

/*!
 * @internal
 * Internal structure for holding the decoding context for a given Ogg|Opus file
 */
struct oggOpus_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The decoder context handle and handle to the Ogg|Opus
	 * file being decoded
	 */
	OpusDecoder *decoder;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	bool eof;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
};

oggOpus_t::oggOpus_t(fd_t &&fd) noexcept : audioFile_t(audioType_t::oggOpus, std::move(fd)),
	decoderCtx{makeUnique<decoderContext_t>()} { }
oggOpus_t::decoderContext_t::decoderContext_t() noexcept : decoder{}, playbackBuffer{}, eof{false} { }

oggOpus_t *oggOpus_t::openR(const char *const fileName) noexcept
{
	auto file = makeUnique<oggOpus_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!file || !file->valid() || !isOggOpus(file->_fd))
		return nullptr;
	return nullptr;
}

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by OggOpus_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *OggOpus_OpenR(const char *FileName)
{
	std::unique_ptr<oggOpus_t> file(oggOpus_t::openR(FileName));
	if (!file)
		return nullptr;
	auto &ctx = *file->decoderContext();
	const fileInfo_t &info = file->fileInfo();

	if (ExternalPlayback == 0)
		file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));

	return file.release();
}

/*!
 * This function gets the \c fileInfo_t structure for an opened file
 * @param p_OpusFile A pointer to a file opened with \c OggOpus_OpenR()
 * @return A \c fileInfo_t pointer containing various metadata about an opened file or \c nullptr
 */
const fileInfo_t *OggOpus_GetFileInfo(void *p_OpusFile) { return audioFileInfo(p_OpusFile); }

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_OpusFile A pointer to a file opened with \c OggOpus_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_OpusFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long OggOpus_FillBuffer(void *p_OpusFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(p_OpusFile, OutBuffer, nOutBufferLen); }

int64_t oggOpus_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	return -2;
}

oggOpus_t::decoderContext_t::~decoderContext_t() noexcept { }

/*!
 * Closes an opened audio file
 * @param p_OpusFile A pointer to a file opened with \c OggOpus_OpenR(), or \c nullptr for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_OpusFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 */
int OggOpus_CloseFileR(void *p_OpusFile) { return audioCloseFile(p_OpusFile); }

/*!
 * Plays an opened Ogg|Opus file using OpenAL on the default audio device
 * @param p_OpusFile A pointer to a file opened with \c OggOpus_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c OggOpus_OpenR() used to open the file at \p p_OpusFile,
 * this function will do nothing.
 */
void OggOpus_Play(void *p_OpusFile) { return audioPlay(p_OpusFile); }
void OggOpus_Pause(void *p_OpusFile) { return audioPause(p_OpusFile); }
void OggOpus_Stop(void *p_OpusFile) { return audioStop(p_OpusFile); }

/*!
 * Checks the file given by \p FileName for whether it is an Ogg|Opus
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an Ogg|Opus file or not
 */
bool Is_OggOpus(const char *FileName) { return oggOpus_t::isOggOpus(FileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a Ogg|Opus
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a Ogg|Opus file or not
 */
bool oggOpus_t::isOggOpus(const int32_t fd) noexcept
{
	ogg_packet header;
	return isOgg(fd, header) && isOpus(header);
}

/*!
 * Checks the file given by \p fileName for whether it is a Ogg|Opus
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a Ogg|Opus file or not
 */
bool oggOpus_t::isOggOpus(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isOggOpus(file);
}

/*!
 * @internal
 * This structure controls decoding Ogg|Opus files when using the high-level API on them
 */
API_Functions OggOpusDecoder =
{
	OggOpus_OpenR,
	nullptr,
	audioFileInfo,
	nullptr,
	audioFillBuffer,
	nullptr,
	audioCloseFile,
	nullptr,
	audioPlay,
	audioPause,
	audioStop
};
