#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <windows.h>
#endif
#include <string.h>

#include <mad.h>
#include <id3tag.h>

#include "libAudio.h"
#include "libAudio_Common.h"

#ifndef SHRT_MAX
#define SHRT_MAX 0x7FFF
#endif

typedef struct _MP3_Intern
{
	FILE *f_MP3;
	mad_stream *p_Stream;
	mad_frame *p_Frame;
	mad_synth *p_Synth;
	//mad_timer_t *p_Timer;
	BYTE buffer[8192];
	BYTE inbuff[16392];
	FileInfo *p_FI;
	bool InitialFrame;
	int PCMDecoded;
	char *f_name;
	Playback *p_Playback;
	bool eof;
} MP3_Intern;

short FixedToShort(mad_fixed_t Fixed)
{
	if (Fixed >= MAD_F_ONE)
		return SHRT_MAX;
	if (Fixed <= -MAD_F_ONE)
		return -SHRT_MAX;

	Fixed = Fixed >> (MAD_F_FRACBITS - 15);
	return (signed short)Fixed;
}

void *MP3_OpenR(char *FileName)
{
	MP3_Intern *ret = NULL;
	FILE *f_MP3 = NULL;

	f_MP3 = fopen(FileName, "rb");
	if (f_MP3 == NULL)
		return ret;

	ret = (MP3_Intern *)malloc(sizeof(MP3_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(MP3_Intern));

	ret->f_MP3 = f_MP3;
	ret->f_name = strdup(FileName);
	ret->p_Stream = (mad_stream *)malloc(sizeof(mad_stream));
	ret->p_Frame = (mad_frame *)malloc(sizeof(mad_frame));
	ret->p_Synth = (mad_synth *)malloc(sizeof(mad_synth));
	//ret->p_Timer = (mad_timer_t *)malloc(sizeof(mad_timer_t));

	mad_stream_init(ret->p_Stream);
	mad_frame_init(ret->p_Frame);
	mad_synth_init(ret->p_Synth);
	//mad_timer_reset(ret->p_Timer);

	return ret;
}

FileInfo *MP3_GetFileInfo(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	FileInfo *ret = NULL;
	int rem = 0;
	id3_file *f_id3 = NULL;
	id3_tag *id_tag = NULL;
	id3_frame *id_frame = NULL;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	f_id3 = id3_file_open(/*fileno(p_MF->f_MP3)*/p_MF->f_name, ID3_FILE_MODE_READONLY);
	id_tag = id3_file_tag(f_id3);
#ifdef _WINDOWS
	id_frame = id3_tag_findframe(id_tag, "TLEN", 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = NULL;
		UINT nStrings = 0;
		UCHAR *Time = NULL;

		id_field = id3_frame_field(id_frame, 1);
		nStrings = id3_field_getnstrings(id_field);

		if (nStrings > 0)
		{
			Time = id3_ucs4_latin1duplicate(id3_field_getstrings(id_field, 0));

			if (Time != NULL)
			{
				ret->TotalTime = (double)atol((const char *)Time);
				free(Time);
				Time = NULL;
			}
		}
	}
#endif

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_ALBUM, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = NULL;
		UINT nStrings = 0;
		UCHAR *Album = NULL;

		id_field = id3_frame_field(id_frame, 1);
		nStrings = id3_field_getnstrings(id_field);

		if (nStrings > 0)
		{
			UINT i = 0;
			for (i = 0; i < nStrings; i++)
			{
				Album = id3_ucs4_latin1duplicate(id3_field_getstrings(id_field, 0));

				if (Album != NULL)
				{
					if (ret->Album == NULL)
					{
						ret->Album = (char *)malloc(strlen((char *)Album) + 1);
						memset(ret->Album, 0x00, strlen((char *)Album) + 1);
					}
					else
					{
						ret->Album = (char *)realloc(ret->Album, strlen(ret->Album) + strlen((char *)Album) + 4);
						memcpy(ret->Album + strlen(ret->Album), " / ", 4);
					}
					memcpy(ret->Album + strlen(ret->Album), Album, strlen((char *)Album) + 1);
					free(Album);
					Album = NULL;
				}
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_ARTIST, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = NULL;
		UINT nStrings = 0;
		UCHAR *Artist = NULL;

		id_field = id3_frame_field(id_frame, 1);
		nStrings = id3_field_getnstrings(id_field);

		if (nStrings > 0)
		{
			UINT i = 0;
			for (i = 0; i < nStrings; i++)
			{
				Artist = id3_ucs4_latin1duplicate(id3_field_getstrings(id_field, 0));

				if (Artist != NULL)
				{
					if (ret->Artist == NULL)
					{
						ret->Artist = (char *)malloc(strlen((char *)Artist) + 1);
						memset(ret->Artist, 0x00, strlen((char *)Artist) + 1);
					}
					else
					{
						ret->Artist = (char *)realloc(ret->Artist, strlen(ret->Artist) + strlen((char *)Artist) + 4);
						memcpy(ret->Artist + strlen(ret->Artist), " / ", 4);
					}
					memcpy(ret->Artist + strlen(ret->Artist), Artist, strlen((char *)Artist) + 1);
					free(Artist);
					Artist = NULL;
				}
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_TITLE, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = NULL;
		UINT nStrings = 0;
		UCHAR *Title = NULL;

		id_field = id3_frame_field(id_frame, 1);
		nStrings = id3_field_getnstrings(id_field);

		if (nStrings > 0)
		{
			UINT i = 0;
			for (i = 0; i < nStrings; i++)
			{
				Title = id3_ucs4_latin1duplicate(id3_field_getstrings(id_field, 0));

				if (Title != NULL)
				{
					if (ret->Title == NULL)
					{
						ret->Title = (char *)malloc(strlen((char *)Title) + 1);
						memset(ret->Title, 0x00, strlen((char *)Title) + 1);
					}
					else
					{
						ret->Title = (char *)realloc(ret->Title, strlen(ret->Title) + strlen((char *)Title) + 4);
						memcpy(ret->Title + strlen(ret->Title), " / ", 4);
					}
					memcpy(ret->Title + strlen(ret->Title), Title, strlen((char *)Title) + 1);
					free(Title);
					Title = NULL;
				}
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_COMMENT, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = NULL;
		UINT nStrings = 0;
		UCHAR *Comment = NULL;

		id_field = id3_frame_field(id_frame, 1);
		nStrings = id3_field_getnstrings(id_field);

		if (nStrings > 0)
		{
			UINT i = 0;
			for (i = 0; i < nStrings; i++)
			{
				Comment = id3_ucs4_latin1duplicate(id3_field_getstrings(id_field, 0));

				if (Comment != NULL)
				{
					if (ret->OtherComments.size() == 0)
						ret->nOtherComments = 0;

					ret->OtherComments.push_back(strdup((char *)Comment));
					free(Comment);
					Comment = NULL;
					ret->nOtherComments++;
				}
			}
		}
	}

	fseek(p_MF->f_MP3, id_tag->paddedsize, SEEK_SET);
	id3_file_close(f_id3);

	rem = (p_MF->p_Stream->buffer == NULL ? 0 : p_MF->p_Stream->bufend - p_MF->p_Stream->next_frame);
	fread(p_MF->inbuff + rem, 2 * 8192 - rem, 1, p_MF->f_MP3);
	mad_stream_buffer(p_MF->p_Stream, (const BYTE *)p_MF->inbuff, (2 * 8192));

	while (p_MF->p_Frame->header.bitrate == 0 || p_MF->p_Frame->header.samplerate == 0)
		mad_frame_decode(p_MF->p_Frame, p_MF->p_Stream);
	p_MF->InitialFrame = true;
	ret->BitRate = p_MF->p_Frame->header.samplerate;
	ret->BitsPerSample = 16;
	ret->Channels = (p_MF->p_Frame->header.mode == MAD_MODE_SINGLE_CHANNEL ? 1 : 2);

	if (ExternalPlayback == 0)
		p_MF->p_Playback = new Playback(ret, MP3_FillBuffer, p_MF->buffer, 8192, p_MP3File);
	p_MF->p_FI = ret;

	return ret;
}

int MP3_CloseFileR(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	int ret;

	delete p_MF->p_Playback;

	mad_synth_finish(p_MF->p_Synth);
	mad_frame_finish(p_MF->p_Frame);
	mad_stream_finish(p_MF->p_Stream);
	//mad_timer_reset(p_MF->p_Timer);

	free(p_MF->f_name);
	ret = fclose(p_MF->f_MP3);
	free(p_MF);
	return ret;
}

void GetData(MP3_Intern *p_MF, bool *eof)
{
	int rem = (p_MF->p_Stream->buffer == NULL ? 0 : p_MF->p_Stream->bufend - p_MF->p_Stream->next_frame);

	if (p_MF->p_Stream->next_frame != NULL)
	{
		memmove(p_MF->inbuff, p_MF->p_Stream->next_frame, rem);
	}

	fread(p_MF->inbuff + rem, (2 * 8192) - rem, 1, p_MF->f_MP3);
	*eof = (feof(p_MF->f_MP3) == FALSE ? false : true);

	if (*eof == true)
		memset(p_MF->inbuff + (2 * 8192), 0x00, 8);

	mad_stream_buffer(p_MF->p_Stream, (const BYTE *)p_MF->inbuff, (2 * 8192));
}

int DecodeFrame(MP3_Intern *p_MF, bool *eof)
{
	if (p_MF->InitialFrame == false)
	{
		if (mad_frame_decode(p_MF->p_Frame, p_MF->p_Stream) != 0)
		{
			if (MAD_RECOVERABLE(p_MF->p_Stream->error) == 0)
			{
				if (p_MF->p_Stream->error == MAD_ERROR_BUFLEN)
				{
					GetData(p_MF, eof);
					if (*eof == true)
						return -2;
					return DecodeFrame(p_MF, eof);
				}
				else
				{
					printf("Unrecoverable frame level error (%i).\n", p_MF->p_Stream->error);
					return -1;
				}
			}
		}
	}

	return 0;
}

long MP3_FillBuffer(void *p_MP3File, BYTE *OutBuffer, int nOutBufferLen)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	BYTE *OBuff = OutBuffer;

	while ((OBuff - OutBuffer) < nOutBufferLen && p_MF->eof == false)
	{
		short *out = (short *)(p_MF->buffer + (OBuff - OutBuffer));
		int nOut = 0;

		if (p_MF->PCMDecoded == 0)
		{
			int ret;
			// Get input if needed, get the stream buffer part of libMAD to process that input.
			if (p_MF->p_Stream->buffer == NULL || p_MF->p_Stream->error == MAD_ERROR_BUFLEN)
				GetData(p_MF, &p_MF->eof);

			// Decode a frame:
			if ((ret = DecodeFrame(p_MF, &p_MF->eof)) != 0)
				return ret;

			if (p_MF->InitialFrame == true)
				p_MF->InitialFrame = false;

			// Synth the PCM
			mad_synth_frame(p_MF->p_Synth, p_MF->p_Frame);
		}

		// copy the PCM to our output buffer
		for (int i = p_MF->PCMDecoded; i < p_MF->p_Synth->pcm.length; i++)
		{
			short Sample = 0;

			Sample = FixedToShort(p_MF->p_Synth->pcm.samples[0][i]);
			out[((i - p_MF->PCMDecoded) * p_MF->p_FI->Channels) + 0] = Sample;

			if (p_MF->p_FI->Channels == 2)
			{
				Sample = FixedToShort(p_MF->p_Synth->pcm.samples[1][i]);
				out[((i - p_MF->PCMDecoded) * p_MF->p_FI->Channels) + 1] = Sample;
			}

			nOut += sizeof(short) * p_MF->p_FI->Channels;

			if (((OBuff - OutBuffer) + nOut) >= nOutBufferLen)
			{
				p_MF->PCMDecoded = i + 1;
				i = p_MF->p_Synth->pcm.length;
			}
		}

		if (((OBuff - OutBuffer) + nOut) < nOutBufferLen)
		{
			p_MF->PCMDecoded = 0;
		}

		memcpy(OBuff, out, nOut);
		OBuff += nOut;
	}

	return OBuff - OutBuffer;
}

void MP3_Play(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;

	p_MF->p_Playback->Play();
}

bool Is_MP3(char *FileName)
{
	FILE *f_MP3 = fopen(FileName, "rb");
	char ID3[3];
	char MP3Sig[2];

	if (f_MP3 == NULL)
		return false;

	fread(ID3, 3, 1, f_MP3);
	fseek(f_MP3, 0, SEEK_SET);
	fread(MP3Sig, 2, 1, f_MP3);
	fclose(f_MP3);

	if (strncmp(ID3, "ID3", 3) == 0)
		return true;
	if ((((((short)MP3Sig[0]) << 8) | MP3Sig[1]) & 0xFFFA) == 0xFFFA)
		return true;

	return false;
}
