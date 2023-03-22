// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2009-2023 Rachel Mant <git@dragonmux.network>
#include <limits>

#include "mp3.hxx"
#include "string.hxx"

/*!
 * @internal
 * @file loadMP3.cxx
 * @brief The implementation of the MP3 decoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2010-2022
 */

using substrate::make_unique_nothrow;

namespace libAudio::mp3
{
	/*!
	 * @internal
	 * This function applies a simple conversion algorithm to convert the input
	 * fixed-point MAD sample to a short for playback
	 * @param value The fixed point sample to convert
	 * @return The converted fixed point sample
	 * @bug This function applies no noise shaping or dithering
	 *   So the output is sub-par to what it could be. FIXME!
	 */
	inline int16_t fixedToInt16(mad_fixed_t value)
	{
		using limits = std::numeric_limits<int16_t>;
		if (value >= MAD_F_ONE)
			return limits::max();
		if (value <= -MAD_F_ONE)
			return limits::min();

		value = value >> (MAD_F_FRACBITS - 15U);
		return int16_t(value);
	}

	constexpr static std::array<char, 3> id3Magic{{'I', 'D', '3'}};
} // namespace libAudio::mp3

using namespace libAudio;

mp3_t::mp3_t(fd_t &&fd, audioModeRead_t) noexcept : audioFile_t{audioType_t::mp3, std::move(fd)},
	decoderCtx{make_unique_nothrow<decoderContext_t>()} { }
mp3_t::decoderContext_t::decoderContext_t() noexcept : stream{}, frame{}, synth{}, inputBuffer{}, playbackBuffer{},
	initialFrame{true}, samplesUsed{0}, eof{false}
{
	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
}

constexpr uint32_t mp3Xing = ('X' << 24) | ('i' << 16) | ('n' << 8) | 'g';
constexpr uint32_t mp3Info = ('I' << 24) | ('n' << 16) | ('f' << 8) | 'o';
constexpr uint32_t mp3XingFrames = 0x00000001;

struct freeDelete final { void operator ()(void *ptr) noexcept { free(ptr); } };

uint32_t mp3_t::decoderContext_t::parseXingHeader() noexcept
{
	mad_bitptr *bitStream = &stream.anc_ptr;
	uint32_t xingHeader;
	uint32_t frames = 0;
	uint32_t remaining = stream.anc_bitlen;
	if (remaining < 64)
		return frames;

	xingHeader = mad_bit_read(bitStream, 32);
	remaining -= 32;
	if (xingHeader != mp3Xing && xingHeader != mp3Info)
		return frames;
	xingHeader = mad_bit_read(bitStream, 32);
	remaining -= 32;

	if (xingHeader & mp3XingFrames)
	{
		if (remaining < 32)
			return frames;
		frames = mad_bit_read(bitStream, 32);
		//remaining -= 32;
	}

	return frames;
}

std::unique_ptr<char []> copyTag(const id3_tag *tags, const char *tag) noexcept
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

bool cloneComments(const id3_tag *tags, const char *tag, std::vector<std::unique_ptr<char []>> &comments) noexcept
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
		comments.emplace_back(std::move(value));
	}
	return true;
}

uint64_t decodeIntTag(const id3_tag *tags, const char *tag) noexcept
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

bool mp3_t::readMetadata() noexcept
{
	fd_t fileDesc = fd().dup();
	auto &ctx = *decoderContext();
	fileInfo_t &info = fileInfo();
	id3_file *const file = id3_file_fdopen(fileDesc, ID3_FILE_MODE_READONLY);
	const id3_tag *const tags = id3_file_tag(file);

	info.totalTime = decodeIntTag(tags, "TLEN") / 1000;
	info.album = copyTag(tags, ID3_FRAME_ALBUM);
	info.artist = copyTag(tags, ID3_FRAME_ARTIST);
	info.title = copyTag(tags, ID3_FRAME_TITLE);
	cloneComments(tags, ID3_FRAME_COMMENT, info.other);

	int64_t seekOffset = tags->paddedsize;
	id3_file_close(file);
	fileDesc.invalidate();
	if (fd().seek(seekOffset, SEEK_SET) != seekOffset)
		return false;

	const uint32_t offset = uint32_t(!ctx.stream.buffer ? 0 : ctx.stream.bufend - ctx.stream.next_frame);
	if (!fd().read(ctx.inputBuffer.data() + offset, ctx.inputBuffer.size() - offset))
		return false;
	mad_stream_buffer(&ctx.stream, ctx.inputBuffer.data(), uint32_t(ctx.inputBuffer.size()));

	while (!ctx.frame.header.bitrate || !ctx.frame.header.samplerate)
		mad_frame_decode(&ctx.frame, &ctx.stream);

	const uint32_t frameCount = ctx.parseXingHeader();
	if (frameCount != 0)
	{
		mad_timer_t songLength = ctx.frame.header.duration;
		mad_timer_multiply(&songLength, frameCount);
		const uint64_t totalTime = mad_timer_count(songLength, MAD_UNITS_SECONDS);
		if (!info.totalTime || info.totalTime != totalTime)
			info.totalTime = totalTime;
	}
	info.bitRate = ctx.frame.header.samplerate;
	info.bitsPerSample = 16;
	info.channels = (ctx.frame.header.mode == MAD_MODE_SINGLE_CHANNEL ? 1 : 2);

	return true;
}

