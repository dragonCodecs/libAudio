#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <string.h>

#include "libAudio.h"
#include "libAudio_Common.h"

#ifdef _WINDOWS
#ifdef __CDECL__
#undef __CDECL__
#endif
#define __CDECL__ __cdecl
#else
#ifndef __CDECL__
#define __CDECL__
#endif
#endif

typedef struct _OggVorbis_Intern
{
	OggVorbis_File ovf;
	BYTE buffer[8192];
	Playback *p_Playback;
} OggVorbis_Intern;

void *OggVorbis_OpenR(const char *FileName)
{
	OggVorbis_Intern *ret = NULL;
	ov_callbacks callbacks;

	FILE *f_Ogg = fopen(FileName, "rb");
	if (f_Ogg == NULL)
		return ret;

	ret = (OggVorbis_Intern *)malloc(sizeof(OggVorbis_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(OggVorbis_Intern));

	callbacks.close_func = (int (__CDECL__ *)(void *))fclose;
	callbacks.read_func = (size_t (__CDECL__ *)(void *, size_t, size_t, void *))fread;
	callbacks.seek_func = fseek_wrapper;
	callbacks.tell_func = (long (__CDECL__ *)(void *))ftell;
	ov_open_callbacks(f_Ogg, (OggVorbis_File *)ret, NULL, 0, callbacks);

	return ret;
}

FileInfo *OggVorbis_GetFileInfo(void *p_VorbisFile)
{
	OggVorbis_Intern *p_VF = (OggVorbis_Intern *)p_VorbisFile;
	FileInfo *ret = NULL;
	vorbis_comment *comments = NULL;
	char **p_comments = NULL;
	int nComment = 0;
	vorbis_info *info = NULL;
	OggVorbis_File *p_OVFile = (OggVorbis_File *)p_VorbisFile;
	if (p_OVFile == NULL)
		return ret;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	info = ov_info(p_OVFile, -1);
	comments = ov_comment(p_OVFile, -1);
	p_comments = comments->user_comments;

	ret->BitRate = info->rate;
	ret->Channels = info->channels;
	ret->BitsPerSample = 16;

	while (p_comments[nComment] && nComment < comments->comments)
	{
		if (strncasecmp(p_comments[nComment], "title=", 6) == 0)
		{
			if (ret->Title == NULL)
				ret->Title = strdup(p_comments[nComment] + 6);
			else
			{
				int nOCText = strlen(ret->Title);
				int nCText = strlen(p_comments[nComment] + 6);
				ret->Title = (char *)realloc(ret->Title, nOCText + nCText + 4);
				memcpy(ret->Title + nOCText, " / ", 3);
				memcpy(ret->Title + nOCText + 3, p_comments[nComment] + 6, nCText + 1);
			}
		}
		else if (strncasecmp(p_comments[nComment], "artist=", 7) == 0)
		{
			if (ret->Artist == NULL)
				ret->Artist = strdup(p_comments[nComment] + 7);
			else
			{
				int nOCText = strlen(ret->Artist);
				int nCText = strlen(p_comments[nComment] + 7);
				ret->Artist = (char *)realloc(ret->Artist, nOCText + nCText + 4);
				memcpy(ret->Artist + nOCText, " / ", 3);
				memcpy(ret->Artist + nOCText + 3, p_comments[nComment] + 6, nCText + 1);
			}
		}
		else if (strncasecmp(p_comments[nComment], "album=", 6) == 0)
		{
			if (ret->Album == NULL)
				ret->Album = strdup(p_comments[nComment] + 6);
			else
			{
				int nOCText = strlen(ret->Album);
				int nCText = strlen(p_comments[nComment] + 6);
				ret->Album = (char *)realloc(ret->Album, nOCText + nCText + 4);
				memcpy(ret->Album + nOCText, " / ", 3);
				memcpy(ret->Album + nOCText + 3, p_comments[nComment] + 6, nCText + 1);
			}
		}
		else
		{
			ret->OtherComments.push_back(strdup(p_comments[nComment]));
			ret->nOtherComments++;
		}
		nComment++;
	}

	if (ExternalPlayback == 0)
		p_VF->p_Playback = new Playback(ret, OggVorbis_FillBuffer, p_VF->buffer, 8192, p_VorbisFile);

	return ret;
}

long OggVorbis_FillBuffer(void *p_VorbisFile, BYTE *OutBuffer, int nOutBufferLen)
{
	int bitstream;
	OggVorbis_File *p_OVFile = (OggVorbis_File *)p_VorbisFile;
	bitstream;
	long ret = 0;
	long readtot = 1;

	while (readtot != OV_HOLE && readtot != OV_EBADLINK && ret < nOutBufferLen && readtot != 0)
	{
		readtot = ov_read(p_OVFile, (char *)OutBuffer + ret, nOutBufferLen - ret, 0, 2, 1, &bitstream);
		if (0 < readtot)
			ret += readtot;
	}

	return ret;
}

int OggVorbis_CloseFileR(void *p_VorbisFile)
{
	int ret = 0;
	OggVorbis_Intern *p_VF = (OggVorbis_Intern *)p_VorbisFile;

	delete p_VF->p_Playback;

	ret = ov_clear((OggVorbis_File *)p_VF);
	free(p_VorbisFile);
	return ret;
}

void OggVorbis_Play(void *p_VorbisFile)
{
	((OggVorbis_Intern *)p_VorbisFile)->p_Playback->Play();
}

bool Is_OggVorbis(const char *FileName)
{
	FILE *f_Ogg = fopen(FileName, "rb");
	char OggSig[4];
	char VorbisSig[6];
	if (f_Ogg == NULL)
		return false;

	fread(OggSig, 4, 1, f_Ogg);
	fseek(f_Ogg, 25, SEEK_CUR);
	fread(VorbisSig, 6, 1, f_Ogg);
	fclose(f_Ogg);

	if (strncmp(OggSig, "OggS", 4) != 0 ||
		strncmp(VorbisSig, "vorbis", 6) != 0)
		return false;
	else
		return true;
}
