#include <string>

#include "oggVorbis.hxx"
#include "string.hxx"

inline std::string operator ""_s(const char *const str, const size_t len) noexcept
	{ return {str, len}; }

/*!
 * @internal
 * @file loadOggVorbis.cpp
 * @brief The implementation of the Ogg|Vorbis decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2019
 */

namespace libAudio
{
	namespace oggVorbis
	{
		size_t read(void *buffer, size_t size, size_t count, void *filePtr)
		{
			const auto file = static_cast<const oggVorbis_t *>(filePtr);
			size_t bytes = 0;
			const bool result = file->fd().read(buffer, size * count, bytes);
			if (result)
				return bytes;
			return 0;
		}

		int seek(void *filePtr, int64_t offset, int whence)
		{
			const auto file = static_cast<const oggVorbis_t *>(filePtr);
			return file->fd().seek(offset, whence);
		}

		long tell(void *filePtr)
		{
			const auto file = static_cast<const oggVorbis_t *>(filePtr);
			return file->fd().tell();
		}
	}
}

using namespace libAudio;

oggVorbis_t::oggVorbis_t(fd_t &&fd, audioModeRead_t) noexcept :
	audioFile_t(audioType_t::oggVorbis, std::move(fd)), decoderCtx(makeUnique<decoderContext_t>()) { }
oggVorbis_t::decoderContext_t::decoderContext_t() noexcept : decoder{}, playbackBuffer{}, eof{false} { }

bool maybeCopyComment(std::unique_ptr<char []> &dst, const char *const value, const std::string &tag) noexcept
{
	const bool result = !strncasecmp(value, tag.data(), tag.size());
	if (result)
		copyComment(dst, value + tag.size());
	return result;
}

void copyComments(fileInfo_t &info, const vorbis_comment &tags) noexcept
{
	for (int i = 0; i < tags.comments; ++i)
	{
		const char *const value = tags.user_comments[i];
		if (!maybeCopyComment(info.title, value, "title="_s) &&
			!maybeCopyComment(info.artist, value, "artist="_s) &&
			!maybeCopyComment(info.album, value, "album="_s))
		{
			std::unique_ptr<char []> other;
			copyComment(other, value);
			info.other.emplace_back(std::move(other));
		}
	}
}

oggVorbis_t *oggVorbis_t::openR(const char *const fileName) noexcept
{
	auto ovFile = makeUnique<oggVorbis_t>(fd_t(fileName, O_RDONLY | O_NOCTTY), audioModeRead_t{});
	if (!ovFile || !ovFile->valid() || !isOggVorbis(ovFile->_fd))
		return nullptr;
	auto &ctx = *ovFile->decoderContext();
	fileInfo_t &info = ovFile->fileInfo();

	ov_callbacks callbacks;
	callbacks.close_func = nullptr;
	callbacks.read_func = oggVorbis::read;
	callbacks.seek_func = oggVorbis::seek;
	callbacks.tell_func = oggVorbis::tell;
	ov_open_callbacks(ovFile.get(), &ctx.decoder, NULL, 0, callbacks);

	const vorbis_info &vorbisInfo = *ov_info(&ctx.decoder, -1);
	info.bitRate = vorbisInfo.rate;
	info.channels = vorbisInfo.channels;
	info.bitsPerSample = 16;
	if (ov_seekable(&ctx.decoder))
		info.totalTime = ov_time_total(&ctx.decoder, -1);
	copyComments(info, *ov_comment(&ctx.decoder, -1));

	return ovFile.release();
}

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by OggVorbis_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *OggVorbis_OpenR(const char *FileName)
{
	std::unique_ptr<oggVorbis_t> file(oggVorbis_t::openR(FileName));
	if (!file)
		return nullptr;
	auto &ctx = *file->decoderContext();
	const fileInfo_t &info = file->fileInfo();

	if (ExternalPlayback == 0)
		file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));

	return file.release();
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c OggVorbis_Play() or \c OggVorbis_FillBuffer()
 * @bug \p p_VorbisFile must not be NULL as no checking on the parameter is done. FIXME!
 */
const fileInfo_t *OggVorbis_GetFileInfo(void *p_VorbisFile) { return audioFileInfo(p_VorbisFile); }

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_VorbisFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long OggVorbis_FillBuffer(void *p_VorbisFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(p_VorbisFile, OutBuffer, nOutBufferLen); }

int64_t oggVorbis_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	auto buffer = static_cast<char *>(bufferPtr);
	uint32_t offset = 0;
	const fileInfo_t &info = fileInfo();
	auto &ctx = *decoderContext();

	if (ctx.eof)
		return -2;
	while (offset < length && !ctx.eof)
	{
		const long result = ov_read(&ctx.decoder, buffer + offset, length - offset,
			0, info.bitsPerSample / 8, 1, nullptr);
		if (result > 0)
			offset += uint32_t(result);
		else if (result == OV_HOLE || result == OV_EBADLINK)
			return -1;
		else if (result == 0)
			ctx.eof = true;
	}
	return offset;
}

oggVorbis_t::decoderContext_t::~decoderContext_t() noexcept
	{ ov_clear(&decoder); }

/*!
 * Closes an opened audio file
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_VorbisFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_VorbisFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int OggVorbis_CloseFileR(void *p_VorbisFile) { return audioCloseFile(p_VorbisFile); }

/*!
 * Plays an opened Ogg|Vorbis file using OpenAL on the default audio device
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c OggVorbis_OpenR() used to open the file at \p p_VorbisFile,
 * this function will do nothing.
 * @bug \p p_VorbisFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_VorbisFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void OggVorbis_Play(void *p_VorbisFile) { return audioPlay(p_VorbisFile); }
void OggVorbis_Pause(void *p_VorbisFile) { return audioPause(p_VorbisFile); }
void OggVorbis_Stop(void *p_VorbisFile) { return audioStop(p_VorbisFile); }

/*!
 * Checks the file given by \p FileName for whether it is an Ogg|Vorbis
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an Ogg|Vorbis file or not
 */
bool Is_OggVorbis(const char *FileName) { return oggVorbis_t::isOggVorbis(FileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a Ogg|Vorbis
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a Ogg|Vorbis file or not
 */
bool oggVorbis_t::isOggVorbis(const int32_t fd) noexcept
{
	char oggSig[4];
	char vorbisSig[6];
	if (fd == -1 ||
		read(fd, oggSig, 4) != 4 ||
		lseek(fd, 25, SEEK_CUR) != 29 ||
		read(fd, vorbisSig, 6) != 6 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		strncmp(oggSig, "OggS", 4) != 0 ||
		strncmp(vorbisSig, "vorbis", 6) != 0)
		return false;
	return true;
}

/*!
 * Checks the file given by \p fileName for whether it is a Ogg|Vorbis
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a Ogg|Vorbis file or not
 */
bool oggVorbis_t::isOggVorbis(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isOggVorbis(file);
}

/*!
 * @internal
 * This structure controls decoding Ogg|Vorbis files when using the high-level API on them
 */
API_Functions OggVorbisDecoder =
{
	OggVorbis_OpenR,
	OggVorbis_OpenW,
	audioFileInfo,
	audioFileInfo,
	audioFillBuffer,
	audioWriteBuffer,
	audioCloseFile,
	audioCloseFile,
	audioPlay,
	audioPause,
	audioStop
};
