// SPDX-License-Identifier: BSD-3-Clause
#include <limits>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadWAV.cpp
 * @brief The implementation of the WAV decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2020
 */

/*!
 * @internal
 * Internal structure for holding the decoding context for a given WAV file
 */
struct wav_t::decoderContext_t final
{
	std::array<uint8_t, 8192> inputBuffer;
	size_t bytesAvailable, bytesUsed;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	/*!
	 * @internal
	 * The byte possition where the final byte of the data chunk should be in the file
	 */
	off_t offsetDataLength;
	/*!
	 * @internal
	 * The compression flags read from the WAV file
	 */
	uint16_t compression;
	uint16_t bitsPerSample;
	/*!
	 * @internal
	 * A flag indicating if this WAV's data is floating point
	 */
	bool floatData;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
	template<size_t N> bool copyDataTo(std::array<uint8_t, N> &buffer, const fd_t &file,
		const size_t sampleByteCount) noexcept;

private:
	bool maybeReadData(const fd_t &file, const size_t sampleByteCount) noexcept;
};

wav_t::wav_t(fd_t &&fd) noexcept : audioFile_t(audioType_t::wave, std::move(fd)),
	ctx(makeUnique<decoderContext_t>()) { }
wav_t::decoderContext_t::decoderContext_t() noexcept : inputBuffer{}, bytesAvailable{0},
	bytesUsed{0}, playbackBuffer{}, offsetDataLength{0}, compression{0}, bitsPerSample{0},
	floatData{false} { }

namespace libAudio::wave
{
	constexpr std::array<char, 4> formatChunk{{'f', 'm', 't', ' '}};
	constexpr std::array<char, 4> dataChunk{{'d', 'a', 't', 'a'}};

	constexpr std::array<char, 4> riffMagic{{'R', 'I', 'F', 'F'}};
	constexpr std::array<char, 4> waveMagic{{'W', 'A', 'V', 'E'}};
} // namespace libAudio::wave

bool wav_t::skipToChunk(const std::array<char, 4> &chunkName) const noexcept
{
	std::array<char, 4> chunkTag;
	const fd_t &file = fd();
	if (!file.read(chunkTag))
		return false;

	const off_t fileSize = file.length();
	off_t offset = file.tell();
	if (fileSize == -1 || offset == -1)
		return false;
	while (chunkTag != chunkName && offset < fileSize)
	{
		uint32_t chunkLength = 0;
		if (!file.readLE(chunkLength) ||
			chunkLength > (fileSize - offset) ||
			file.seek(chunkLength, SEEK_CUR) != (offset + chunkLength + 4) ||
			!file.read(chunkTag))
			return false;
		offset += chunkLength + 8;
	}
	return chunkTag == chunkName;
}

uint32_t mapBPS(const uint16_t bitsPerSample)
{
	if (bitsPerSample == 8)
		return bitsPerSample;
	return 16;
}

bool wav_t::readFormat() noexcept
{
	auto &ctx = *context();
	fileInfo_t &info = fileInfo();
	const fd_t &file = fd();
	std::array<char, 6> unused;
	uint16_t channels;

	if (!file.readLE(ctx.compression) ||
		!file.readLE(channels) ||
		!file.readLE(info.bitRate) ||
		!file.read(unused) ||
		!file.readLE(ctx.bitsPerSample) ||
		!channels ||
		!info.bitRate ||
		!ctx.bitsPerSample ||
		(ctx.bitsPerSample % 8))
		return false;
	info.channels = channels;
	info.bitsPerSample = mapBPS(ctx.bitsPerSample);
	ctx.floatData = ctx.compression == 3;
	return true;
}

