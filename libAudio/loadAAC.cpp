#include <algorithm>
#include <neaacdec.h>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadAAC.cpp
 * @brief The implementation of the AAC decoder API
 * @note Not to be confused with the M4A/MP4 decoder
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2013
 */

/*!
 * @internal
 * Gives the header size for ADIF AAC streams
 * @note This was taken from FAAD2's aacinfo.c
 */
/* Following 2 definitions are taken from aacinfo.c from faad2: */
#define ADIF_MAX_SIZE 30 /* Should be enough */
//#define ADTS_MAX_SIZE 10 /* Should be enough */

/*!
 * @internal
 * Gives the header size for ADTS AAC streams
 * @note This was taken from ffmpeg's aac_parser.c, but there was another,
 *   conflicting, definition in FAAD2's aacinfo.c which is 10 rather than 7
 */
/* ffmpeg's aac_parser.c specifies the following for ADTS_MAX_SIZE: */
#define ADTS_MAX_SIZE 7

/*!
 * @internal
 * Internal structure for holding the decoding context for a given AAC file
 */
struct aac_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The decoder context handle
	 */
	NeAACDecHandle decoder;
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;
	/*!
	 * @internal
	 * @var int samplesUsed
	 * The number of frames decoded relative to the total number
	 * @var int sampleCount
	 * The total number of frames to decode
	 */
	uint64_t sampleCount, samplesUsed;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t *decodeBuffer;
	/*!
	 * @internal
	 * The playback data buffer
	 */
	uint8_t playbackBuffer[8192];

	decoderContext_t();
	~decoderContext_t() noexcept;
};

aac_t::aac_t(fd_t &&fd) noexcept : audioFile_t(audioType_t::aac, std::move(fd)), ctx(makeUnique<decoderContext_t>()) { }
aac_t::decoderContext_t::decoderContext_t() : decoder{NeAACDecOpen()}, eof{false}, sampleCount{0},
	samplesUsed{0}, decodeBuffer{nullptr}, playbackBuffer{} { }

aac_t *aac_t::openR(const char *const fileName) noexcept
{
	std::unique_ptr<aac_t> aacFile(makeUnique<aac_t>(fd_t(fileName, O_RDONLY | O_NOCTTY)));
	if (!aacFile || !aacFile->valid() || !isAAC(aacFile->_fd))
		return nullptr;

	auto &ctx = *aacFile->context();
	fileInfo_t &info = aacFile->fileInfo();
	const fd_t &file = aacFile->fd();
	std::array<uint8_t, ADTS_MAX_SIZE> frameHeader;

	if (!file.read(frameHeader) ||
		file.seek(0, SEEK_SET) != 0)
		return nullptr;
	unsigned long bitRate;
	unsigned char channels;
	NeAACDecInit(ctx.decoder, frameHeader.data(), frameHeader.size(), &bitRate, &channels);
	NeAACDecConfiguration *const config = NeAACDecGetCurrentConfiguration(ctx.decoder);
	config->outputFormat = FAAD_FMT_16BIT;
	NeAACDecSetConfiguration(ctx.decoder, config);

	info.bitRate = bitRate;
	info.channels = channels;
	info.bitsPerSample = 16;

	return aacFile.release();
}

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by AAC_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *AAC_OpenR(const char *FileName)
{
	std::unique_ptr<aac_t> file(aac_t::openR(FileName));
	if (!file)
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	if (ExternalPlayback == 0)
		file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));

	return file.release();
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_AACFile A pointer to a file opened with \c AAC_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c AAC_Play() or \c AAC_FillBuffer()
 * @bug \p p_AACFile must not be NULL as no checking on the parameter is done. FIXME!
 */
const fileInfo_t *AAC_GetFileInfo(void *p_AACFile) { return audioFileInfo(p_AACFile); }

aac_t::decoderContext_t::~decoderContext_t() noexcept
	{ NeAACDecClose(decoder); }

/*!
 * Closes an opened audio file
 * @param p_AACFile A pointer to a file opened with \c AAC_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_AACFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_AACFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int AAC_CloseFileR(void *p_AACFile) { return audioCloseFile(p_AACFile); }

namespace libAudio
{
	namespace aac
	{
		/*!
		* @internal
		* Internal structure used to read the AAC bitstream so that packets
		* of data can be sent into FAAD2 correctly as packets so to ease the
		* job of decoding
		*/
		struct bitStream_t final
		{
		private:
			/*!
			* @internal
			* The data buffer being used as a bitstream
			*/
			uint8_t *data;
			/*!
			* @internal
			* The total number of bits available in the buffer
			*/
			uint64_t bitTotal;
			/*!
			* @internal
			* The index of the current bit relative to the total
			*   number of bits available in the buffer
			*/
			uint64_t currentBit;

		public:
			/*!
			* @internal
			* Internal function used to open a buffer as a bitstream
			* @param bufferLen The length of the buffer to be used
			* @param buffer The buffer to be used
			*/
			bitStream_t(uint8_t *const buffer, const uint32_t bufferLen) noexcept :
				data{buffer}, bitTotal{bufferLen * 8}, currentBit{0} { }

			template<typename T, size_t N> bitStream_t(std::array<T, N> &buffer) noexcept :
				bitStream_t(reinterpret_cast<uint8_t *>(buffer.data()), buffer.size()) { }

			/*!
			* @internal
			* Internal function used to get the next \p NumBits bits sequentially from the bitstream
			* @param bitCount The number of bits to get
			*/
			uint64_t value(const uint32_t bitCount) noexcept
			{
				uint64_t result = 0;
				for (uint32_t num = 0; num < bitCount; ++num)
				{
					const uint32_t byte = currentBit / 8;
					const uint32_t bit = 7 - (currentBit % 8);
					result <<= 1;
					const uint8_t value = data[byte] & (1 << bit);
					result += value >> bit;
					++currentBit;
					if (currentBit == bitTotal)
						break;
				}
				return result;
			}

			/*!
			* @internal
			* Internal function used to skip a number of bits in the bitstream
			* @param bitCount The number of bits to skip
			*/
			void skip(const uint32_t bitCount) noexcept
				{ currentBit += bitCount; }
		};
	}
}

