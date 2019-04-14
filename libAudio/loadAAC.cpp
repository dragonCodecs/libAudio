#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <conio.h>
#include <windows.h>
#endif

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

struct aac_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The decoder context handle
	 */
	NeAACDecHandle decoder;

	decoderContext_t();
	~decoderContext_t() noexcept;
};

/*!
 * @internal
 * Internal structure for holding the decoding context for a given AAC file
 */
typedef struct _AAC_Intern
{
	/*!
	 * @internal
	 * The \c FileInfo for the AAC file being decoded
	 */
	FileInfo *p_FI;
	/*!
	 * @internal
	 * @var int nLoop
	 * The number of frames decoded relative to the total number
	 * @var int nCurrLoop
	 * The total number of frames to decode
	 */
	int nLoop, nCurrLoop;
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t buffer[8192];
	/*!
	 * @internal
	 * The frame headers read at the start of the file
	 */
	uint8_t FrameHeader[ADTS_MAX_SIZE];

	aac_t inner;

	_AAC_Intern(const char *const fileName) : inner(fd_t(fileName, O_RDONLY | O_NOCTTY)) { }
} AAC_Intern;

aac_t::aac_t(fd_t &&fd) noexcept : audioFile_t(audioType_t::aac, std::move(fd)), ctx(makeUnique<decoderContext_t>()) { }
aac_t::decoderContext_t::decoderContext_t() : decoder{NeAACDecOpen()} { }

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by AAC_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *AAC_OpenR(const char *FileName)
{
	AAC_Intern *ret = NULL;

	ret = new (std::nothrow) AAC_Intern(FileName);
	if (!ret || !ret->inner.context())
		return nullptr;

	ret->eof = false;

	return ret;
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_AACFile A pointer to a file opened with \c AAC_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c AAC_Play() or \c AAC_FillBuffer()
 * @bug \p p_AACFile must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *AAC_GetFileInfo(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	FileInfo *ret = NULL;
	NeAACDecConfiguration *ADC;
	const fd_t &fd = p_AF->inner.fd();
	auto &ctx = *p_AF->inner.context();

	p_AF->p_FI = ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	fd.read(p_AF->FrameHeader, ADTS_MAX_SIZE);
	fd.seek(-ADTS_MAX_SIZE, SEEK_CUR);
	NeAACDecInit(ctx.decoder, p_AF->FrameHeader, ADTS_MAX_SIZE, (ULONG *)&ret->BitRate, (uint8_t *)&ret->Channels);
	ADC = NeAACDecGetCurrentConfiguration(ctx.decoder);
	ret->BitsPerSample = 16;

	if (ExternalPlayback == 0)
		p_AF->inner.player(makeUnique<playback_t>(p_AACFile, AAC_FillBuffer, p_AF->buffer, 8192, ret));
	ADC->outputFormat = FAAD_FMT_16BIT;
	NeAACDecSetConfiguration(ctx.decoder, ADC);

	return ret;
}

aac_t::decoderContext_t::~decoderContext_t() noexcept
{
	NeAACDecClose(decoder);
}

/*!
 * Closes an opened audio file
 * @param p_AACFile A pointer to a file opened with \c AAC_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_AACFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_AACFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int AAC_CloseFileR(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	delete p_AF;
	return 0;
}

/*!
 * @internal
 * Internal structure used to read the AAC bitstream so that packets
 * of data can be sent into FAAD2 correctly as packets so to ease the
 * job of decoding
 */
struct BitStream final
{
	/*!
	 * @internal
	 * The data buffer being used as a bitstream
	 */
	uint8_t *Data;
	/*!
	 * @internal
	 * The total number of bits available in the buffer
	 */
	long NumBit;
	/*!
	 * @internal
	 * The total size of the bit-buffer in bytes
	 */
	long Size;
	/*!
	 * @internal
	 * The index of the current bit relative to the total
	 *   number of bits available in the buffer
	 */
	long CurrentBit;
};

/*!
 * @internal
 * Internal function used to open a buffer as a bitstream
 * @param size The length of the buffer to be used
 * @param buffer The buffer to be used
 */
std::unique_ptr<BitStream> OpenBitStream(int size, uint8_t *buffer)
{
	auto BS = makeUnique<BitStream>();
	BS->Size = size;
	BS->NumBit = size * 8;
	BS->CurrentBit = 0;
	BS->Data = buffer;
	return BS;
}

/*!
 * @internal
 * Internal function used to get the next \p NumBits bits sequentially from the bitstream
 * @param BS The bitstream to get the bits from
 * @param NumBits The number of bits to get
 */
ULONG GetBit(BitStream *BS, int NumBits)
{
	int Num = 0;
	ULONG ret = 0;

	if (NumBits == 0)
		return 0;
	while (Num < NumBits)
	{
		int Byte = BS->CurrentBit / 8;
		int Bit = 7 - (BS->CurrentBit % 8);
		ret = ret << 1;
		ret += ((BS->Data[Byte] & (1 << Bit)) >> Bit);
		BS->CurrentBit++;
		if (BS->CurrentBit == BS->NumBit)
			return ret;
		Num++;
	}
	return ret;
}

/*!
 * @internal
 * Internal function used to skip a number of bits in the bitstream
 * @param BS The bitstream to skip the bits in
 * @param NumBits The number of bits to skip
 */
void SkipBit(BitStream *BS, int NumBits)
{
	BS->CurrentBit += NumBits;
}

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
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	uint8_t *OBuf = OutBuffer, *FrameHeader = p_AF->FrameHeader;
	const fd_t &fd = p_AF->inner.fd();
	auto &ctx = *p_AF->inner.context();

	if (p_AF->eof == true)
		return -2;
	while ((OBuf - OutBuffer) < nOutBufferLen && p_AF->eof == false)
	{
		static uint8_t *Buff2 = NULL;
		uint8_t *Buff = NULL;
		uint32_t FrameLength = 0;
		NeAACDecFrameInfo FI;
		auto BS = OpenBitStream(ADTS_MAX_SIZE, FrameHeader);

		if (!fd.read(FrameHeader, ADTS_MAX_SIZE) ||
			fd.isEOF())
		{
			p_AF->eof = fd.isEOF();
			if (!p_AF->eof)
				return OBuf - OutBuffer;
			continue;
		}
		const uint32_t read = (uint32_t)GetBit(BS.get(), 12);
		if (read != 0xFFF)
		{
			p_AF->eof = true;
			continue;
		}
		SkipBit(BS.get(), 18);
		FrameLength = (uint32_t)GetBit(BS.get(), 13);
		Buff = (uint8_t *)malloc(FrameLength);
		memcpy(Buff, FrameHeader, ADTS_MAX_SIZE);
		if (!fd.read(Buff + ADTS_MAX_SIZE, FrameLength - ADTS_MAX_SIZE) ||
			fd.isEOF())
		{
			p_AF->eof = fd.isEOF();
			if (!p_AF->eof)
			{
				free(Buff);
				return OBuf - OutBuffer;
			}
		}

		Buff2 = (uint8_t *)NeAACDecDecode(ctx.decoder, &FI, Buff, FrameLength);
		free(Buff);

		if (FI.error != 0)
		{
			printf("Error: %s\n", NeAACDecGetErrorMessage(FI.error));
			continue;
		}

		memcpy(OBuf, Buff2, (FI.samples * (p_AF->p_FI->BitsPerSample / 8)));
		OBuf += (FI.samples * (p_AF->p_FI->BitsPerSample / 8));
	}

	return OBuf - OutBuffer;
}

int64_t aac_t::fillBuffer(void *const buffer, const uint32_t length) { return -1; }

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
void AAC_Play(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	p_AF->inner.play();
}

void AAC_Pause(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	p_AF->inner.pause();
}

void AAC_Stop(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	p_AF->inner.stop();
}

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
		read(fd, aacSig, 2) != 2)
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
	AAC_GetFileInfo,
	AAC_FillBuffer,
	AAC_CloseFileR,
	AAC_Play,
	AAC_Pause,
	AAC_Stop
};
