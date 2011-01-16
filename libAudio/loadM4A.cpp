#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <conio.h>
#include <windows.h>
#endif
#include <string.h>

#include <neaacdec.h>
#include <mp4v2/mp4v2.h>

#include "libAudio.h"
#include "libAudio_Common.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct _M4A_Intern
{
	NeAACDecHandle p_dec;
	MP4FileHandle p_MP4;
	MP4TrackId nTrack;
	uint32_t ActualSampleRate;
	uint8_t ActualChannels;
	FileInfo *p_FI;
	int nLoops, nCurrLoop, samplesUsed, nSamples;
	uint8_t *p_Samples;
	bool eof;
	uint8_t buffer[8192];
	Playback *p_Playback;
} M4A_Intern;

void *MP4DecOpen(const char *FileName, MP4FileMode Mode);
int MP4DecSeek(void *MP4File, int64_t pos);
int MP4DecRead(void *MP4File, void *DataOut, int64_t DataOutLen, int64_t *Read, int64_t);
int MP4DecWrite(void *MP4File, const void *DataIn, int64_t DataInLen, int64_t *Written, int64_t);
int MP4DecClose(void *MP4File);

MP4FileProvider MP4DecFunctions =
{
	MP4DecOpen,
	MP4DecSeek,
	MP4DecRead,
	MP4DecWrite,
	MP4DecClose
};

void *MP4DecOpen(const char *FileName, MP4FileMode Mode)
{
	if (Mode != FILEMODE_READ)
		return NULL;
	return fopen(FileName, "rb");
}

int MP4DecSeek(void *MP4File, int64_t pos)
{
#ifdef _WINDOWS
	return (_fseeki64((FILE *)MP4File, pos, SEEK_SET) == 0 ? FALSE : TRUE);
#else
	return (fseeko64((FILE *)MP4File, pos, SEEK_SET) == 0 ? FALSE : TRUE);
#endif
}

int MP4DecRead(void *MP4File, void *DataOut, int64_t DataOutLen, int64_t *Read, int64_t)
{
	int ret = fread(DataOut, 1, (size_t)DataOutLen, (FILE *)MP4File);
	if (ret <= 0 && DataOutLen != 0)
		return TRUE;
	*Read = ret;
	return FALSE;
}

int MP4DecWrite(void *MP4File, const void *DataIn, int64_t DataInLen, int64_t *Written, int64_t)
{
	if (fwrite(DataIn, 1, (size_t)DataInLen, (FILE *)MP4File) != DataInLen)
		return TRUE;
	*Written = DataInLen;
	return FALSE;
}

int MP4DecClose(void *MP4File)
{
	return (fclose((FILE *)MP4File) == 0 ? FALSE : TRUE);
}

MP4TrackId GetAACTrack(M4A_Intern *ret)
{
	/* find AAC track */
	int i, numTracks = MP4GetNumberOfTracks(ret->p_MP4, NULL, 0);

	for (i = 0; i < numTracks; i++)
	{
		NeAACDecConfiguration *ADC;
		uint8_t *Buff = NULL;
		uint32_t BuffLen = 0;
		MP4TrackId Track = MP4FindTrackId(ret->p_MP4, i, NULL, 0);
		const char *TrackType = MP4GetTrackType(ret->p_MP4, Track);

		if (!MP4_IS_AUDIO_TRACK_TYPE(TrackType))
			continue;

		MP4GetTrackESConfiguration(ret->p_MP4, Track, &Buff, &BuffLen);

		NeAACDecInit(ret->p_dec, Buff, BuffLen, (unsigned long *)&ret->ActualSampleRate, &ret->ActualChannels);
		ADC = NeAACDecGetCurrentConfiguration(ret->p_dec);
		ADC->outputFormat = FAAD_FMT_16BIT;
		NeAACDecSetConfiguration(ret->p_dec, ADC);
		free(Buff);
		return Track;
	}

	/* can't decode this */
	return -1;
}