using namespace libAudio;

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_AACFile A pointer to a file opened with \c AAC_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_AACFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long AAC_FillBuffer(void *p_AACFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(p_AACFile, OutBuffer, nOutBufferLen); }

uint8_t *aac_t::nextFrame() noexcept
{
	const fd_t &file = fd();
	auto &ctx = *context();
	std::array<uint8_t, ADTS_MAX_SIZE> frameHeader;
	if (!file.read(frameHeader) ||
		file.isEOF())
	{
		ctx.eof = file.isEOF();
		return nullptr;
	}
	aac::bitStream_t stream{frameHeader};
	const uint16_t read = uint16_t(stream.value(12));
	if (read != 0xFFF)
	{
		ctx.eof = true;
		return nullptr;
	}
	stream.skip(18);
	const uint16_t FrameLength = uint16_t(stream.value(13));
	std::unique_ptr<uint8_t []> buffer = makeUnique<uint8_t []>(FrameLength);
	memcpy(buffer.get(), frameHeader.data(), frameHeader.size());
	if (!file.read(buffer.get() + ADTS_MAX_SIZE, FrameLength - ADTS_MAX_SIZE) ||
		file.isEOF())
	{
		ctx.eof = file.isEOF();
		if (!ctx.eof)
			return nullptr;
	}
	NeAACDecFrameInfo FI{};
	ctx.decodeBuffer = static_cast<uint8_t *>(NeAACDecDecode(ctx.decoder, &FI, buffer.get(), FrameLength));
	if (FI.error != 0)
	{
		printf("Error: %s\n", NeAACDecGetErrorMessage(FI.error));
		ctx.sampleCount = 0;
		ctx.samplesUsed = 0;
		return nullptr;
	}
	const uint8_t sampleBytes = fileInfo().bitsPerSample / 8;
	ctx.sampleCount = FI.samples * sampleBytes;
	ctx.samplesUsed = 0;
	return ctx.decodeBuffer;
}

int64_t aac_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	const auto buffer = static_cast<uint8_t *>(bufferPtr);
	uint32_t offset = 0;
	auto &ctx = *context();

	if (ctx.eof)
		return -2;
	while (offset < length && !ctx.eof)
	{
		if (ctx.samplesUsed == ctx.sampleCount)
		{
			if (!nextFrame())
			{
				if (!ctx.eof)
					return offset;
				continue;
			}
		}
		uint8_t *const decodeBuffer = ctx.decodeBuffer;

		const uint32_t count = std::min(ctx.sampleCount, uint64_t{length - offset});
		memcpy(buffer + offset, decodeBuffer + ctx.samplesUsed, count);
		ctx.samplesUsed += count;
		offset += count;
	}

	return offset;
}

/*!
 * Plays an opened AAC file using OpenAL on the default audio device
 * @param p_AACFile A pointer to a file opened with \c AAC_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c AAC_OpenR() used to open the file at \p p_AACFile,
 * this function will do nothing.
 * @bug \p p_AACFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_AACFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void AAC_Play(void *p_AACFile) { return audioPlay(p_AACFile); }
void AAC_Pause(void *p_AACFile) { return audioPause(p_AACFile); }
void AAC_Stop(void *p_AACFile) { return audioStop(p_AACFile); }

/*!
 * Checks the file given by \p FileName for whether it is an AAC
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an AAC file or not
 */
bool Is_AAC(const char *FileName) { return aac_t::isAAC(FileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a MP3
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a MP3 file or not
 */
bool aac_t::isAAC(const int32_t fd) noexcept
{
	uint8_t aacSig[2];
	if (fd == -1 ||
		read(fd, aacSig, 2) != 2 ||
		lseek(fd, 0, SEEK_SET) != 0)
		return false;
	// Detect an ADTS header:
	aacSig[1] &= 0xF6;
	if (aacSig[0] != 0xFF || aacSig[1] != 0xF0)
		return false;
	// not going to bother detecting ADIF yet..
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
bool aac_t::isAAC(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isAAC(file);
}

/*!
 * @internal
 * This structure controls decoding AAC files when using the high-level API on them
 */
API_Functions AACDecoder =
{
	AAC_OpenR,
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
