// SPDX-License-Identifier: BSD-3-Clause
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
 * @date 2010-2020
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
	WavpackStreamReader64 callbacks;

	decoderContext_t(std::string fileName) noexcept;
	~decoderContext_t() noexcept;
	std::unique_ptr<char []> readTag(const char *const tag) noexcept;
	void nextFrame(const uint8_t channels) noexcept;
	libAUDIO_NO_DISCARD(void *wvcFile() noexcept) { return wvcFileFD.valid() ? &wvcFileFD : nullptr; }
	static fd_t wvcFile(std::string &fileName) noexcept;
};

namespace libAudio
{
	namespace wavPack
	{
		/*!
		* @internal
		* \c read() is the internal read callback for WavPack file decoding.
		* This prevents nasty things from happening on Windows thanks to the run-time mess there.
		* @param filePtr \c FILE handle for the WavPack file as a void pointer
		* @param buffer The buffer to read into
		* @param length The number of bytes to read into the buffer
		* @return The return result of \c read()
		*/
		int32_t read(void *filePtr, void *buffer, int32_t length)
		{
			const fd_t &file = *static_cast<fd_t *>(filePtr);
			return int32_t(file.read(buffer, length, nullptr));
		}

		/*!
		* @internal
		* \c tell() is the internal read possition callback for WavPack file decoding.
		* This prevents nasty things from happening on Windows thanks to the run-time mess there.
		* @param filePtr \c FILE handle for the WavPack file as a void pointer
		* @return An integer giving the read possition of the file in bytes
		*/
		int64_t tell(void *filePtr)
		{
			const fd_t &file = *static_cast<fd_t *>(filePtr);
			return file.tell();
		}

		/*!
		* @internal
		* \c seekAbs() is the internal absolute seek callback for WavPack file decoding.
		* This prevents nasty things from happening on Windows thanks to the run-time mess there.
		* @param filePtr \c FILE handle for the WavPack file as a void pointer
		* @param offset The offset through the file to which to seek to
		* @return A truth value giving if the seek succeeded or not
		*/
		int seekAbs(void *filePtr, int64_t offset)
		{
			const fd_t &file = *static_cast<fd_t *>(filePtr);
			return file.seek(offset, SEEK_SET) != offset;
		}

		/*!
		* @internal
		* \c seekRel() is the internal any-place (relative) seek callback for WavPack file decoding.
		* This prevents nasty things from happening on Windows thanks to the run-time mess there.
		* @param filePtr \c FILE handle for the WavPack file as a void pointer
		* @param offset The offset through the file to which to seek to
		* @param mode The mode (location in the file) identifier to base the seek on
		* @return A truth value giving if the seek succeeded or not
		*/
		int seekRel(void *filePtr, int64_t offset, int mode)
		{
			const fd_t &file = *static_cast<fd_t *>(filePtr);
			return file.seek(offset, mode) == -1;
		}

		int ungetc(void *filePtr, int)
		{
			const fd_t &file = *static_cast<fd_t *>(filePtr);
			return int(file.seek(-1, SEEK_CUR));
		}

		/*!
		* @internal
		* \c len() is the internal file length callback for WavPack file decoding.
		* This prevents nasty things from happening on Windows thanks to the run-time mess there.
		* @param filePtr \c FILE handle for the WavPack file as a void pointer
		* @return An integer giving the length of the file in bytes
		*/
		int64_t length(void *filePtr)
		{
			const fd_t &file = *static_cast<fd_t *>(filePtr);
			return file.length();
		}

		/*!
		* @internal
		* \c canSeek() is the internal callback for determining if a WavPack file being
		* decoded can be seeked on or not. \n This does two things: \n
		* - It prevents nasty things from happening on Windows thanks to the run-time mess there
		* - It uses \c lseek() as a no-operation to determine if we can seek or not.
		*
		* @param filePtr \c FILE handle for the WavPack file as a void pointer
		* @return A truth value giving if seeking can work or not
		*/
		int canSeek(void *filePtr)
		{
			const fd_t &file = *static_cast<fd_t *>(filePtr);
			return file.tell() != -1;
		}
	}
}

using namespace libAudio;

wavPack_t::wavPack_t(fd_t &&fd, const char *const fileName) noexcept : audioFile_t(audioType_t::wavPack, std::move(fd)),
	ctx(makeUnique<decoderContext_t>(fileName)) { }
wavPack_t::decoderContext_t::decoderContext_t(std::string fileName) noexcept : decoder{nullptr}, playbackBuffer{},
	decodeBuffer{}, sampleCount{0}, samplesUsed{0}, eof{false}, wvcFileFD{wvcFile(fileName)}, callbacks{wavPack::read,
		nullptr, wavPack::tell, wavPack::seekAbs, wavPack::seekRel, wavPack::ungetc, wavPack::length, wavPack::canSeek,
		nullptr, nullptr} { }

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
 * Constructs a wavPack_t using the file given by \c fileName for reading and playback
 * and returns a pointer to the context of the opened file
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
wavPack_t *wavPack_t::openR(const char *const fileName) noexcept
{
	auto file{makeUnique<wavPack_t>(fd_t{fileName, O_RDONLY | O_NOCTTY}, fileName)};
	if (!file || !file->valid() || !isWavPack(file->_fd))
		return nullptr;
	fd_t &fileDesc = const_cast<fd_t &>(file->fd());
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	ctx.decoder = WavpackOpenFileInputEx64(&ctx.callbacks, &fileDesc,
		ctx.wvcFile(), nullptr, OPEN_NORMALIZE | OPEN_TAGS, 15);

	info.channels = WavpackGetNumChannels(ctx.decoder);
	info.bitsPerSample = WavpackGetBitsPerSample(ctx.decoder);
	info.bitRate = WavpackGetSampleRate(ctx.decoder);
	info.album = ctx.readTag("album");
	info.artist = ctx.readTag("artist");
	info.title = ctx.readTag("title");

	if (!ExternalPlayback)
		file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by WavPack_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *wavPackOpenR(const char *fileName) { return wavPack_t::openR(fileName); }

wavPack_t::decoderContext_t::~decoderContext_t() noexcept
	{ WavpackCloseFile(decoder); }

void wavPack_t::decoderContext_t::nextFrame(const uint8_t channels) noexcept
{
	sampleCount = WavpackUnpackSamples(decoder, decodeBuffer.data(),
		uint32_t(decodeBuffer.size()) / channels) * channels;
	samplesUsed = 0;
	eof = sampleCount == 0;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param bufferPtr A pointer to the buffer to be filled
 * @param length An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 */
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
 * Checks the file given by \p fileName for whether it is an WavPack
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an WavPack file or not
 */
bool isWavPack(const char *fileName) { return wavPack_t::isWavPack(fileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a WavPack
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a WavPack file or not
 */
bool wavPack_t::isWavPack(const int32_t fd) noexcept
{
	char wavPackSig[4];
	if (fd == -1 ||
		read(fd, wavPackSig, 4) != 4 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		memcmp(wavPackSig, "wvpk", 4) != 0)
		return false;
	return true;
}

/*!
 * Checks the file given by \p fileName for whether it is a WavPack
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a WavPack file or not
 */
bool wavPack_t::isWavPack(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isWavPack(file);
}