/*!
 * Constructs a wav_t using the file given by \c fileName for reading and playback
 * and returns a pointer to the context of the opened file
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
wav_t *wav_t::openR(const char *const fileName) noexcept
{
	auto file{makeUnique<wav_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isWAV(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();
	const fd_t &fd = file->fd();
	const off_t fileSize = fd.length();
	uint32_t chunkLength = 0;

	if (fileSize == -1 ||
		fd.seek(4, SEEK_SET) != 4 ||
		!fd.readLE(chunkLength) ||
		chunkLength > (fileSize - 8) ||
		fd.seek(4, SEEK_CUR) != 12 ||
		!file->skipToChunk(libAudio::wave::formatChunk))
		return nullptr;
	off_t offset = fd.tell();
	if (offset == -1 ||
		!fd.readLE(chunkLength) ||
		chunkLength > (fileSize - offset - 4) ||
		chunkLength < 16 ||
		!file->readFormat())
		return nullptr;

	// Currently we do not care if the file has extra data, we're only looking to work with PCM.
	for (uint32_t i = 16; i < chunkLength; ++i)
	{
		char value = 0;
		if (!fd.read(value))
			return nullptr;
	}

	if (!file->skipToChunk(libAudio::wave::dataChunk) ||
		!fd.readLE(chunkLength) ||
		(offset = fd.tell()) == -1 ||
		chunkLength > (fileSize - offset) ||
		fd.isEOF())
		return nullptr;
	info.totalTime = chunkLength / info.channels;
	info.totalTime /= ctx.bitsPerSample / 8;
	info.totalTime /= info.bitRate;
	ctx.offsetDataLength = chunkLength + fd.tell();

	if (!ExternalPlayback)
		file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by WAV_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *wavOpenR(const char *fileName) { return wav_t::openR(fileName); }

wav_t::decoderContext_t::~decoderContext_t() noexcept { }
int8_t dataToSample(const std::array<uint8_t, 1> &data) noexcept
	{ return int8_t(data[0] ^ 0x80U); }
int16_t dataToSample(const std::array<uint8_t, 2> &data) noexcept
	{ return int16_t((uint16_t(data[1]) << 8U) | data[0]); }
int16_t dataToSample(const std::array<uint8_t, 3> &data) noexcept
	{ return int16_t((uint16_t(data[2]) << 8U) | data[1]); }
int16_t dataToSample(const std::array<uint8_t, 4> &data) noexcept
	{ return int16_t((uint16_t(data[3]) << 8U) | data[2]); }

float dataToFloat(const std::array<uint8_t, 4> &data) noexcept
{
	const uint32_t value = (uint32_t(data[3]) << 24) |
		(uint32_t(data[2]) << 16) | (uint32_t(data[1]) << 8) | data[0];
	float result{};
	static_assert(sizeof(float) == 4, "Float is not the expected size of 4 on this platform");
	memcpy(&result, &value, 4);
	return result;
}

bool wav_t::decoderContext_t::maybeReadData(const fd_t &file, const size_t sampleByteCount) noexcept
{
	if (bytesUsed == bytesAvailable)
	{
		const auto amount = std::min(sampleByteCount, inputBuffer.size());
		const auto result = file.read(inputBuffer.data(), amount, nullptr);
		if (result <= 0)
			return false;
		else
			bytesAvailable = size_t(result);
		bytesUsed = 0;
	}
	return true;
}

template<size_t N> bool wav_t::decoderContext_t::copyDataTo(std::array<uint8_t, N> &buffer,
	const fd_t &file, const size_t sampleByteCount) noexcept
{
	if (!maybeReadData(file, sampleByteCount))
		return false;
	const size_t amount = std::min(bytesAvailable - bytesUsed, buffer.size());
	memcpy(buffer.data(), inputBuffer.data() + bytesUsed, amount);
	bytesUsed += amount;
	if (amount == buffer.size())
		return true; // If we're done, exit early to avoid the expense of the second half of this function
	else if (!maybeReadData(file, sampleByteCount))
		return false;
	memcpy(buffer.data() + amount, inputBuffer.data() + bytesUsed, buffer.size() - amount);
	bytesUsed += buffer.size() - amount;
	return true;
}

template<typename T, uint8_t N> uint32_t readIntSamples(wav_t &wavFile, void *buffer,
	const uint32_t length, const size_t sampleByteCount)
{
	auto &ctx = *wavFile.context();
	const auto playbackBuffer = static_cast<T *>(buffer);
	uint32_t offset = 0;
	for (uint32_t index = 0; offset < length && offset < sampleByteCount; ++index)
	{
		std::array<uint8_t, N> data{};
		if (!ctx.copyDataTo(data, wavFile.fd(), sampleByteCount - offset))
			break;
		playbackBuffer[index] = dataToSample(data);
		offset += sizeof(T);
	}
	return offset;
}

template<typename T, uint8_t N> uint32_t readFloatSamples(wav_t &wavFile, void *buffer,
	const uint32_t length, const size_t sampleByteCount)
{
	using limits = std::numeric_limits<T>;
	auto &ctx = *wavFile.context();
	const auto playbackBuffer = static_cast<T *>(buffer);
	uint32_t offset = 0;
	for (uint32_t index = 0; offset < length && offset < sampleByteCount; ++index)
	{
		std::array<uint8_t, N> data{};
		if (!ctx.copyDataTo(data, wavFile.fd(), sampleByteCount - offset))
			break;
		const float sample = dataToFloat(data);
		playbackBuffer[index] = T(sample * limits::max());
		offset += sizeof(T);
	}
	return offset;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param buffer A pointer to the buffer to be filled
 * @param length An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 */
