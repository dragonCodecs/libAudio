#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <malloc.h>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadWAV.cpp
 * @brief The implementation of the WAV decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2013
 */

/*!
 * @internal
 * Internal structure for holding the decoding context for a given WAV file
 */
struct wav_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	/*!
	 * @internal
	 * The byte possition where the final byte of the data chunk should be in the file
	 */
	uint32_t offsetDataLength;
	/*!
	 * @internal
	 * A flag indicating if this WAV's data is floating point
	 */
	bool floatData;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
};

typedef struct _WAV_Intern
{
	/*!
	 * @internal
	 * The compression flags read from the WAV file
	 */
	uint16_t compression;

	wav_t inner;

	_WAV_Intern(const char *const fileName) : inner(fd_t(fileName, O_RDONLY | O_NOCTTY)) { }
} WAV_Intern;

wav_t::wav_t(fd_t &&fd) noexcept : audioFile_t(audioType_t::wave, std::move(fd)),
	ctx(makeUnique<decoderContext_t>()) { }
wav_t::decoderContext_t::decoderContext_t() noexcept { }

bool read(const fd_t &fd, uint8_t &value) noexcept
	{ return fd.read(value); }

bool read(const fd_t &fd, uint16_t &value) noexcept
{
	std::array<uint8_t, 2> data{};
	const bool result = fd.read(data);
	value = (uint16_t(data[1]) << 8) | data[0];
	return result;
}

bool read(const fd_t &fd, uint32_t &value) noexcept
{
	std::array<uint8_t, 4> data{};
	const bool result = fd.read(data);
	value = (uint32_t(data[3]) << 24) | (uint32_t(data[2]) << 16) |
		(uint32_t(data[1]) << 8) | data[0];
	return result;
}

constexpr std::array<char, 4> waveFormatChunk{'f', 'm', 't', ' '};
constexpr std::array<char, 4> waveDataChunk{'d', 'a', 't', 'a'};

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
		if (!read(file, chunkLength) ||
			chunkLength > (fileSize - offset) ||
			file.seek(chunkLength, SEEK_CUR) != (offset + chunkLength + 4) ||
			!file.read(chunkTag))
			return false;
		offset += chunkLength + 8;
	}
	return chunkTag == chunkName;
}

