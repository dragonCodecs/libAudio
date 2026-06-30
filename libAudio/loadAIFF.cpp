// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Mary Guillemard <mary@mary.zone>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

#include <substrate/utility>
#include "libAudio.h"
#include "libAudio.hxx"

#ifdef HAVE_LIBID3_TAG
#include <id3tag.h>
#endif

#include "string.hxx"

/*!
 * @internal
 * @file loadAIFF.cpp
 * @brief The implementation of the AIFF decoder API
 * @author Mary Guillemard <mary@mary.zone>
 * @date 2026
 */

using substrate::make_unique_nothrow;

/*!
 * @internal
 * Internal structure for holding the decoding context for a given AIFF file
 */
struct aiff_t::decoderContext_t final
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
	 * The compression format read from the AIFF file
	 */
	std::array<char, 4> compression;
	uint16_t bitsPerSample;

	uint32_t soundStartOffset;
	uint32_t soundEndOffset;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
	template<size_t N> bool copyDataTo(std::array<uint8_t, N> &buffer, const fd_t &file,
		const size_t sampleByteCount) noexcept;

private:
	bool maybeReadData(const fd_t &file, const size_t sampleByteCount) noexcept;
};

aiff_t::aiff_t(fd_t &&fd) noexcept : audioFile_t(audioType_t::aiff, std::move(fd)),
	decoderCtx(make_unique_nothrow<decoderContext_t>()), _isAIFFC(false) { }
aiff_t::decoderContext_t::decoderContext_t() noexcept : inputBuffer{}, bytesAvailable{0},
	bytesUsed{0}, playbackBuffer{}, compression{0}, bitsPerSample{0}, soundStartOffset{0},
	soundEndOffset{0} { }
aiff_t::decoderContext_t::~decoderContext_t() noexcept { }

namespace libAudio::aiff
{
	constexpr std::array<char, 4> formMagic{{'F', 'O', 'R', 'M'}};
	constexpr std::array<char, 4> aiffFormType{{'A', 'I', 'F', 'F'}};
	constexpr std::array<char, 4> aifcFormType{{'A', 'I', 'F', 'C'}};

	constexpr std::array<char, 4> commonChunkId{{'C', 'O', 'M', 'M'}};
	constexpr std::array<char, 4> soundChunkId{{'S', 'S', 'N', 'D'}};
	constexpr std::array<char, 4> nameChunkId{{'N', 'A', 'M', 'E'}};
	constexpr std::array<char, 4> authorChunkId{{'A', 'U', 'T', 'H'}};
	constexpr std::array<char, 4> copyrightChunkId{{'(', 'c', ')', ' '}};
	constexpr std::array<char, 4> annotationChunkId{{'A', 'N', 'N', 'O'}};
	constexpr std::array<char, 4> commentChunkId{{'C', 'O', 'M', 'T'}};

	// Non standard ID3 chunk id
	constexpr std::array<char, 4> id3ChunkId{{'I', 'D', '3', ' '}};

	constexpr std::array<char, 4> noneCompressionId{{'N', 'O', 'N', 'E'}};
	constexpr std::array<char, 4> sowtCompressionId{{'s', 'o', 'w', 't'}};
	constexpr std::array<char, 4> fl32CompressionId{{'f', 'l', '3', '2'}};
	constexpr std::array<char, 4> fl32AltCompressionId{{'F', 'L', '3', '2'}};


} // namespace libAudio::aiff

void aiff_t::ensurePlayable() noexcept
{
	if (!_player)
	{
		auto &ctx = *context();
		const fileInfo_t &info = fileInfo();
		player(make_unique_nothrow<playback_t>(this, audioFillBuffer, ctx.playbackBuffer, 8192U, info));
	}
}

bool aiff_t::decoderContext_t::maybeReadData(const fd_t &file, const size_t sampleByteCount) noexcept
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

