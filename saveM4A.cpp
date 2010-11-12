#ifndef __NO_SAVE_M4A__

#include <faac.h>
#ifdef _WINDOWS
#include <mp4.h>
#else
#include <mp4v2/mp4v2.h>
#endif

#ifdef strncasecmp
#undef strncasecmp
#endif

#include "libAudio.h"
#include "libAudio_Common.h"

typedef struct _M4A_Enc_Intern
{
	faacEncHandle p_enc;
	FILE *f_AAC;
	ULONG MaxInSamp;
	ULONG MaxOutByte;
	MP4FileHandle p_mp4;
	MP4TrackId track;
	bool err;
	int Channels;
} M4A_Enc_Intern;

void *M4A_OpenW(char *FileName)
{
	M4A_Enc_Intern *ret = NULL;
	FILE *f_AAC = NULL;

	f_AAC = fopen(FileName, "wb+");
	if (f_AAC == NULL)
		return ret;

	ret = (M4A_Enc_Intern *)malloc(sizeof(M4A_Enc_Intern));
	if (ret == NULL)
		return ret;

	ret->f_AAC = f_AAC;
	ret->err = false;

	ret->p_mp4 = MP4LatchCreate(f_AAC, fwrite, MP4_DETAILS_ERROR);// | MP4_DETAILS_WRITE_ALL);

	return ret;
}

void M4A_SetFileInfo(void *p_AACFile, FileInfo *p_FI)
{
	M4A_Enc_Intern *p_AF = (M4A_Enc_Intern *)p_AACFile;
	faacEncConfigurationPtr p_conf;
	UCHAR *ASC;
	ULONG lenASC;

	p_AF->Channels = p_FI->Channels;
	p_AF->p_enc = faacEncOpen(p_FI->BitRate, p_FI->Channels, &p_AF->MaxInSamp, &p_AF->MaxOutByte);
	if (p_AF->p_enc == NULL)
	{
		p_AF->err = true;
		return;
	}
	MP4SetTimeScale(p_AF->p_mp4, p_FI->BitRate);
	p_AF->track = MP4AddAudioTrack(p_AF->p_mp4, p_FI->BitRate, 1024);//p_AF->MaxInSamp / p_FI->Channels, MP4_MPEG4_AUDIO_TYPE);
	MP4SetAudioProfileLevel(p_AF->p_mp4, 0x0F);

	if (p_FI->Album != NULL)
		MP4SetMetadataAlbum(p_AF->p_mp4, p_FI->Album);
	if (p_FI->Artist != NULL)
		MP4SetMetadataArtist(p_AF->p_mp4, p_FI->Artist);
	if (p_FI->Title != NULL)
		MP4SetMetadataTool(p_AF->p_mp4, p_FI->Title);

	MP4SetMetadataWriter(p_AF->p_mp4, "libAudio 0.5.78");

	p_conf = faacEncGetCurrentConfiguration(p_AF->p_enc);
	if (p_conf == NULL)
	{
		p_AF->err = true;
		return;
	}
	p_conf->inputFormat = FAAC_INPUT_16BIT;
	p_conf->mpegVersion = MPEG4;
	p_conf->bitRate = 128000;
//	p_conf->bitRate = 64000;
	p_conf->outputFormat = 0;
	p_conf->useLfe = p_conf->useTns = p_conf->allowMidside = 0;
	p_conf->aacObjectType = LOW;//MAIN;
	p_conf->bandWidth = 0;
	p_conf->quantqual = 100;
	if (faacEncSetConfiguration(p_AF->p_enc, p_conf) == 0)
	{
		p_AF->err = true;
		return;
	}
	if (faacEncGetDecoderSpecificInfo(p_AF->p_enc, &ASC, &lenASC) != 0)
	{
		return;
		p_AF->err = true;
	}

	MP4SetTrackESConfiguration(p_AF->p_mp4, p_AF->track, ASC, lenASC);

	free(ASC);
}

long M4A_WriteBuffer(void *p_AACFile, BYTE *InBuffer, int nInBufferLen)
{
	M4A_Enc_Intern *p_AF = (M4A_Enc_Intern *)p_AACFile;
	int nOB, j = 0;
	UCHAR *OB = NULL;

	if (p_AF->p_enc == NULL)
		return -3;
	if (p_AF->err == true)
		return -4;

	if (nInBufferLen == -2)
	{
		OB = (UCHAR *)malloc(p_AF->MaxOutByte);

		nOB = faacEncEncode(p_AF->p_enc, NULL, 0, OB, p_AF->MaxOutByte);
		if (nOB > 0)
			MP4WriteSample(p_AF->p_mp4, p_AF->track, OB, nOB);
		nOB = faacEncEncode(p_AF->p_enc, NULL, 0, OB, p_AF->MaxOutByte);
		if (nOB > 0)
			MP4WriteSample(p_AF->p_mp4, p_AF->track, OB, nOB);
		nOB = faacEncEncode(p_AF->p_enc, NULL, 0, OB, p_AF->MaxOutByte);
		if (nOB > 0)
			MP4WriteSample(p_AF->p_mp4, p_AF->track, OB, nOB);
		nOB = faacEncEncode(p_AF->p_enc, NULL, 0, OB, p_AF->MaxOutByte);
		if (nOB > 0)
			MP4WriteSample(p_AF->p_mp4, p_AF->track, OB, nOB);

		free(OB);
		return -2;
	}

	OB = (UCHAR *)malloc(p_AF->MaxOutByte);
	while (j < nInBufferLen)
	{
		if (j + ((int)p_AF->MaxInSamp * 2) > nInBufferLen)
		{
			nOB = faacEncEncode(p_AF->p_enc, (int *)(InBuffer + j), (nInBufferLen - j) / 2, OB, p_AF->MaxOutByte);
			j = nInBufferLen;
		}
		else
		{
			nOB = faacEncEncode(p_AF->p_enc, (int *)(InBuffer + j), p_AF->MaxInSamp, OB, p_AF->MaxOutByte);
			j += p_AF->MaxInSamp * 2;
		}

		if (nOB < 0)
			break;
		if (nOB == 0)
			continue;

		MP4WriteSample(p_AF->p_mp4, p_AF->track, OB, nOB);
	}
	free(OB);

	return nInBufferLen;
}

int M4A_CloseFileW(void *p_AACFile)
{
	M4A_Enc_Intern *p_AF = (M4A_Enc_Intern *)p_AACFile;
	int ret = 0;

	MP4Close(p_AF->p_mp4);
	faacEncClose(p_AF->p_enc);

	ret = fclose(p_AF->f_AAC);
	free(p_AF);
	return ret;
}

#endif /*__NO_SAVE_M4A__*/