/*!
 * Constructs a mp3_t using the file given by \c fileName for reading and playback
 * and returns a pointer to the context of the opened file
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
mp3_t *mp3_t::openR(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<mp3_t>(fd_t{fileName, O_RDONLY | O_NOCTTY}, audioModeRead_t{})};
	if (!file || !file->valid() || !isMP3(file->_fd) || !file->readMetadata())
		return nullptr;
	auto &ctx = *file->decoderContext();
	const fileInfo_t &info = file->fileInfo();

	if (!ExternalPlayback)
		file->player(make_unique_nothrow<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192U, info));
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by MP3_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *mp3OpenR(const char *fileName) { return mp3_t::openR(fileName); }

mp3_t::decoderContext_t::~decoderContext_t() noexcept
{
	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_stream_finish(&stream);
}

/*!
 * @internal
 * Gets the next buffer of MP3 data from the MP3 file
 * @param fd The file to read data from
 */
bool mp3_t::decoderContext_t::readData(const fd_t &fd) noexcept
{
	const size_t rem = !stream.buffer ? 0 : stream.bufend - stream.next_frame;

	if (stream.next_frame)
		memmove(inputBuffer.data(), stream.next_frame, rem);

	const size_t count = inputBuffer.size() - rem;
	if (!fd.read(inputBuffer.data() + rem, count))
		return false;
	eof = fd.isEOF();

	/*if (*eof)
		memset(p_MF->inbuff + (2 * 8192), 0x00, 8);*/

	mad_stream_buffer(&stream, inputBuffer.data(), uint32_t(inputBuffer.size()));
	return true;
}

/*!
 * @internal
 * Loads the next frame of audio from the MP3 file
 * @param fd The file to decode a frame from
 */
int32_t mp3_t::decoderContext_t::decodeFrame(const fd_t &fd) noexcept
{
	if (!initialFrame &&
		mad_frame_decode(&frame, &stream) &&
		!MAD_RECOVERABLE(stream.error))
	{
		if (stream.error == MAD_ERROR_BUFLEN)
		{
			if (!readData(fd) || eof)
				return -2;
			return decodeFrame(fd);
		}
		else
		{
			printf("Unrecoverable frame-level error (%i).\n", stream.error);
			return -1;
		}
	}

	return 0;
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
int64_t mp3_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	const auto buffer = static_cast<uint8_t *>(bufferPtr);
	uint32_t offset = 0;
	const fileInfo_t &info = fileInfo();
	auto &ctx = *decoderContext();

	while (offset < length && !ctx.eof)
	{
		int16_t *playbackBuffer = reinterpret_cast<int16_t *>(ctx.playbackBuffer + offset);
		uint32_t count = 0;

		if (!ctx.samplesUsed)
		{
			int ret = -1;
			// Get input if needed, get the stream buffer part of libMAD to process that input.
			if ((!ctx.stream.buffer || ctx.stream.error == MAD_ERROR_BUFLEN) &&
				!ctx.readData(fd()))
				return ret;

			// Decode a frame:
			ret = ctx.decodeFrame(fd());
			if (ret)
				return ret;

			if (ctx.initialFrame)
				ctx.initialFrame = false;

			// Synth the PCM
			mad_synth_frame(&ctx.synth, &ctx.frame);
		}

		// copy the PCM to our output buffer
		for (uint16_t index = 0, i = ctx.samplesUsed; i < ctx.synth.pcm.length; ++i)
		{
			playbackBuffer[index++] = mp3::fixedToInt16(ctx.synth.pcm.samples[0][i]);
			if (info.channels == 2)
				playbackBuffer[index++] = mp3::fixedToInt16(ctx.synth.pcm.samples[1][i]);
			count += sizeof(int16_t) * info.channels;

			if ((offset + count) >= length)
			{
				ctx.samplesUsed = ++i;
				break;
			}
		}

		if ((offset + count) < length)
			ctx.samplesUsed = 0;

		if (buffer != ctx.playbackBuffer)
			memcpy(buffer + offset, playbackBuffer, count);
		offset += count;
	}

	return offset;
}

/*!
 * Checks the file given by \p fileName for whether it is an MP3
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an MP3 file or not
 */
bool isMP3(const char *fileName) { return mp3_t::isMP3(fileName); }

inline uint16_t asUint16(const std::array<uint8_t, 2> &value) noexcept
	{ return (uint16_t{value[0]} << 8) | value[1]; }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a MP3
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a MP3 file or not
 */
bool mp3_t::isMP3(const int32_t fd) noexcept
{
	std::array<char, 3> id3Magic;
	std::array<uint8_t, 2> mp3Magic;
	return
		fd != -1 &&
		read(fd, id3Magic.data(), id3Magic.size()) == id3Magic.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		read(fd, mp3Magic.data(), mp3Magic.size()) == mp3Magic.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		(id3Magic == libAudio::mp3::id3Magic || asUint16(mp3Magic) == 0xFFFB);
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
bool mp3_t::isMP3(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	return file.valid() && isMP3(file);
}