template<size_t N> bool aiff_t::decoderContext_t::copyDataTo(std::array<uint8_t, N> &buffer,
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

#ifdef HAVE_LIBID3_TAG
struct freeDelete final { void operator ()(void *ptr) noexcept { free(ptr); } };

static std::unique_ptr<char []> copyTag(const id3_tag *tags, const char *tag) noexcept
{
	std::unique_ptr<char []> result;
	const id3_frame *frame = id3_tag_findframe(tags, tag, 0);
	if (!frame)
		return nullptr;
	const id3_field *field = id3_frame_field(frame, 1);
	if (!field)
		return nullptr;
	const uint32_t stringCount = id3_field_getnstrings(field);
	for (uint32_t i = 0; i < stringCount; ++i)
	{
		const id3_ucs4_t *const utf32Str = id3_field_getstrings(field, i);
		if (!utf32Str)
			continue;
		std::unique_ptr<id3_utf8_t, freeDelete> str(id3_ucs4_utf8duplicate(utf32Str));
		if (!str)
			continue;
		copyComment(result, reinterpret_cast<char *>(str.get()));
	}
	return result;
}

static bool cloneComments(const id3_tag *tags, const char *tag, fileInfo_t &info) noexcept
{
	const id3_frame *frame = id3_tag_findframe(tags, tag, 0);
	if (!frame)
		return false;
	const id3_field *field = id3_frame_field(frame, 1);
	if (!field)
		return false;
	const uint32_t stringCount = id3_field_getnstrings(field);
	for (uint32_t i = 0; i < stringCount; ++i)
	{
		const id3_ucs4_t *const utf32Str = id3_field_getstrings(field, i);
		if (!utf32Str)
			continue;
		std::unique_ptr<id3_utf8_t, freeDelete> str(id3_ucs4_utf8duplicate(utf32Str));
		if (!str)
			continue;
		std::unique_ptr<char []> value;
		copyComment(value, reinterpret_cast<char *>(str.get()));
		info.addOtherComment(std::move(value));
	}
	return true;
}

static uint64_t decodeIntTag(const id3_tag *tags, const char *tag) noexcept
{
	std::unique_ptr<char []> result;
	const id3_frame *frame = id3_tag_findframe(tags, tag, 0);
	if (!frame)
		return 0;
	const id3_field *field = id3_frame_field(frame, 1);
	if (!field)
		return 0;
	const uint32_t stringCount = id3_field_getnstrings(field);
	if (!stringCount)
		return 0;
	const id3_ucs4_t *str = id3_field_getstrings(field, 0);
	if (!str)
		return 0;
	return id3_ucs4_getnumber(str);
}

// XXX: This should be working but id3tag seems to not be able to parse IDv2 format?
bool aiff_t::readID3Chunk() noexcept
{
	fd_t fileDesc = fd().dup();
	fileInfo_t &info = fileInfo();
	id3_file *const file = id3_file_fdopen(fileDesc, ID3_FILE_MODE_READONLY);
	const id3_tag *const tags = id3_file_tag(file);

	const uint64_t totalTime = decodeIntTag(tags, "TLEN") / 1000U;
	if (totalTime)
		info.totalTime(totalTime);

	std::unique_ptr<char []> album = copyTag(tags, ID3_FRAME_ALBUM);
	if (album)
		info.album(std::move(album));

	std::unique_ptr<char []> artist = copyTag(tags, ID3_FRAME_ARTIST);
	if (artist)
		info.artist(std::move(artist));

	std::unique_ptr<char []> title = copyTag(tags, ID3_FRAME_TITLE);
	if (title)
		info.title(std::move(title));

	cloneComments(tags, ID3_FRAME_COMMENT, info);

	id3_file_close(file);
	fileDesc.invalidate();

	return true;
}
#endif

static std::unique_ptr<char []> readString(const fd_t &fd ,uint32_t dataSize)
{
	std::unique_ptr<char []> result{new char[dataSize + 1]};

	if (!fd.read(result, dataSize))
		return nullptr;

	result[dataSize] = '\0';

	return result;
}

static std::unique_ptr<char []> readPascalString(const fd_t &fd)
{
	uint8_t len;

	if (!fd.read(len))
		return nullptr;

	if (len & 1)
		len++;

	std::unique_ptr<char []> result{new char[len + 1]};

	if (!fd.read(result, len))
		return nullptr;

	result[len] = '\0';
	return result;
}

/* Representation of a IEEE 754 extended-precision (80-bit) float */
struct aiff_f80_t
{
	uint64_t value;
	uint16_t expAndSign;
};


/* For some reasons, AIFF uses a IEEE 754 extended-precision (80-bit)
 * This handle extraction of the integer part as the sample rate isn't expected to be a floating point.
 */
static uint64_t convertSampleRateToInteger(struct aiff_f80_t &raw)
{
	/* Disallow negative values */
	if (raw.expAndSign & 0x8000)
		return 0;

	/* Disallow reserved form (NaN, INF,...)*/
	if ((raw.expAndSign & 0x7fff) == 0x7fff)
		return 0;

	/* Remove exponent bias */
	int32_t exp = static_cast<int32_t>(raw.expAndSign) - 16383;

	/* If the exponent is negative, we have no integer part */
	if (exp < 0)
		return 0;

	if (exp > 63)
		return 0;

	if (exp == 0)
		return raw.value >> (63 - exp);
	else
		return (raw.value + (1ULL << (exp - 1))) >> (63 - exp);
}

uint32_t aiff_t::readChunk() noexcept
{
	const fd_t &fd = this->fd();
	auto &ctx = *context();
	fileInfo_t &info = fileInfo();
	std::array<char, 4> chunkId{};
	uint32_t chunkDataSize;

	const off_t offset = fd.tell();
	if (!fd.read(chunkId) ||
		!fd.readBE(chunkDataSize))
		return 0;

	if (chunkId == libAudio::aiff::commonChunkId) {
		uint16_t numChannels;
		uint32_t numSampleFrames;
		aiff_f80_t rawSampleRate;

		if (!fd.readBE(numChannels) ||
			!fd.readBE(numSampleFrames) ||
			!fd.readBE(ctx.bitsPerSample) ||
			!fd.readBE(rawSampleRate.expAndSign) ||
			!fd.readBE(rawSampleRate.value)) {
			return 0;
		}

		uint64_t sampleRate = convertSampleRateToInteger(rawSampleRate);
		info.channels(static_cast<uint8_t>(numChannels));
		info.bitRate(static_cast<uint32_t>(sampleRate));
		info.bitsPerSample(static_cast<uint8_t>(ctx.bitsPerSample));
		info.totalTime(numSampleFrames / sampleRate);

		if (_isAIFFC) {
			std::array<char, 4> compressionId{};
			if (!fd.read(compressionId)) {
				return 0;
			}

			std::unique_ptr<char []> compressionName = readPascalString(fd);
			if (!compressionName)
				return 0;

			ctx.compression = compressionId;
		} else {
			ctx.compression = libAudio::aiff::noneCompressionId;
		}

	}
	else if (chunkId == libAudio::aiff::soundChunkId) {
		uint32_t blockOffset;
		uint32_t blockSize;

		if (!fd.readBE(blockOffset) ||
			!fd.readBE(blockSize)) {
			return 0;
		}

		// TODO: Support this
		if (blockOffset != 0 || blockSize != 0)
			return 0;

		ctx.soundStartOffset = offset + 16;
		ctx.soundEndOffset = ctx.soundStartOffset + chunkDataSize - 8;

	}
	else if (chunkId == libAudio::aiff::nameChunkId) {
		std::unique_ptr<char []> dst = readString(fd, chunkDataSize);
		if (!dst)
			return 0;
		copyComment(info.titlePtr(), dst.get());
	}
	else if (chunkId == libAudio::aiff::authorChunkId) {
		std::unique_ptr<char []> dst = readString(fd, chunkDataSize);
		if (!dst)
			return 0;
		copyComment(info.artistPtr(), dst.get());
	}
	else if (chunkId == libAudio::aiff::annotationChunkId) {
		std::unique_ptr<char []> dst = readString(fd, chunkDataSize);
		if (!dst)
			return 0;
		info.addOtherComment(std::move(dst));
	}
#ifdef HAVE_LIBID3_TAG
	else if (chunkId == libAudio::aiff::id3ChunkId) {
		if (!readID3Chunk())
			return 0;
	}
#endif

	chunkDataSize += 8;

	if (chunkDataSize & 1)
		chunkDataSize++;

	if (fd.seek(offset + chunkDataSize, SEEK_SET) !=
		offset + chunkDataSize)
		return 0;

	return chunkDataSize;
}

bool aiff_t::readInfo(const uint32_t containerSize) noexcept
{
	const fd_t &fd = this->fd();

	off_t offset = fd.tell();
	if (offset == -1 ||
		offset == containerSize)
		return false;

	// Parse every chunks of the file
	while (offset < containerSize)
	{
		uint32_t chunkSize = this->readChunk();
		if (chunkSize == 0)
			return false;

		offset += chunkSize;
	}

	return true;
}

/*!
 * Constructs a aiff_t using the file given by \c fileName for reading and playback
 * and returns a pointer to the context of the opened file
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
aiff_t *aiff_t::openR(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<aiff_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isAIFF(file->_fd))
		return nullptr;
	const fd_t &fd = file->fd();
	auto &ctx = *file->context();
	const off_t fileSize = fd.length();
	uint32_t dataSize = 0;
	std::array<char, 4> formType{};

	if (fileSize == -1 ||
		fd.seek(4, SEEK_SET) != 4 ||
		!fd.readBE(dataSize) ||
		dataSize > (fileSize - 8))
		return nullptr;

	if (!fd.read(formType) ||
		(formType != libAudio::aiff::aiffFormType &&
		 formType != libAudio::aiff::aifcFormType))
		return nullptr;

	file->_isAIFFC = formType == libAudio::aiff::aifcFormType;

	// Adjust data size to include the first 8 bytes.
	dataSize -= 8;

	if (!file->readInfo(dataSize))
		return nullptr;

	if (fd.seek(ctx.soundStartOffset, SEEK_SET) != ctx.soundStartOffset)
		return nullptr;

	return file.release();
}

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by AIFF_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *aiffOpenR(const char *fileName) { return aiff_t::openR(fileName); }

/*!
 * Checks the file given by \p fileName for whether it is a AIFF
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a AIFF file or not
 */
bool isAIFF(const char *fileName) { return aiff_t::isAIFF(fileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a AIFF
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a AIFF file or not
 */
bool aiff_t::isAIFF(const int32_t fd) noexcept
{
	std::array<char, 4> formMagic{};
	std::array<char, 4> formType{};
	return
		fd != -1 &&
		static_cast<size_t>(read(fd, formMagic.data(), formMagic.size())) == formMagic.size() &&
		lseek(fd, 4, SEEK_CUR) == 8 &&
		static_cast<size_t>(read(fd, formType.data(), formType.size())) == formType.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		formMagic == libAudio::aiff::formMagic &&
		(formType == libAudio::aiff::aiffFormType ||
		 formType == libAudio::aiff::aifcFormType);
}

/*!
 * Checks the file given by \p fileName for whether it is a AIFF
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a AIFF file or not
 */
bool aiff_t::isAIFF(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	return file.valid() && isAIFF(file);
}

static int8_t bigEndianDataToSample(const std::array<uint8_t, 1> &data) noexcept
	{ return int8_t(data[0] ^ 0x80U); }
static int16_t bigEndianDataToSample(const std::array<uint8_t, 2> &data) noexcept
	{ return int16_t((uint16_t(data[0]) << 8U) | data[1]); }
static int16_t bigEndianDataToSample(const std::array<uint8_t, 3> &data) noexcept
	{ return int16_t((uint16_t(data[1]) << 8U) | data[2]); }
static int16_t bigEndianDataToSample(const std::array<uint8_t, 4> &data) noexcept
	{ return int16_t((uint16_t(data[2]) << 8U) | data[3]); }

template<typename T, uint8_t N> uint32_t readIntBigEndianSamples(aiff_t &aiffFile, void *buffer,
	const uint32_t length, const size_t sampleByteCount)
{
	auto &ctx = *aiffFile.context();
	const auto playbackBuffer = static_cast<T *>(buffer);
	uint32_t offset = 0;
	for (uint32_t index = 0; offset < length && offset < sampleByteCount; ++index)
	{
		std::array<uint8_t, N> data{};
		if (!ctx.copyDataTo(data, aiffFile.fd(), sampleByteCount - offset))
			break;
		playbackBuffer[index] = bigEndianDataToSample(data);
		offset += sizeof(T);
	}
	return offset;
}

static int16_t littleEndianDataToSample(const std::array<uint8_t, 2> &data) noexcept
	{ return int16_t((uint16_t(data[1]) << 8U) | data[0]); }
static int16_t littleEndianDataToSample(const std::array<uint8_t, 3> &data) noexcept
	{ return int16_t((uint16_t(data[2]) << 8U) | data[1]); }
static int16_t littleEndianDataToSample(const std::array<uint8_t, 4> &data) noexcept
	{ return int16_t((uint16_t(data[3]) << 8U) | data[2]); }

template<typename T, uint8_t N> uint32_t readIntLittleEndianSamples(aiff_t &aiffFile, void *buffer,
	const uint32_t length, const size_t sampleByteCount)
{
	auto &ctx = *aiffFile.context();
	const auto playbackBuffer = static_cast<T *>(buffer);
	uint32_t offset = 0;
	for (uint32_t index = 0; offset < length && offset < sampleByteCount; ++index)
	{
		std::array<uint8_t, N> data{};
		if (!ctx.copyDataTo(data, aiffFile.fd(), sampleByteCount - offset))
			break;
		playbackBuffer[index] = littleEndianDataToSample(data);
		offset += sizeof(T);
	}
	return offset;
}

static float dataToFloat(const std::array<uint8_t, 4> &data) noexcept
{
	const uint32_t value = (uint32_t(data[3]) << 24) |
		(uint32_t(data[2]) << 16) | (uint32_t(data[1]) << 8) | data[0];
	float result{};
	static_assert(sizeof(float) == 4, "Float is not the expected size of 4 on this platform");
	memcpy(&result, &value, 4);
	return result;
}

template<typename T, uint8_t N> uint32_t readFloatSamples(aiff_t &wavFile, void *buffer,
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
int64_t aiff_t::fillBuffer(void *const buffer, const uint32_t length)
{
	const fd_t &file = fd();
	auto &ctx = *context();

	const off_t fileOffset = file.tell();
	if (file.isEOF() || fileOffset == -1 ||
	    fileOffset >= ctx.soundEndOffset)
		return -2;
	const size_t sampleByteCount = size_t(ctx.soundEndOffset - fileOffset);

	if (ctx.compression == libAudio::aiff::noneCompressionId) {
		// 8-bit char reader
		if (ctx.bitsPerSample == 8)
			return readIntBigEndianSamples<int8_t, 1>(*this, buffer, length, sampleByteCount);
		// 16-bit short big-endian reader
		else if (ctx.bitsPerSample == 16)
			return readIntBigEndianSamples<int16_t, 2>(*this, buffer, length, sampleByteCount);
		// 24-bit int big-endian reader
		else if (ctx.bitsPerSample == 24)
			return readIntBigEndianSamples<int16_t, 3>(*this, buffer, length, sampleByteCount);
		// 32-bit int big-endian reader
		else if (ctx.bitsPerSample == 32)
			return readIntBigEndianSamples<int16_t, 4>(*this, buffer, length, sampleByteCount);
	} else if (ctx.compression == libAudio::aiff::sowtCompressionId) {
		// 8-bit char reader
		if (ctx.bitsPerSample == 8)
			return readIntBigEndianSamples<int8_t, 1>(*this, buffer, length, sampleByteCount);
		// 16-bit short little-endian reader
		else if (ctx.bitsPerSample == 16)
			return readIntLittleEndianSamples<int16_t, 2>(*this, buffer, length, sampleByteCount);
		// 24-bit int little-endian reader
		else if (ctx.bitsPerSample == 24)
			return readIntLittleEndianSamples<int16_t, 3>(*this, buffer, length, sampleByteCount);
		// 32-bit int little-endian reader
		else if (ctx.bitsPerSample == 32)
			return readIntLittleEndianSamples<int16_t, 4>(*this, buffer, length, sampleByteCount);
	} else if ((ctx.compression == libAudio::aiff::fl32CompressionId ||
				ctx.compression == libAudio::aiff::fl32AltCompressionId) && ctx.bitsPerSample == 32) {
		return readFloatSamples<int16_t, 4>(*this, buffer, length, sampleByteCount);
	}

	return 0;
}
