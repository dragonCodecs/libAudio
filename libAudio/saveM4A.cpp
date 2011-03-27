#include <faac.h>
#include <mp4v2/mp4v2.h>

#ifdef strncasecmp
#undef strncasecmp
#endif

#include "libAudio.h"
#include "libAudio_Common.h"

/*!
 * @internal
 * @file saveM4A.cpp
 * The implementation of the M4A/MP4 encoder API
 * @author Richard Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2011
 */

/*!
 * @internal
 * Internal structure for holding the encoding context for a given M4A/MP4 file
 */
typedef struct _M4A_Enc_Intern
{
	faacEncHandle p_enc;
	ULONG MaxInSamp;
	ULONG MaxOutByte;
	MP4FileHandle p_mp4;
	MP4TrackId track;
	bool err;
	int Channels;
} M4A_Enc_Intern;

void *MP4EncOpen(const char *FileName, MP4FileMode Mode);
int MP4EncSeek(void *MP4File, int64_t pos);
int MP4EncRead(void *MP4File, void *DataOut, int64_t DataOutLen, int64_t *Read, int64_t);
int MP4EncWrite(void *MP4File, const void *DataIn, int64_t DataInLen, int64_t *Written, int64_t);
int MP4EncClose(void *MP4File);

MP4FileProvider MP4EncFunctions =
{
	MP4EncOpen,
	MP4EncSeek,
	MP4EncRead,
	MP4EncWrite,
	MP4EncClose
};

void *MP4EncOpen(const char *FileName, MP4FileMode Mode)
{
	if (Mode != FILEMODE_CREATE)
		return NULL;
	return fopen(FileName, "wb+");
}

int MP4EncSeek(void *MP4File, int64_t pos)
{
#ifdef _WINDOWS
	return (_fseeki64((FILE *)MP4File, pos, SEEK_SET) == 0 ? FALSE : TRUE);
#else
	return (fseeko64((FILE *)MP4File, pos, SEEK_SET) == 0 ? FALSE : TRUE);
#endif
}

int MP4EncRead(void *MP4File, void *DataOut, int64_t DataOutLen, int64_t *Read, int64_t)
{
	int ret = fread(DataOut, 1, (size_t)DataOutLen, (FILE *)MP4File);
	if (ret <= 0 && DataOutLen != 0)
		return TRUE;
	*Read = ret;
	return FALSE;
}

int MP4EncWrite(void *MP4File, const void *DataIn, int64_t DataInLen, int64_t *Written, int64_t)
{
	if (fwrite(DataIn, 1, (size_t)DataInLen, (FILE *)MP4File) != DataInLen)
		return TRUE;
	*Written = DataInLen;
	return FALSE;
}

int MP4EncClose(void *MP4File)
{
	return (fclose((FILE *)MP4File) == 0 ? FALSE : TRUE);
}

void *M4A_OpenW(const char *FileName)
{
	M4A_Enc_Intern *ret = NULL;
	FILE *f_AAC = NULL;

	ret = (M4A_Enc_Intern *)malloc(sizeof(M4A_Enc_Intern));
	if (ret == NULL)
		return ret;

	ret->err = false;
	ret->p_mp4 = MP4CreateProvider(FileName, &MP4EncFunctions, MP4_DETAILS_ERROR);// | MP4_DETAILS_WRITE_ALL);

	return ret;
}

void M4A_SetFileInfo(void *p_AACFile, FileInfo *p_FI)
{
	const MP4Tags *p_Tags;
	faacEncConfigurationPtr p_conf;
	UCHAR *ASC;
	ULONG lenASC;
	M4A_Enc_Intern *p_AF = (M4A_Enc_Intern *)p_AACFile;

	p_Tags = MP4TagsAlloc();
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
		MP4TagsSetAlbum(p_Tags, p_FI->Album);
	if (p_FI->Artist != NULL)
		MP4TagsSetArtist(p_Tags, p_FI->Artist);
	if (p_FI->Title != NULL)
		MP4TagsSetName(p_Tags, p_FI->Title);

	MP4TagsSetEncodingTool(p_Tags, "libAudio 0.1.44");
	MP4TagsStore(p_Tags, p_AF->p_mp4);
	MP4TagsFree(p_Tags);

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

	if (p_AF == NULL)
		return 0;
	MP4Close(p_AF->p_mp4);
	faacEncClose(p_AF->p_enc);

	free(p_AF);
	return 0;
}
