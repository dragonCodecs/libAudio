#include <stdio.h>
#include <malloc.h>

#include <mad.h>
#include <id3tag.h>

#include "libAudio.h"
#include "libAudio.hxx"

#include <limits.h>

/*!
 * @internal
 * @file loadMP3.cpp
 * @brief The implementation of the MP3 decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2013
 */

#ifndef SHRT_MAX
/*!
 * @internal
 * Backup definition of \c SHRT_MAX, which should be defined in limit.h,
 * but which assumes that short's length is 16 bits
 */
#define SHRT_MAX 0x7FFF
#endif

struct mp3_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The MP3 stream handle
	 */
	mad_stream stream;
	/*!
	 * @internal
	 * The MP3 frame handle
	 */
	mad_frame frame;
	/*!
	 * @internal
	 * The MP3 audio synthesis handle
	 */
	mad_synth synth;
	/*!
	 * @internal
	 * The internal input data buffer
	 */
	std::array<uint8_t, 8192> inputBuffer;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	/*!
	 * @internal
	 * A flag indicating if we have yet to decode this MP3's initial frame
	 */
	bool initialFrame;
	/*!
	 * @internal
	 * A count giving the amount of decoded PCM which has been used with
	 * the current values stored in buffer
	 */
	uint16_t samplesUsed;
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;

	decoderContext_t();
	~decoderContext_t() noexcept;
	bool readData(const fd_t &fd) noexcept WARN_UNUSED;
	int32_t decodeFrame(const fd_t &fd) noexcept WARN_UNUSED;
};

/*!
 * @internal
 * Internal structure for holding the decoding context for a given MP3 file
 */
typedef struct _MP3_Intern
{
	/*!
	 * @internal
	 * The \c FileInfo for the MP3 file being decoded
	 */
	FileInfo *p_FI;
	/*!
	 * @internal
	 * A string holding the name of the MP3 file relative to it's opening path
	 * which is used when reading the ID3 data
	 */
	char *f_name;

	mp3_t inner;

	_MP3_Intern(fd_t &&fd) : inner(std::move(fd)) { }
} MP3_Intern;

/*!
 * @internal
 * This function applies a simple conversion algorithm to convert the input
 * fixed-point MAD sample to a short for playback
 * @param Fixed The fixed point sample to convert
 * @return The converted fixed point sample
 * @bug This function applies no noise shaping or dithering
 *   So the output is sub-par to what it could be. FIXME!
 */
inline short FixedToShort(mad_fixed_t Fixed)
{
	if (Fixed >= MAD_F_ONE)
		return SHRT_MAX;
	if (Fixed <= -MAD_F_ONE)
		return -SHRT_MAX;

	Fixed = Fixed >> (MAD_F_FRACBITS - 15);
	return (signed short)Fixed;
}

mp3_t::mp3_t(fd_t &&fd) noexcept : audioFile_t(audioType_t::mp3, std::move(fd)), ctx(makeUnique<decoderContext_t>()) { }
mp3_t::decoderContext_t::decoderContext_t() : stream{}, frame{}, synth{}, inputBuffer{}, playbackBuffer{},
	initialFrame{true}, samplesUsed{0}, eof{false} { }

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by MP3_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *MP3_OpenR(const char *FileName)
{
	MP3_Intern *ret = NULL;
	FILE *f_MP3 = NULL;

	f_MP3 = fopen(FileName, "rb");
	if (f_MP3 == NULL)
		return ret;

	ret = new (std::nothrow) MP3_Intern(fd_t(FileName, O_RDONLY | O_NOCTTY));
	if (!ret || !ret->inner.context())
		return nullptr;
	auto &ctx = *ret->inner.context();

	ret->f_name = strdup(FileName);
	mad_stream_init(&ctx.stream);
	mad_frame_init(&ctx.frame);
	mad_synth_init(&ctx.synth);

	return ret;
}

#define MP3_XING (('X' << 24) | ('i' << 16) | ('n' << 8) | 'g')
#define MP3_INFO (('I' << 24) | ('n' << 16) | ('f' << 8) | 'o')
#define MP3_XING_FRAMES	0x00000001

