#include "libAudio.h"
#include "libAudio.hxx"
#include "oggOpus.hxx"

/*!
 * @internal
 * @file loadOggOpus.cxx
 * @brief The implementation of the Ogg|Opus decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2019-2021
 */

namespace libAudio
{
	namespace oggOpus
	{
		int read(void *const filePtr, unsigned char *buffer, const int bufferLen)
		{
			const auto *const file{static_cast<const oggOpus_t *>(filePtr)};
			size_t bytes{0};
			if (file->fd().read(buffer, bufferLen, bytes))
				return int(bytes);
			return -1;
		}

		int seek(void *const filePtr, const opus_int64 offset, const int whence)
		{
			const auto *const file{static_cast<const oggOpus_t *>(filePtr)};
			return file->fd().seek(offset, whence) >= 0 ? 0 : -1;
		}

		opus_int64 tell(void *const filePtr)
		{
			const auto *const file{static_cast<const oggOpus_t *>(filePtr)};
			return file->fd().tell();
		}

		constexpr static OpusFileCallbacks callbacks
		{
			read,
			seek,
			tell,
			nullptr // We intentionally don't allow opusfile to close the file on us.
		};
	} // namespace oggOpus
} // namespace libAudio

using namespace libAudio;

oggOpus_t::oggOpus_t(fd_t &&fd, audioModeRead_t) noexcept : audioFile_t(audioType_t::oggOpus, std::move(fd)),
	decoderCtx{makeUnique<decoderContext_t>()} { }
oggOpus_t::decoderContext_t::decoderContext_t() noexcept : decoder{}, playbackBuffer{}, eof{false} { }

/*!
 * Constructs an oggOpus_t using the file given by \c fileName for reading and playback
 * and returns a pointer to the context of the opened file
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
oggOpus_t *oggOpus_t::openR(const char *const fileName) noexcept
{
	auto file{makeUnique<oggOpus_t>(fd_t{fileName, O_RDONLY | O_NOCTTY}, audioModeRead_t{})};
	if (!file || !file->valid() || !isOggOpus(file->_fd))
		return nullptr;
	auto &ctx = *file->decoderContext();
	fileInfo_t &info = file->fileInfo();
	int error = 0;

	ctx.decoder = op_open_callbacks(file.get(), &oggOpus::callbacks, nullptr, 0, &error);
	if (!ctx.decoder)
		return nullptr;

	info.channels = 2;
	info.bitRate = 48000;
	info.bitsPerSample = 16;
	if (op_seekable(ctx.decoder))
		info.totalTime = op_pcm_total(ctx.decoder, -1) / 48000;
	//OpusTags *tags = op_tags(ctx.decoder, -1);

	if (!ExternalPlayback)
		file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by OggOpus_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *oggOpusOpenR(const char *fileName) { return oggOpus_t::openR(fileName); }

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param bufferPtr A pointer to the buffer to be filled
 * @param bufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 */
int64_t oggOpus_t::fillBuffer(void *const bufferPtr, const uint32_t bufferLen)
{
	const auto buffer = static_cast<int16_t *>(bufferPtr);
	const uint32_t length = bufferLen >> 1;
	uint32_t offset = 0;
	auto &ctx = *decoderContext();

	if (ctx.eof)
		return -2;
	while (offset < length && !ctx.eof)
	{
		const int result = op_read_stereo(ctx.decoder, buffer + offset, length - offset);
		if (result > 0)
			offset += uint32_t(result) << 1;
		else if (result == OP_HOLE || result == OP_EBADLINK)
			return -1;
		else if (result == 0)
			ctx.eof = true;
	}
	return offset << 1;
}

oggOpus_t::decoderContext_t::~decoderContext_t() noexcept { op_free(decoder); }

/*!
 * Checks the file given by \p fileName for whether it is an Ogg|Opus
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an Ogg|Opus file or not
 */
bool isOggOpus(const char *fileName) { return oggOpus_t::isOggOpus(fileName); }

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
