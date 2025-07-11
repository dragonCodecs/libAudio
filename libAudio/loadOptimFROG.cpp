// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2009-2023 Rachel Mant <git@dragonmux.network>
#include <OptimFROG/OptimFROG.h>

#include <substrate/utility>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadOptimFROG.cpp
 * @brief The implementation of the OptimFROG decoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2009-2021
 */

using substrate::make_unique_nothrow;

/*!
 * @internal
 * Internal structure for holding the decoding context for a given OptimFROG file
 */
struct optimFROG_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The decoder context handle for the OptimFROG file being decoded
	 */
	void *decoder;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	bool eof;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
};

long OptimFROG_FillBuffer(void *ofrgFile, void *OutBuffer, uint32_t nOutBufferLen);

namespace libAudio::optimFROG
{
	condition_t close(void *) { return 1; }

	sInt32_t read(void *const filePtr, void *const buffer, const uInt32_t count)
	{
		const auto *const file{static_cast<const optimFROG_t *>(filePtr)};
		size_t bytes{0};
		const auto result{file->fd().read(buffer, count, bytes)};
		if (result)
			return static_cast<sInt32_t>(bytes);
		return -1;
	}

	condition_t isEOF(void *const filePtr)
	{
		const auto *const file{static_cast<const optimFROG_t *>(filePtr)};
		return file->fd().isEOF() ? C_TRUE : C_FALSE;
	}

	condition_t seekable(void *const filePtr)
	{
		const auto *const file{static_cast<const optimFROG_t *>(filePtr)};
		return file->fd().seek(0, SEEK_CUR) == -1 && errno == ESPIPE ? C_FALSE : C_TRUE;
	}

	sInt64_t length(void *const filePtr)
	{
		const auto *const file{static_cast<const optimFROG_t *>(filePtr)};
		return file->fd().length();
	}

	sInt64_t tell(void *const filePtr)
	{
		const auto *const file{static_cast<const optimFROG_t *>(filePtr)};
		return file->fd().tell();
	}

	condition_t seek(void *const filePtr, const sInt64_t offset)
	{
		const auto *const file{static_cast<const optimFROG_t *>(filePtr)};
		return file->fd().seek(offset, SEEK_SET) == offset ? C_TRUE : C_FALSE;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
	static ReadInterface readCallbacks
	{
		close,
		read,
		isEOF,
		seekable,
		length,
		tell,
		seek
	};

	constexpr static std::array<char, 4> magic{{'O', 'F', 'R', ' '}};
} // namespace libAudio::optimFROG

using namespace libAudio;

optimFROG_t::optimFROG_t(fd_t &&fd) noexcept : audioFile_t{audioType_t::optimFROG, std::move(fd)},
	decoderCtx{make_unique_nothrow<decoderContext_t>()} { }
optimFROG_t::decoderContext_t::decoderContext_t() noexcept : decoder{OptimFROG_createInstance()},
	playbackBuffer{}, eof{false} { }

optimFROG_t *optimFROG_t::openR(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<optimFROG_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isOptimFROG(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	if (!OptimFROG_openExt(ctx.decoder, &optimFROG::readCallbacks, file.get(), true))
		return nullptr;

	OptimFROG_Info ofgInfo{};
	if (!OptimFROG_getInfo(ctx.decoder, &ofgInfo))
		return nullptr;
	info.channels(static_cast<uint8_t>(ofgInfo.channels));
	info.bitRate(ofgInfo.samplerate);
	info.bitsPerSample(static_cast<uint8_t>(ofgInfo.bitspersample));
	if (info.bitsPerSample() > 16U)
		info.bitsPerSample(16U);
	info.totalTime(ofgInfo.length_ms / 1000);

	if (!ExternalPlayback)
		file->player(make_unique_nothrow<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192U, info));
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by OptimFROG_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *optimFROGOpenR(const char *fileName) { return optimFROG_t::openR(fileName); }

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param bufferPtr A pointer to the buffer to be filled
 * @param length An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 */
int64_t optimFROG_t::fillBuffer(void *const bufferPtr, const uint32_t bufferLen)
{
	auto *const buffer = static_cast<uint8_t *>(bufferPtr);
	uint32_t offset{0};
	const fileInfo_t &info = fileInfo();
	auto &ctx = *context();
	const uint32_t stride{info.channels() * (info.bitsPerSample() / 8U)};

	if (ctx.eof)
		return -2;
	while (offset < bufferLen && !ctx.eof)
	{
		const auto samples{std::min<size_t>(bufferLen - offset, sizeof(ctx.playbackBuffer)) / stride};
		const auto result{OptimFROG_read(ctx.decoder, buffer + offset, static_cast<uInt32_t>(samples), C_TRUE)};
		if (result == -1)
			return -1;
		if (size_t(result) < samples)
			ctx.eof = true;
		offset += result * stride;
	}
	return offset;
}

optimFROG_t::decoderContext_t::~decoderContext_t() noexcept
	{ OptimFROG_close(decoder); }

/*!
 * Checks the file given by \p fileName for whether it is an OptimFROG
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an OptimFROG file or not
 */
bool isOptimFROG(const char *fileName) { return optimFROG_t::isOptimFROG(fileName); }

bool optimFROG_t::isOptimFROG(const int32_t fd) noexcept
{
	std::array<char, 4> optimFrogMagic{};
	return
		fd != -1 &&
		static_cast<size_t>(read(fd, optimFrogMagic.data(), optimFrogMagic.size())) == optimFrogMagic.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		optimFrogMagic == libAudio::optimFROG::magic;
}

bool optimFROG_t::isOptimFROG(const char *const fileName) noexcept
{
	fd_t file{fileName, O_RDONLY | O_NOCTTY};
	return file.valid() && isOptimFROG(file);
}
