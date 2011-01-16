#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <windows.h>
#endif
#include <string.h>

#include <FLAC/all.h>

#include "libAudio.h"
#include "libAudio_Common.h"

typedef struct _FLAC_Decoder_Context
{
	FLAC__StreamDecoder *p_dec;
	FILE *f_FLAC;
	FileInfo *fi_Info;
	BYTE buffer[16384];
	int nRead;
	Playback *p_Playback;
} FLAC_Decoder_Context;

FLAC__StreamDecoderReadStatus f_fread(const FLAC__StreamDecoder *p_dec, BYTE *Buffer, size_t *bytes, void *ClientData)
{
	FILE *f_FLAC = ((FLAC_Decoder_Context *)ClientData)->f_FLAC;

	if (*bytes > 0)
	{
		*bytes = fread(Buffer, 1, *bytes, f_FLAC);
		if (ferror(f_FLAC) > 0)
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		else if (*bytes == 0)
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		else
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}

	return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

FLAC__StreamDecoderSeekStatus f_fseek(const FLAC__StreamDecoder *p_dec, UINT64 amount, void *ClientData)
{
	FILE *f_FLAC = ((FLAC_Decoder_Context *)ClientData)->f_FLAC;

	if (f_FLAC == stdin)
		return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
	else if (fseek(f_FLAC, (long)amount, SEEK_SET) < 0)
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;

	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus f_ftell(const FLAC__StreamDecoder *p_dec, UINT64 *offset, void *ClientData)
{
	FILE *f_FLAC = ((FLAC_Decoder_Context *)ClientData)->f_FLAC;
	long pos;

	if (f_FLAC == stdin)
		return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
	else if ((pos = ftell(f_FLAC)) < 0)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;

	*offset = (UINT64)pos;
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus f_flen(const FLAC__StreamDecoder *p_dec, UINT64 *len, void *ClientData)
{
	FILE *f_FLAC = ((FLAC_Decoder_Context *)ClientData)->f_FLAC;
	struct stat m_stat;

	if (f_FLAC == stdin)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;

	if (fstat(fileno(f_FLAC), &m_stat) != 0)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;

	*len = m_stat.st_size;
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

int f_feof(const FLAC__StreamDecoder *p_dec, void *ClientData)
{
	FILE *f_FLAC = ((FLAC_Decoder_Context *)ClientData)->f_FLAC;

	return (feof(f_FLAC) == 0 ? FALSE : TRUE);
}

FLAC__StreamDecoderWriteStatus f_data(const FLAC__StreamDecoder *p_dec, const FLAC__Frame *p_frame, const int * const buffers[], void *ClientData)
{
	FLAC_Decoder_Context *p_FF = ((FLAC_Decoder_Context *)ClientData);
	short *PCM = (short *)p_FF->buffer;
	const short *Left = (const short *)buffers[0];
	const short *Right = (const short *)buffers[1];
	UINT i = 0;

	for (i = (p_FF->nRead / 2); i < p_frame->header.blocksize; i++)
	{
		PCM[(i * 2) + 0] = (short)Left[i * 2];
		PCM[(i * 2) + 1] = (short)Right[i * 2];
	}
	p_FF->nRead = p_frame->header.blocksize * 4;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void f_metadata(const FLAC__StreamDecoder *p_dec, const FLAC__StreamMetadata *p_metadata, void *ClientData)
{
	FLAC_Decoder_Context *p_FF = (FLAC_Decoder_Context *)ClientData;
	FileInfo *p_FI = p_FF->fi_Info;

	if (p_metadata->type >= FLAC__METADATA_TYPE_UNDEFINED)
		return;

	switch (p_metadata->type)
	{
		case FLAC__METADATA_TYPE_STREAMINFO:
		{
			const FLAC__StreamMetadata_StreamInfo *p_md = &p_metadata->data.stream_info;
			p_FI->Channels = p_md->channels;
			p_FI->BitRate = p_md->sample_rate;
			p_FI->BitsPerSample = p_md->bits_per_sample;
			if (ExternalPlayback == 0)
				p_FF->p_Playback = new Playback(p_FI, FLAC_FillBuffer, p_FF->buffer, 16384, ClientData);
			break;
		}
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
		{
			const FLAC__StreamMetadata_VorbisComment *p_md = &p_metadata->data.vorbis_comment;
			UINT nComment = 0;
			char **p_comments = (char **)malloc(sizeof(char *) * p_md->num_comments);

			for (UINT i = 0; i < p_md->num_comments; i++)
			{
				p_comments[i] = (char *)p_md->comments->entry;
			}

			while (p_comments[nComment] && nComment < p_md->num_comments)
			{
				if (strncasecmp(p_comments[nComment], "title=", 6) == 0)
				{
					if (p_FI->Title == NULL)
						p_FI->Title = strdup(p_comments[nComment] + 6);
					else
					{
						int nOCText = strlen(p_FI->Title);
						int nCText = strlen(p_comments[nComment] + 6);
						p_FI->Title = (char *)realloc(p_FI->Title, nOCText + nCText + 4);
						memcpy(p_FI->Title + nOCText, " / ", 3);
						memcpy(p_FI->Title + nOCText + 3, p_comments[nComment] + 6, nCText + 1);
					}
				}
				else if (strncasecmp(p_comments[nComment], "artist=", 7) == 0)
				{
					if (p_FI->Artist == NULL)
						p_FI->Artist = strdup(p_comments[nComment] + 7);
					else
					{
						int nOCText = strlen(p_FI->Artist);
						int nCText = strlen(p_comments[nComment] + 7);
						p_FI->Artist = (char *)realloc(p_FI->Artist, nOCText + nCText + 4);
						memcpy(p_FI->Artist + nOCText, " / ", 3);
						memcpy(p_FI->Artist + nOCText + 3, p_comments[nComment] + 6, nCText + 1);
					}
				}
				else if (strncasecmp(p_comments[nComment], "album=", 6) == 0)
				{
					if (p_FI->Album == NULL)
						p_FI->Album = strdup(p_comments[nComment] + 6);
					else
					{
						int nOCText = strlen(p_FI->Album);
						int nCText = strlen(p_comments[nComment] + 6);
						p_FI->Album = (char *)realloc(p_FI->Album, nOCText + nCText + 4);
						memcpy(p_FI->Album + nOCText, " / ", 3);
						memcpy(p_FI->Album + nOCText + 3, p_comments[nComment] + 6, nCText + 1);
					}
				}
				else
				{
					p_FI->OtherComments.push_back(strdup(p_comments[nComment]));
					p_FI->nOtherComments++;
				}

				nComment++;
			}

			free(p_comments);
			p_comments = 0;

			break;
		}
	}

	return;
}

void f_error(const FLAC__StreamDecoder *p_dec, FLAC__StreamDecoderErrorStatus errStat, void *ClientData)
{
}

void *FLAC_OpenR(const char *FileName)
{
	FLAC_Decoder_Context *ret = NULL;
	FLAC__StreamDecoder *p_dec = NULL;
	char Sig[4];

	FILE *f_FLAC = fopen(FileName, "rb");
	if (f_FLAC == NULL)
		return ret;

	ret = (FLAC_Decoder_Context *)malloc(sizeof(FLAC_Decoder_Context));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FLAC_Decoder_Context));

	ret->f_FLAC = f_FLAC;
	ret->p_dec = p_dec = FLAC__stream_decoder_new();

	fread(Sig, 4, 1, f_FLAC);
	fseek(f_FLAC, 0, SEEK_SET);
	if (strncmp(Sig, "OggS", 4) == 0)
		FLAC__stream_decoder_init_ogg_stream(p_dec, f_fread, f_fseek, f_ftell, f_flen, f_feof, f_data, f_metadata, f_error, ret);
	else
		FLAC__stream_decoder_init_stream(p_dec, f_fread, f_fseek, f_ftell, f_flen, f_feof, f_data, f_metadata, f_error, ret);

	FLAC__stream_decoder_set_metadata_ignore_all(p_dec);
	FLAC__stream_decoder_set_metadata_respond(p_dec, FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__stream_decoder_set_metadata_respond(p_dec, FLAC__METADATA_TYPE_VORBIS_COMMENT);

	return ret;
}

FileInfo *FLAC_GetFileInfo(void *p_FLACFile)
{
	FLAC_Decoder_Context *p_FF = (FLAC_Decoder_Context *)p_FLACFile;

	p_FF->fi_Info = (FileInfo *)malloc(sizeof(FileInfo));
	memset(p_FF->fi_Info, 0x00, sizeof(FileInfo));
	FLAC__stream_decoder_process_until_end_of_metadata(p_FF->p_dec);

	return p_FF->fi_Info;
}

int FLAC_CloseFileR(void *p_FLACFile)
{
	int ret = 0;
	FLAC_Decoder_Context *p_FF = (FLAC_Decoder_Context *)p_FLACFile;

	delete p_FF->p_Playback;

	ret = !FLAC__stream_decoder_finish(p_FF->p_dec);
	FLAC__stream_decoder_delete(p_FF->p_dec);
	fclose(p_FF->f_FLAC);
	free(p_FF);
	return ret;
}

long FLAC_FillBuffer(void *p_FLACFile, BYTE *OutBuffer, int nOutBufferLen)
{
	FLAC_Decoder_Context *p_FF = (FLAC_Decoder_Context *)p_FLACFile;
	long ret = 0;
	long readtot = 1;
	FLAC__StreamDecoderState state;

	if (p_FF->nRead <= 0)
	{
		while (readtot < nOutBufferLen && readtot != 0)
		{
			FLAC__stream_decoder_process_single(p_FF->p_dec);
			state = FLAC__stream_decoder_get_state(p_FF->p_dec);
			if (state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED)
			{
				readtot = -2;
				break;
			}
			readtot = p_FF->nRead;
		}
	}

	state = FLAC__stream_decoder_get_state(p_FF->p_dec);
	if (state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED)
	{
		return -2;
	}
	// Write back at most nOutBufferLen in data!
	if (OutBuffer != p_FF->buffer)
	{
		if (p_FF->nRead - nOutBufferLen >= 0)
			memcpy(OutBuffer, p_FF->buffer + (16384 - p_FF->nRead), nOutBufferLen);
		else
			memcpy(OutBuffer, p_FF->buffer + (16384 - p_FF->nRead), p_FF->nRead);
	}
	p_FF->nRead -= nOutBufferLen;
	ret = nOutBufferLen;

	return ret;
}

void FLAC_Play(void *p_FLACFile)
{
	FLAC_Decoder_Context *p_FF = (FLAC_Decoder_Context *)p_FLACFile;

	p_FF->p_Playback->Play();
}

bool Is_FLAC(const char *FileName)
{
	FILE *f_FLAC = fopen(FileName, "rb");
	char FLACSig[4];
	if (f_FLAC == NULL)
		return false;

	fread(FLACSig, 4, 1, f_FLAC);
	fclose(f_FLAC);

	if (strncmp(FLACSig, "fLaC", 4) != 0)
		return false;
	else
		return true;
}

API_Functions FLACDecoder =
{
	FLAC_OpenR,
	FLAC_GetFileInfo,
	FLAC_FillBuffer,
	FLAC_CloseFileR,
	FLAC_Play
};
