#include <FLAC/all.h>
#include <string.h>

#include "libAudio.h"
#include "libAudio_Common.h"

typedef struct _FLAC_Encoder_Context
{
	FLAC__StreamEncoder *p_enc;
	FLAC__StreamMetadata *p_meta[2];
	FILE *f_FLAC;
	FileInfo *p_FI;
} FLAC_Encoder_Context;

FLAC__StreamEncoderWriteStatus f_fwrite(const FLAC__StreamEncoder *p_enc, const BYTE *buffer, size_t nBytes, UINT nSamp, UINT nCurrFrame, void *p_FLACFile)
{
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;
	if (buffer == NULL)
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

	if (nBytes != 0)
		fwrite(buffer, nBytes, 1, p_FF->f_FLAC);

	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

FLAC__StreamEncoderSeekStatus f_fseek(const FLAC__StreamEncoder *p_enc, UINT64 offset, void *p_FLACFile)
{
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;

	if (fseek(p_FF->f_FLAC, (long)offset, SEEK_SET) < 0)
		return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

FLAC__StreamEncoderTellStatus f_ftell(const FLAC__StreamEncoder *p_enc, UINT64 *offset, void *p_FLACFile)
{
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;
	long off;

	if ((off = ftell(p_FF->f_FLAC)) < 0)
		return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;

	*offset = off;
	return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}

void f_fmetadata(const FLAC__StreamEncoder *p_enc, const FLAC__StreamMetadata *p_meta, void *p_FLACFile)
{
}

void *FLAC_OpenW(char *FileName)
{
	FLAC_Encoder_Context *ret = NULL;

	FILE *f_FLAC = fopen(FileName, "w+b");
	if (f_FLAC == NULL)
		return ret;

	ret = (FLAC_Encoder_Context *)malloc(sizeof(FLAC_Encoder_Context));
	if (ret == NULL)
		return ret;

	ret->f_FLAC = f_FLAC;
	ret->p_enc = FLAC__stream_encoder_new();
	FLAC__stream_encoder_set_compression_level(ret->p_enc, 4);
	//FLAC__stream_encoder_set_loose_mid_side_stereo(ret->p_enc, true);

	return ret;
}

void FLAC_SetFileInfo(void *p_FLACFile, FileInfo *p_FI)
{
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;
	FLAC__StreamMetadata_VorbisComment_Entry entry;

	FLAC__stream_encoder_set_channels(p_FF->p_enc, p_FI->Channels);
	FLAC__stream_encoder_set_bits_per_sample(p_FF->p_enc, p_FI->BitsPerSample);
	FLAC__stream_encoder_set_sample_rate(p_FF->p_enc, p_FI->BitRate);

	p_FF->p_meta[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
	p_FF->p_meta[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	for (int i = 0; i < p_FI->nOtherComments; i++)
	{
		entry.entry = (BYTE *)p_FI->OtherComments[i];
		entry.length = strlen(p_FI->OtherComments[i]);
		FLAC__metadata_object_vorbiscomment_append_comment(p_FF->p_meta[0], entry, true);
	}

	if (p_FI->Album != NULL)
	{
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "Album", p_FI->Album);
		FLAC__metadata_object_vorbiscomment_append_comment(p_FF->p_meta[0], entry, false);
	}
	if (p_FI->Artist != NULL)
	{
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "Artist", p_FI->Artist);
		FLAC__metadata_object_vorbiscomment_append_comment(p_FF->p_meta[0], entry, false);
	}
	if (p_FI->Title != NULL)
	{
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "Title", p_FI->Title);
		FLAC__metadata_object_vorbiscomment_append_comment(p_FF->p_meta[0], entry, false);
	}

	FLAC__stream_encoder_set_metadata(p_FF->p_enc, p_FF->p_meta, 2);

	FLAC__stream_encoder_init_stream(p_FF->p_enc, f_fwrite, f_fseek, f_ftell, f_fmetadata, p_FF);

	p_FF->p_FI = (FileInfo *)malloc(sizeof(FileInfo));
	memcpy(p_FF->p_FI, p_FI, sizeof(FileInfo));
}

long FLAC_WriteBuffer(void *p_FLACFile, BYTE *InBuffer, int nInBufferLen)
{
	if (nInBufferLen <= 0)
		return nInBufferLen;
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;
	int nIBL = (nInBufferLen / (p_FF->p_FI->BitsPerSample / 8)) / p_FF->p_FI->Channels;
	int *IB = (int *)malloc(sizeof(int) * (nInBufferLen / (p_FF->p_FI->BitsPerSample / 8)));

	for (int i = 0; i < nIBL * p_FF->p_FI->Channels; i++)
		IB[i] = (int)(((short *)InBuffer)[i]);

	FLAC__stream_encoder_process_interleaved(p_FF->p_enc, IB, nIBL);

	free(IB);
	return nInBufferLen;
}

int FLAC_CloseFileW(void *p_FLACFile)
{
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;
	int ret = 0;

	FLAC__stream_encoder_finish(p_FF->p_enc);
	FLAC__stream_encoder_delete(p_FF->p_enc);

	if (p_FF->p_FI != NULL)
		free(p_FF->p_FI);

	ret = fclose(p_FF->f_FLAC);
	free(p_FF);
	return ret;
}
