#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <conio.h>
#include <windows.h>
#endif
#include <string.h>

#include <neaacdec.h>

#include "libAudio.h"
#include "libAudio_Common.h"

/* Following 2 definitions are taken from aacinfo.c from faad2: */
#define ADIF_MAX_SIZE 30 /* Should be enough */
//#define ADTS_MAX_SIZE 10 /* Should be enough */

/* ffmpeg's aac_parser.c specifies the following for ADTS_MAX_SIZE: */
#define ADTS_MAX_SIZE 7

typedef struct _AAC_Intern
{
	FILE *f_AAC;
	NeAACDecHandle p_dec;
	FileInfo *p_FI;
	int nLoop, nCurrLoop;
	bool eof;
	BYTE buffer[8192];
	BYTE FrameHeader[ADTS_MAX_SIZE];
	Playback *p_Playback;
} AAC_Intern;

void *AAC_OpenR(char *FileName)
{
	AAC_Intern *ret = NULL;
	FILE *f_AAC = NULL;

	ret = (AAC_Intern *)malloc(sizeof(AAC_Intern));
	if (ret == NULL)
		return ret;

	ret->f_AAC = f_AAC = fopen(FileName, "rb");
	if (f_AAC == NULL)
	{
		free(ret);
		return f_AAC;
	}

	ret->eof = false;
	ret->p_dec = NeAACDecOpen();

	return ret;
}

FileInfo *AAC_GetFileInfo(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	FileInfo *ret = NULL;
	NeAACDecConfiguration *ADC;

	p_AF->p_FI = ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	fread(p_AF->FrameHeader, ADTS_MAX_SIZE, 1, p_AF->f_AAC);
	fseek(p_AF->f_AAC, -(ADTS_MAX_SIZE), SEEK_CUR);
	NeAACDecInit(p_AF->p_dec, p_AF->FrameHeader, ADTS_MAX_SIZE, (ULONG *)&ret->BitRate, (BYTE *)&ret->Channels);
	ADC = NeAACDecGetCurrentConfiguration(p_AF->p_dec);
	ret->BitsPerSample = 16;

	if (ExternalPlayback == 0)
		p_AF->p_Playback = new Playback(ret, AAC_FillBuffer, p_AF->buffer, 8192, p_AACFile);
	ADC->outputFormat = FAAD_FMT_16BIT;
	NeAACDecSetConfiguration(p_AF->p_dec, ADC);

	return ret;
}

int AAC_CloseFileR(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	FILE *f_AAC = p_AF->f_AAC;
	int ret;

	delete p_AF->p_Playback;

	NeAACDecClose(p_AF->p_dec);

	ret = fclose(f_AAC);
	free(p_AF);
	return ret;
}

typedef struct _BitStream
{
	BYTE *Data;
	long NumBit;
	long Size;
	long CurrentBit;
} BitStream;

BitStream *OpenBitStream(int size, BYTE *buffer)
{
	BitStream *BS = (BitStream *)malloc(sizeof(BitStream));
	BS->Size = size;
	BS->NumBit = size * 8;
	BS->CurrentBit = 0;
	BS->Data = buffer;
	return BS;
}

void CloseBitStream(BitStream *BS)
{
	free(BS);
}

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

void SkipBit(BitStream *BS, int NumBits)
{
	BS->CurrentBit += NumBits;
}

long AAC_FillBuffer(void *p_AACFile, BYTE *OutBuffer, int nOutBufferLen)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	FILE *f_AAC = p_AF->f_AAC;
	BYTE *OBuf = OutBuffer, *FrameHeader = p_AF->FrameHeader;

	if (p_AF->eof == true)
		return -2;
	while ((OBuf - OutBuffer) < nOutBufferLen && p_AF->eof == false)
	{
		static BYTE *Buff2 = NULL;
		BYTE *Buff = NULL;
		UINT FrameLength = 0, read = 0;
		NeAACDecFrameInfo FI;
		BitStream *BS = OpenBitStream(ADTS_MAX_SIZE, FrameHeader);

		read = fread(FrameHeader, 1, ADTS_MAX_SIZE, f_AAC);
		if (feof(f_AAC) != FALSE)
		{
			p_AF->eof = true;
			if (read != ADTS_MAX_SIZE)
			{
				CloseBitStream(BS);
				return OBuf - OutBuffer;
			}
			continue;
		}
		read = (UINT)GetBit(BS, 12);
		if (read != 0xFFF)
		{
			p_AF->eof = true;
			CloseBitStream(BS);
			continue;
		}
		SkipBit(BS, 18);
		FrameLength = (UINT)GetBit(BS, 13);
		CloseBitStream(BS);
		Buff = (BYTE *)malloc(FrameLength);
		memcpy(Buff, FrameHeader, ADTS_MAX_SIZE);
		read = fread(Buff + ADTS_MAX_SIZE, 1, FrameLength - ADTS_MAX_SIZE, f_AAC);
		if (feof(f_AAC) != FALSE)
		{
			p_AF->eof = true;
			if (read != FrameLength)
			{
				free(Buff);
				return OBuf - OutBuffer;
			}
		}

		Buff2 = (BYTE *)NeAACDecDecode(p_AF->p_dec, &FI, Buff, FrameLength);
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

void AAC_Play(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;

	p_AF->p_Playback->Play();
}

bool Is_AAC(char *FileName)
{
	FILE *f_AAC = fopen(FileName, "rb");
	BYTE sig[2];

	if (f_AAC == NULL)
		return false;

	fread(sig, 2, 1, f_AAC);
	fclose(f_AAC);

	// Detect an ADTS header:
	sig[1] = sig[1] & 0xF6;
	if (sig[0] != 0xFF || sig[1] != 0xF0)
		return false;
	// not going to bother detecting ADIF yet..

	return true;
}
