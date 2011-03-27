#include <FLAC/all.h>

#include "libAudio.h"
#include "libAudio_Common.h"

/*!
 * @internal
 * @file saveFLAC.cpp
 * The implementation of the FLAC encoder API
 * @author Richard Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2011
 */

/*!
 * @internal
 * Internal structure for holding the encoding context for a given FLAC file
 */
typedef struct _FLAC_Encoder_Context
{
	/*!
	 * @internal
	 * The encoder context itself
	 */
	FLAC__StreamEncoder *p_enc;
	/*!
	 * @internal
	 * The metadata context
	 */
	FLAC__StreamMetadata *p_meta[2];
	/*!
	 * @internal
	 * The file being written to
	 */
	FILE *f_FLAC;
	/*!
	 * @internal
	 * The input metadata in the form of a \c FileInfo structure
	 */
	FileInfo *p_FI;
} FLAC_Encoder_Context;

/*!
 * @internal
 * f_fwrite() is the internal write callback for FLAC file creation. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_enc The encoding context which must not be modified by the function
 * @param buffer The buffer to write which also must not become modified
 * @param nBytes A count holding the number of bytes in \p buffer to write
 * @param nSamp A count holding the number of samples encoded in buffer,
 *   or 0 to indicate metadata is being written
 * @param nCurrFrame If \p nSamp is non-zero, this olds the number of the current
 *   frame being encoded
 * @param p_FLACFile Our own internal context pointer which holds the file to write to
 */
FLAC__StreamEncoderWriteStatus f_fwrite(const FLAC__StreamEncoder *p_enc, const BYTE *buffer, size_t nBytes, UINT nSamp, UINT nCurrFrame, void *p_FLACFile)
{
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;
	if (buffer == NULL)
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

	if (nBytes != 0)
		fwrite(buffer, nBytes, 1, p_FF->f_FLAC);

	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

/*!
 * @internal
 * f_fseek() is the internal seek callback for FLAC file creation. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_enc The encoding context which must not be modified by the function
 * @param offset The offset through the file to which to seek to
 * @param p_FLACFile Our own internal context pointer which holds the file to seek through
 */
FLAC__StreamEncoderSeekStatus f_fseek(const FLAC__StreamEncoder *p_enc, UINT64 offset, void *p_FLACFile)
{
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;

	if (fseek(p_FF->f_FLAC, (long)offset, SEEK_SET) < 0)
		return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

/*!
 * @internal
 * f_ftell() is the internal seek callback for FLAC file creation. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_enc The encoding context which must not be modified by the function
 * @param offset The returned offset location into the file
 * @param p_FLACFile Our own internal context pointer which holds the file to get
 *   the write pointer position of
 */
FLAC__StreamEncoderTellStatus f_ftell(const FLAC__StreamEncoder *p_enc, UINT64 *offset, void *p_FLACFile)
{
	FLAC_Encoder_Context *p_FF = (FLAC_Encoder_Context *)p_FLACFile;
	long off;

	if ((off = ftell(p_FF->f_FLAC)) < 0)
		return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;

	*offset = off;
	return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}

/*!
 * @internal
 * f_fmetadata() is the internal seek callback for FLAC file creation. This prevents
 * nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_enc The encoding context which must not be modified by the function
 * @param p_meta 
 * @param p_FLACFile Our own internal context pointer
 */
void f_fmetadata(const FLAC__StreamEncoder *p_enc, const FLAC__StreamMetadata *p_meta, void *p_FLACFile)
{
}

/*!
 * This function opens the file given by \c FileName for writing and returns a pointer
 * to the context of the opened file which must be used only by FLAC_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *FLAC_OpenW(const char *FileName)
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

/*!
 * This function sets the \c FileInfo structure for a FLAC file being encoded
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c FLAC_WriteBuffer()
 */
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

/*!
 * This function writes a buffer of audio to a FLAC file opened being encoded
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @attention Will not work unless \c FLAC_SetFileInfo() has been called beforehand
 */
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

/*!
 * Closes an open FLAC file
 * @param p_FLACFile A pointer to a file opened with \c FLAC_OpenW()
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_AudioPtr after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 */
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
