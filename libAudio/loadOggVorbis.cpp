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

		constexpr static ov_callbacks callbacks
		{
			read,
			seek,
			nullptr, // We intentionally don't allow vorbisfile to close the file on us.
			tell
		};
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

/*!
 * Constructs an oggVorbis_t using the file given by \c fileName for reading and playback
 * and returns a pointer to the context of the opened file
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
oggVorbis_t *oggVorbis_t::openR(const char *const fileName) noexcept
{
	auto file = makeUnique<oggVorbis_t>(fd_t{fileName, O_RDONLY | O_NOCTTY}, audioModeRead_t{});
	if (!file || !file->valid() || !isOggVorbis(file->_fd))
		return nullptr;
	auto &ctx = *file->decoderContext();
	fileInfo_t &info = file->fileInfo();

	// If this fails, then vorbisfile figured out this is not really an Ogg|Vorbis file.
	if (ov_open_callbacks(file.get(), &ctx.decoder, NULL, 0, oggVorbis::callbacks))
		return nullptr;

	const vorbis_info &vorbisInfo = *ov_info(&ctx.decoder, -1);
	info.bitRate = vorbisInfo.rate;
	info.channels = vorbisInfo.channels;
	info.bitsPerSample = 16;
	if (ov_seekable(&ctx.decoder))
		info.totalTime = ov_time_total(&ctx.decoder, -1);
	copyComments(info, *ov_comment(&ctx.decoder, -1));

	if (!ExternalPlayback)
		file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by OggVorbis_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *oggVorbisOpenR(const char *fileName) { return oggVorbis_t::openR(fileName); }

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param bufferPtr A pointer to the buffer to be filled
 * @param length An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 */
int64_t oggVorbis_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	const auto buffer = static_cast<char *>(bufferPtr);
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
 * Checks the file given by \p fileName for whether it is an Ogg|Vorbis
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an Ogg|Vorbis file or not
 */
bool isOggVorbis(const char *fileName) { return oggVorbis_t::isOggVorbis(fileName); }

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
	ogg_packet header;
	return isOgg(fd, header) && isVorbis(header);
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
	oggVorbisOpenR,
	oggVorbisOpenW,
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