uint32_t ParseXingHeader(mad_bitptr bitStream, uint32_t bitLen)
{
	uint32_t xingHeader;
	uint32_t frames = 0;
	uint32_t remaining = bitLen;
	if (bitLen < 64)
		return frames;

	xingHeader = mad_bit_read(&bitStream, 32);
	remaining -= 32;
	if (xingHeader != MP3_XING && xingHeader != MP3_INFO)
		return frames;
	xingHeader = mad_bit_read(&bitStream, 32);
	remaining -= 32;

	if ((xingHeader & MP3_XING_FRAMES) != 0)
	{
		if (remaining < 32)
			return frames;
		frames = mad_bit_read(&bitStream, 32);
		remaining -= 32;
	}

	return frames;
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_MP3File A pointer to a file opened with \c MP3_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c MP3_Play() or \c MP3_FillBuffer()
 * @bug \p p_MP3File must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *MP3_GetFileInfo(void *p_MP3File)
{
	uint32_t frameCount;
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	FileInfo *ret = NULL;
	id3_file *f_id3 = NULL;
	id3_tag *id_tag = NULL;
	id3_frame *id_frame = NULL;
	auto &ctx = *p_MF->inner.context();
	const fd_t &fd = p_MF->inner.fd();

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	f_id3 = id3_file_open(/*fileno(p_MF->f_MP3)*/p_MF->f_name, ID3_FILE_MODE_READONLY);
	id_tag = id3_file_tag(f_id3);
	id_frame = id3_tag_findframe(id_tag, "TLEN", 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = id3_frame_field(id_frame, 1);
		if (id_field != NULL)
		{
			uint32_t strings = id3_field_getnstrings(id_field);
			if (strings > 0)
			{
				const id3_ucs4_t *str = id3_field_getstrings(id_field, 0);
				if (str != NULL)
					ret->TotalTime = id3_ucs4_getnumber(str) / 1000;
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_ALBUM, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = id3_frame_field(id_frame, 1);
		if (id_field != NULL)
		{
			uint32_t strings = id3_field_getnstrings(id_field);
			if (strings > 0)
			{
				for (uint32_t i = 0; i < strings; i++)
				{
					const id3_ucs4_t *str = id3_field_getstrings(id_field, i);
					if (str != NULL)
					{
						char *Album = (char *)id3_ucs4_utf8duplicate(str);
						if (Album == NULL)
							continue;
						if (ret->Album == NULL)
						{
							ret->Album = (const char *)malloc(strlen(Album) + 1);
							memset((char *)ret->Album, 0x00, strlen(Album) + 1);
						}
						else
						{
							ret->Album = (const char *)realloc((char *)ret->Album, strlen(ret->Album) + strlen(Album) + 4);
							memcpy((char *)ret->Album + strlen(ret->Album), " / ", 4);
						}
						memcpy((char *)ret->Album + strlen(ret->Album), Album, strlen(Album) + 1);
						free(Album);
					}
				}
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_ARTIST, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = id3_frame_field(id_frame, 1);
		if (id_field != NULL)
		{
			uint32_t strings = id3_field_getnstrings(id_field);
			if (strings > 0)
			{
				for (uint32_t i = 0; i < strings; i++)
				{
					const id3_ucs4_t *str = id3_field_getstrings(id_field, i);
					if (str != NULL)
					{
						char *Artist = (char *)id3_ucs4_utf8duplicate(str);
						if (Artist == NULL)
							continue;
						if (ret->Artist == NULL)
						{
							ret->Artist = (const char *)malloc(strlen(Artist) + 1);
							memset((char *)ret->Artist, 0x00, strlen(Artist) + 1);
						}
						else
						{
							ret->Artist = (const char *)realloc((char *)ret->Artist, strlen(ret->Artist) + strlen(Artist) + 4);
							memcpy((char *)ret->Artist + strlen(ret->Artist), " / ", 4);
						}
						memcpy((char *)ret->Artist + strlen(ret->Artist), Artist, strlen(Artist) + 1);
						free(Artist);
					}
				}
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_TITLE, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = id3_frame_field(id_frame, 1);
		if (id_field != NULL)
		{
			uint32_t strings = id3_field_getnstrings(id_field);
			if (strings > 0)
			{
				for (uint32_t i = 0; i < strings; i++)
				{
					const id3_ucs4_t *str = id3_field_getstrings(id_field, i);
					if (str != NULL)
					{
						char *Title = (char *)id3_ucs4_utf8duplicate(str);
						if (Title == NULL)
							continue;
						if (ret->Title == NULL)
						{
							ret->Title = (const char *)malloc(strlen(Title) + 1);
							memset((char *)ret->Title, 0x00, strlen(Title) + 1);
						}
						else
						{
							ret->Title = (const char *)realloc((char *)ret->Title, strlen(ret->Title) + strlen(Title) + 4);
							memcpy((char *)ret->Title + strlen(ret->Title), " / ", 4);
						}
						memcpy((char *)ret->Title + strlen(ret->Title), Title, strlen(Title) + 1);
						free(Title);
					}
				}
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_COMMENT, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = id3_frame_field(id_frame, 1);
		if (id_field != NULL)
		{
			uint32_t strings = id3_field_getnstrings(id_field);
			if (strings > 0)
			{
				for (uint32_t i = 0; i < strings; i++)
				{
					const id3_ucs4_t *str = id3_field_getstrings(id_field, i);
					if (str != NULL)
					{
						char *Comment = (char *)id3_ucs4_utf8duplicate(str);
						if (Comment == NULL)
							continue;
						if (ret->OtherComments.size() == 0)
							ret->nOtherComments = 0;
						ret->OtherComments.push_back(Comment);
						ret->nOtherComments++;
					}
				}
			}
		}
	}

	fd.seek(id_tag->paddedsize, SEEK_SET);
	id3_file_close(f_id3);

	const uint32_t rem = (!ctx.stream.buffer ? 0 : ctx.stream.bufend - ctx.stream.next_frame);
	fd.read(ctx.inputBuffer.data() + rem, ctx.inputBuffer.size() - rem);
	mad_stream_buffer(&ctx.stream, ctx.inputBuffer.data(), ctx.inputBuffer.size());

	while (ctx.frame.header.bitrate == 0 || ctx.frame.header.samplerate == 0)
		mad_frame_decode(&ctx.frame, &ctx.stream);

	frameCount = ParseXingHeader(ctx.stream.anc_ptr, ctx.stream.anc_bitlen);
	if (frameCount != 0)
	{
		uint64_t totalTime;
		mad_timer_t songLength = ctx.frame.header.duration;
		mad_timer_multiply(&songLength, frameCount);
		totalTime = mad_timer_count(songLength, MAD_UNITS_SECONDS);
		if (ret->TotalTime == 0 || ret->TotalTime != totalTime)
			ret->TotalTime = totalTime;
	}
	ret->BitRate = ctx.frame.header.samplerate;
	ret->BitsPerSample = 16;
	ret->Channels = (ctx.frame.header.mode == MAD_MODE_SINGLE_CHANNEL ? 1 : 2);
	p_MF->inner.fileInfo().channels = ret->Channels;

	if (ExternalPlayback == 0)
		p_MF->inner.player(makeUnique<playback_t>(p_MP3File, MP3_FillBuffer, ctx.playbackBuffer, 8192, ret));
	p_MF->p_FI = ret;

	return ret;
}

mp3_t::decoderContext_t::~decoderContext_t() noexcept
{
	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_stream_finish(&stream);
}

/*!
 * Closes an opened audio file
 * @param p_MP3File A pointer to a file opened with \c MP3_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_MP3File after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_MP3File must not be NULL as no checking on the parameter is done. FIXME!
 */
int MP3_CloseFileR(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;

	free(p_MF->f_name);
	delete p_MF;
	return 0;
}

/*!
 * @internal
 * Gets the next buffer of MP3 data from the MP3 file
 * @param p_MF Our internal MP3 context
 * @param eof The pointer to the internal eof member
 *   passed to help speed this function up slightly by not having
 *   to re-dereference the pointers and memory needed to locate
 *   the member
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

	mad_stream_buffer(&stream, inputBuffer.data(), inputBuffer.size());
	return true;
}

/*!
 * @internal
 * Loads the next frame of audio from the MP3 file
 * @param p_MF Our internal MP3 context
 * @param eof A pointer to the internal eof member
 *   for passing to \c GetData()
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
 * @param p_MP3File A pointer to a file opened with \c MP3_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_MP3File must not be NULL as no checking on the parameter is done. FIXME!
 */
long MP3_FillBuffer(void *p_MP3File, uint8_t *OutBuffer, int nOutBufferLen)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	return audioFillBuffer(&p_MF->inner, OutBuffer, nOutBufferLen);
}

int64_t mp3_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	auto *buffer = reinterpret_cast<uint8_t *const>(bufferPtr);
	uint32_t offset = 0;
	const fileInfo_t &info = fileInfo();
	auto &ctx = *context();

	while (offset < length && !ctx.eof)
	{
		int16_t *out = reinterpret_cast<int16_t *>(ctx.playbackBuffer + offset);
		uint32_t nOut = 0;

		if (!ctx.samplesUsed)
		{
			int ret = -1;
			// Get input if needed, get the stream buffer part of libMAD to process that input.
			if ((!ctx.stream.buffer || ctx.stream.error == MAD_ERROR_BUFLEN) &&
				!ctx.readData(fd()))
				return ret;

			// Decode a frame:
			if (ret = ctx.decodeFrame(fd()))
				return ret;

			if (ctx.initialFrame)
				ctx.initialFrame = false;

			// Synth the PCM
			mad_synth_frame(&ctx.synth, &ctx.frame);
		}

		// copy the PCM to our output buffer
		for (uint16_t index = 0, i = ctx.samplesUsed; i < ctx.synth.pcm.length; ++i)
		{
			out[index++] = FixedToShort(ctx.synth.pcm.samples[0][i]);
			if (info.channels == 2)
				out[index++] = FixedToShort(ctx.synth.pcm.samples[1][i]);
			nOut += sizeof(int16_t) * info.channels;

			if ((offset + nOut) >= length)
			{
				ctx.samplesUsed = ++i;
				break;
			}
		}

		if ((offset + nOut) < length)
			ctx.samplesUsed = 0;

		if (buffer != ctx.playbackBuffer)
			memcpy(buffer + offset, out, nOut);
		offset += nOut;
	}

	return offset;
}

/*!
 * Plays an opened MP3 file using OpenAL on the default audio device
 * @param p_MP3File A pointer to a file opened with \c MP3_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c MP3_OpenR() used to open the file at \p p_MP3File,
 * this function will do nothing.
 * @bug \p p_MP3File must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_MP3File check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void MP3_Play(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	p_MF->inner.play();
}

void MP3_Pause(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	p_MF->inner.pause();
}

void MP3_Stop(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	p_MF->inner.stop();
}

/*!
 * Checks the file given by \p FileName for whether it is an MP3
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an MP3 file or not
 */
bool Is_MP3(const char *FileName) { return mp3_t::isMP3(FileName); }

inline uint16_t asUint16(uint8_t *value) noexcept
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
bool mp3_t::isMP3(const int fd) noexcept
{
	char id3[3];
	uint8_t mp3Sig[2];
	if (fd == -1 ||
		read(fd, id3, 3) != 3 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		read(fd, mp3Sig, 2) != 2 ||
		(strncmp(id3, "ID3", 3) != 0 &&
		asUint16(mp3Sig) != 0xFFFB))
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
bool mp3_t::isMP3(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isMP3(file);
}

/*!
 * @internal
 * This structure controls decoding MP3 files when using the high-level API on them
 */
API_Functions MP3Decoder =
{
	MP3_OpenR,
	MP3_GetFileInfo,
	MP3_FillBuffer,
	MP3_CloseFileR,
	MP3_Play,
	MP3_Pause,
	MP3_Stop
};