int64_t wav_t::fillBuffer(void *const buffer, const uint32_t length)
{
	uint32_t offset = 0;
	const fd_t &file = fd();
	auto &ctx = *context();

	const off_t fileOffset = file.tell();
	if (file.isEOF() || fileOffset == -1 || fileOffset >= ctx.offsetDataLength)
		return -2;
	const size_t sampleByteCount = size_t(ctx.offsetDataLength - fileOffset);
	// 8-bit char reader
	if (!ctx.floatData && ctx.bitsPerSample == 8)
		return readIntSamples<int8_t, 1>(*this, buffer, length, sampleByteCount);
	// 16-bit short reader
	else if (!ctx.floatData && ctx.bitsPerSample == 16)
		return readIntSamples<int16_t, 2>(*this, buffer, length, sampleByteCount);
	// 24-bit int reader
	else if (!ctx.floatData && ctx.bitsPerSample == 24)
		return readIntSamples<int16_t, 3>(*this, buffer, length, sampleByteCount);
	// 32-bit int reader
	else if (!ctx.floatData && ctx.bitsPerSample == 32)
		return readIntSamples<int16_t, 4>(*this, buffer, length, sampleByteCount);
	/*// 24-bit float reader
	else if (ctx.floatData && info.bitsPerSample == 24)
	{
		for (int i = 0; i < nOutBufferLen && ftell(f_WAV) < p_WF->DataEnd; i += 2)
		{
			float in;
			fread(&in, 3, 1, f_WAV);
			*((short *)(OutBuffer + ret)) = (short)(in * 65535.0F);
			ret += 2;
		}
	}*/
	// 32-bit float reader
	else if (ctx.floatData && ctx.bitsPerSample == 32)
		return readFloatSamples<int16_t, 4>(*this, buffer, length, sampleByteCount);
	else
		return -1;

	return offset;
}

/*!
 * Checks the file given by \p fileName for whether it is a WAV
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a WAV file or not
 */
bool isWAV(const char *fileName) { return wav_t::isWAV(fileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a WAV
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a WAV file or not
 */
bool wav_t::isWAV(const int32_t fd) noexcept
{
	std::array<char, 4> riffMagic;
	std::array<char, 4> waveMagic;
	return
		fd != -1 &&
		read(fd, riffMagic.data(), riffMagic.size()) == riffMagic.size() &&
		lseek(fd, 4, SEEK_CUR) == 8 &&
		read(fd, waveMagic.data(), waveMagic.size()) == waveMagic.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		riffMagic == libAudio::wave::riffMagic &&
		waveMagic == libAudio::wave::waveMagic;
}

/*!
 * Checks the file given by \p fileName for whether it is a WAV
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a WAV file or not
 */
bool wav_t::isWAV(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isWAV(file);
}