void *M4A_OpenR(const char *FileName)
{
	M4A_Intern *ret = NULL;
	FILE *f_M4A = NULL;

	ret = (M4A_Intern *)malloc(sizeof(M4A_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(M4A_Intern));

	ret->eof = false;
	ret->p_dec = NeAACDecOpen();
	ret->p_MP4 = MP4ReadProvider(FileName, 0, &MP4DecFunctions);
	ret->nTrack = GetAACTrack(ret);

	return ret;
}

FileInfo *M4A_GetFileInfo(void *p_M4AFile)
{
	uint32_t timescale, duration;
	char *value;
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;
	FileInfo *ret = NULL;

	p_MF->p_FI = ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	ret->BitRate = p_MF->ActualSampleRate;
	ret->Channels = p_MF->ActualChannels;

	MP4GetMetadataAlbum(p_MF->p_MP4, &ret->Album);
	MP4GetMetadataArtist(p_MF->p_MP4, &ret->Artist);
	MP4GetMetadataName(p_MF->p_MP4, &ret->Title);
	//ret->BitRate = mp4ff_get_avg_bitrate(p_MF->p_MP4, p_MF->nTrack);
	//ret->BitsPerSample = InterpretFormat(ADC->outputFormat);
	ret->BitsPerSample = 16;
	MP4GetMetadataComment(p_MF->p_MP4, &value);
	if (value == NULL)
		ret->nOtherComments = 0;
	else
	{
		ret->nOtherComments = 1;
		ret->OtherComments.push_back(value);
	}
	timescale = MP4GetTrackTimeScale(p_MF->p_MP4, p_MF->nTrack);
	duration = MP4GetTrackDuration(p_MF->p_MP4, p_MF->nTrack);
	ret->TotalTime = ((double)duration) / ((double)timescale);
	p_MF->nLoops = MP4GetTrackNumberOfSamples(p_MF->p_MP4, p_MF->nTrack);
	p_MF->nCurrLoop = 0;

	if (ExternalPlayback == 0)
		p_MF->p_Playback = new Playback(ret, M4A_FillBuffer, p_MF->buffer, 8192, p_M4AFile);

	return ret;
}

int M4A_CloseFileR(void *p_M4AFile)
{
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;

	delete p_MF->p_Playback;

	NeAACDecClose(p_MF->p_dec);
	MP4Close(p_MF->p_MP4);

	free(p_MF);
	return 0;
}

long M4A_FillBuffer(void *p_M4AFile, BYTE *OutBuffer, int nOutBufferLen)
{
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;
	BYTE *OBuf = OutBuffer;

	while ((OBuf - OutBuffer) < nOutBufferLen && p_MF->eof == false)
	{
		uint32_t nUsed;
		if (p_MF->samplesUsed == p_MF->nSamples)
		{
			if (p_MF->nCurrLoop < p_MF->nLoops)
			{
				NeAACDecFrameInfo FI;
				uint8_t *Buff = NULL;
				uint32_t nBuff = 0;
				p_MF->nCurrLoop++;
				if (MP4ReadSample(p_MF->p_MP4, p_MF->nTrack, p_MF->nCurrLoop, &Buff, &nBuff) == false)
				{
					p_MF->eof = true;
					return -2;
				}
				p_MF->p_Samples = (BYTE *)NeAACDecDecode(p_MF->p_dec, &FI, Buff, nBuff);
				free(Buff);

				p_MF->nSamples = FI.samples;
				p_MF->samplesUsed = 0;
				if (FI.error != 0)
				{
					printf("Error: %s\n", NeAACDecGetErrorMessage(FI.error));
					p_MF->nSamples = 0;
					/*printf("\nPress Any Key To Continue....\n");
					getch();
					exit(FI.error);*/
					continue;
				}
			}
			else if (p_MF->nCurrLoop == p_MF->nLoops)
				return -1;

		}

		nUsed = min(p_MF->nSamples - p_MF->samplesUsed, (nOutBufferLen - (OBuf - OutBuffer)) / 2);
		memcpy(OBuf, p_MF->p_Samples + (p_MF->samplesUsed * 2), nUsed * 2);
		OBuf += nUsed * 2;
		p_MF->samplesUsed += nUsed;
	}

	return OBuf - OutBuffer;
}

void M4A_Play(void *p_M4AFile)
{
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;

	p_MF->p_Playback->Play();
}

// Standard "ftyp" Atom for a MOV based MP4 AAC file:
// 00 00 00 20 66 74 79 70 4D 34 41 20
// .  .  .     f  t  y  p  M  4  A

bool Is_M4A(const char *FileName)
{
	FILE *f_M4A = fopen(FileName, "rb");
	CHAR Len[4];
	char TypeSig[4];
	char FileType[4];

	if (f_M4A == NULL)
		return false;

	// How to deal with endieness:
	fread(&Len, 4, 1, f_M4A);
	//if (Len[3] != 32)
	//	return false;

	fread(TypeSig, 4, 1, f_M4A);
	fread(FileType, 4, 1, f_M4A);
	fclose(f_M4A);

	if (strncmp(TypeSig, "ftyp", 4) != 0 ||
		(strncmp(FileType, "M4A ", 4) != 0 &&
		strncmp(FileType, "mp42", 4) != 0))
		return false;

	return true;
}