bool wav_t::readFormat(void *intern) noexcept
{
	auto p_WF = static_cast<WAV_Intern *>(intern);
	auto &ctx = *context();
	fileInfo_t &info = fileInfo();
	const fd_t &file = fd();
	std::array<char, 6> unused;
	uint16_t channels, bitsPerSample;

	if (!read(file, p_WF->compression) ||
		!read(file, channels) ||
		!read(file, info.bitRate) ||
		!file.read(unused) ||
		!read(file, bitsPerSample) ||
		!channels ||
		!info.bitRate ||
		!bitsPerSample ||
		(bitsPerSample % 8))
		return false;
	info.channels = channels;
	info.bitsPerSample = bitsPerSample;
	ctx.floatData = p_WF->compression == 3;
	return true;
}

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by WAV_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *WAV_OpenR(const char *FileName)
{
	std::unique_ptr<WAV_Intern> ret = makeUnique<WAV_Intern>(FileName);
	if (!ret || !wav_t::isWAV(ret->inner.fd()))
		return nullptr;
	auto &ctx = *ret->inner.context();
	fileInfo_t &info = ret->inner.fileInfo();
	const fd_t &file = ret->inner.fd();
	const off_t fileSize = file.length();
	uint32_t chunkLength = 0;

	if (fileSize == -1 ||
		file.seek(4, SEEK_SET) != 4 ||
		!read(file, chunkLength) ||
		chunkLength > (fileSize - 8) ||
		file.seek(4, SEEK_CUR) != 12 ||
		!ret->inner.skipToChunk(waveFormatChunk))
		return nullptr;
	off_t offset = file.tell();
	if (offset == -1 ||
		!read(file, chunkLength) ||
		chunkLength > (fileSize - offset - 4) ||
		chunkLength < 16 ||
		!ret->inner.readFormat(ret.get()))
		return nullptr;

	// Currently we do not care if the file has extra data, we're only looking to work with PCM.
	for (uint32_t i = 16; i < chunkLength; ++i)
	{
		char value = 0;
		if (!file.read(value))
			return nullptr;
	}

	if (!ret->inner.skipToChunk(waveDataChunk) ||
		!read(file, chunkLength) ||
		(offset = file.tell()) == -1 ||
		chunkLength > (fileSize - offset) ||
		file.isEOF())
		return nullptr;
	info.totalTime = chunkLength / info.channels;
	info.totalTime /= info.bitsPerSample / 8;
	info.totalTime /= info.bitRate;
	ctx.offsetDataLength = chunkLength + file.tell();

	if (ExternalPlayback == 0)
		ret->inner.player(makeUnique<playback_t>(ret.get(), WAV_FillBuffer, ctx.playbackBuffer, 8192, info));

	return ret.release();
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_WAVFile A pointer to a file opened with \c WAV_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c WAV_Play() or \c WAV_FillBuffer()
 * @bug \p p_WAVFile must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *WAV_GetFileInfo(void *p_WAVFile)
{
	WAV_Intern *p_WF = (WAV_Intern *)p_WAVFile;
	return audioFileInfo(&p_WF->inner);
}

wav_t::decoderContext_t::~decoderContext_t() noexcept { }

/*!
 * Closes an opened audio file
 * @param p_WAVFile A pointer to a file opened with \c WAV_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_WAVFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_WAVFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int WAV_CloseFileR(void *p_WAVFile)
{
	WAV_Intern *p_WF = (WAV_Intern *)p_WAVFile;
	delete p_WF;
	return 0;
}

int8_t dataToSample(const std::array<uint8_t, 1> &data) noexcept
	{ return int8_t(data[0]) ^ 0x80; }
int16_t dataToSample(const std::array<uint8_t, 2> &data) noexcept
	{ return int16_t((uint16_t(data[1]) << 8) | data[0]); }
int16_t dataToSample(const std::array<uint8_t, 3> &data) noexcept
	{ return int16_t((uint16_t(data[2]) << 8) | data[1]); }
int16_t dataToSample(const std::array<uint8_t, 4> &data) noexcept
	{ return int16_t((uint16_t(data[3]) << 8) | data[2]); }

template<typename T, uint8_t N> uint32_t readSamples(wav_t &wavFile, void *buffer,
	const uint32_t length, const uint32_t sampleByteCount)
{
	const auto playbackBuffer = static_cast<T *>(buffer);
	uint32_t offset = 0;
	const fd_t &file = wavFile.fd();
	for (uint32_t index = 0; offset < length && offset < sampleByteCount; ++index)
	{
		std::array<uint8_t, N> data{};
		if (!file.read(data))
			break;
		playbackBuffer[index] = dataToSample(data);
		offset += N;
	}
	return offset;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_WAVFile A pointer to a file opened with \c WAV_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_WAVFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long WAV_FillBuffer(void *p_WAVFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	WAV_Intern *p_WF = (WAV_Intern *)p_WAVFile;
	return audioFillBuffer(&p_WF->inner, OutBuffer, nOutBufferLen);
}

int64_t wav_t::fillBuffer(void *const buffer, const uint32_t length)
{
	uint32_t offset = 0;
	const fileInfo_t &info = fileInfo();
	const fd_t &file = fd();
	auto &ctx = *context();

	const off_t fileOffset = file.tell();
	if (file.isEOF() || fileOffset == -1 || fileOffset >= ctx.offsetDataLength)
		return -2;
	const uint32_t sampleByteCount = ctx.offsetDataLength - fileOffset;
	// 8-bit char reader
	if (!ctx.floatData && info.bitsPerSample == 8)
		return readSamples<int8_t, 1>(*this, buffer, length, sampleByteCount);
	// 16-bit short reader
	else if (!ctx.floatData && info.bitsPerSample == 16)
		return readSamples<int16_t, 2>(*this, buffer, length, sampleByteCount);
	// 24-bit int reader
	else if (!ctx.floatData && info.bitsPerSample == 24)
		return readSamples<int16_t, 3>(*this, buffer, length, sampleByteCount);
	// 32-bit int reader
	else if (!ctx.floatData && info.bitsPerSample == 32)
		return readSamples<int16_t, 4>(*this, buffer, length, sampleByteCount);
	// 32-bit float reader
	/*else if (ctx.floatData && info.bitsPerSample == 32)
	{
		for (int i = 0; i < nOutBufferLen && ftell(f_WAV) < p_WF->DataEnd; i += 2)
		{
			float in;
			fread(&in, 4, 1, f_WAV);
			if (in > -2.0F && in < 2.0F)
				*((short *)(OutBuffer + ret)) = (short)(in * 32767.5F);
			else
				*((short *)(OutBuffer + ret)) = (short)((in / ((float)((int)in))) * 65535.0F);
			ret += 2;
		}
	}
	// 24-bit float reader
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
	else
		return -1;

	return offset;
}

/*!
 * Plays an opened WAV file using OpenAL on the default audio device
 * @param p_WAVFile A pointer to a file opened with \c WAV_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c WAV_OpenR() used to open the file at \p p_WAVFile,
 * this function will do nothing.
 * @bug \p p_WAVFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_WAVFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void WAV_Play(void *p_WAVFile)
{
	WAV_Intern *p_WF = (WAV_Intern *)p_WAVFile;
	p_WF->inner.play();
}

void WAV_Pause(void *p_WAVFile)
{
	WAV_Intern *p_WF = (WAV_Intern *)p_WAVFile;
	p_WF->inner.pause();
}

void WAV_Stop(void *p_WAVFile)
{
	WAV_Intern *p_WF = (WAV_Intern *)p_WAVFile;
	p_WF->inner.stop();
}

/*!
 * Checks the file given by \p FileName for whether it is a WAV
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a WAV file or not
 */
bool Is_WAV(const char *FileName) { return wav_t::isWAV(FileName); }

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
	char riffSig[4], waveSig[4];
	if (fd == -1 ||
		read(fd, riffSig, 4) != 4 ||
		lseek(fd, 4, SEEK_CUR) != 8 ||
		read(fd, waveSig, 4) != 4 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		strncmp(riffSig, "RIFF", 4) != 0 ||
		strncmp(waveSig, "WAVE", 4) != 0)
		return false;
	return true;
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

/*!
 * @internal
 * This structure controls decoding WAV files when using the high-level API on them
 */
API_Functions WAVDecoder =
{
	WAV_OpenR,
	WAV_GetFileInfo,
	WAV_FillBuffer,
	WAV_CloseFileR,
	WAV_Play,
	WAV_Pause,
	WAV_Stop
};
