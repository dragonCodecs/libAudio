#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>

#include "libAudio.h"
#include "libAudio_Common.h"

#include <stdlib.h>
#include <time.h>

/*!
 * @internal
 * @file saveOggVorbis.cpp
 * The implementation of the Ogg/Vorbis encoder API
 * @author Richard Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2011
 */

/*!
 * @internal
 * Internal structure for holding the encoding context for a given Ogg/Vorbis file
 */
typedef struct _OV_Intern
{
	/*!
	 * @internal
	 * Sturcture describing info about the Vorbis stream being encoded
	 */
	vorbis_info *vi;
	/*!
	 * @internal
	 * The Vorbis Digital Signal Processing state
	 */
	vorbis_dsp_state *vds;
	/*!
	 * @internal
	 * The Vorbis encoding context
	 */
	vorbis_block *vb;
	/*!
	 * @internal
	 * Structure describing the Vorbis Comments present
	 */
	vorbis_comment *vc;
	/*!
	 * @internal
	 * The Ogg Packet context
	 */
	ogg_packet *opt;
	/*!
	 * @internal
	 * The Ogg Page context
	 */
	ogg_page *ope;
	/*!
	 * @internal
	 * The Ogg Stream state
	 */
	ogg_stream_state *oss;
	/*!
	 * @internal
	 * The Ogg file being written to
	 */
	FILE *f_Ogg;
	/*!
	 * @internal
	 * The input metadata in the form of a \c FileInfo structure
	 */
	FileInfo *p_FI;
} OV_Intern;

/*!
 * This function opens the file given by \c FileName for writing and returns a pointer
 * to the context of the opened file which must be used only by OggVorbis_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *OggVorbis_OpenW(const char *FileName)
{
	OV_Intern *ret = NULL;

	FILE *f_Ogg = fopen(FileName, "wb");
	if (f_Ogg == NULL)
		return ret;

	ret = (OV_Intern *)malloc(sizeof(OV_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(OV_Intern));

	ret->vi = (vorbis_info *)malloc(sizeof(vorbis_info));
	ret->vds = (vorbis_dsp_state *)malloc(sizeof(vorbis_dsp_state));
	ret->vb = (vorbis_block *)malloc(sizeof(vorbis_block));
	ret->vc = (vorbis_comment *)malloc(sizeof(vorbis_comment));
	ret->opt = (ogg_packet *)malloc(sizeof(ogg_packet));
	ret->ope = (ogg_page *)malloc(sizeof(ogg_page));
	ret->oss = (ogg_stream_state *)malloc(sizeof(ogg_stream_state));
	ret->f_Ogg = f_Ogg;

	vorbis_info_init(ret->vi);
	vorbis_comment_init(ret->vc);

	ogg_packet_clear(ret->opt);

	return ret;
}

/*!
 * This function sets the \c FileInfo structure for a Ogg/Vorbis file being encoded
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file
 * @warning This function must be called before using \c OggVorbis_WriteBuffer()
 * @bug p_FI must not be \c NULL as no checking on the parameter is done. FIXME!
 */
void OggVorbis_SetFileInfo(void *p_VorbisFile, FileInfo *p_FI)
{
	OV_Intern *p_VF = (OV_Intern *)p_VorbisFile;
	ogg_packet hdr, hdr_comm, hdr_code;

	vorbis_encode_init(p_VF->vi, p_FI->Channels, p_FI->BitRate, -1, 160000, -1);
	vorbis_encode_ctl(p_VF->vi, OV_ECTL_RATEMANAGE2_SET, NULL);
	vorbis_encode_setup_init(p_VF->vi);

	vorbis_analysis_init(p_VF->vds, p_VF->vi);
	vorbis_block_init(p_VF->vds, p_VF->vb);

	srand((DWORD)time(NULL));
	ogg_stream_init(p_VF->oss, rand());

	if (p_FI->Title != NULL)
		vorbis_comment_add_tag(p_VF->vc, "Title", p_FI->Title);
	if (p_FI->Album != NULL)
		vorbis_comment_add_tag(p_VF->vc, "Album", p_FI->Album);
	if (p_FI->Artist != NULL)
		vorbis_comment_add_tag(p_VF->vc, "Artist", p_FI->Artist);

	for (int i = 0; i < p_FI->nOtherComments; i++)
		vorbis_comment_add(p_VF->vc, p_FI->OtherComments[i]);

	vorbis_analysis_headerout(p_VF->vds, p_VF->vc, &hdr, &hdr_comm, &hdr_code);
	ogg_stream_packetin(p_VF->oss, &hdr);
	ogg_stream_packetin(p_VF->oss, &hdr_comm);
	ogg_stream_packetin(p_VF->oss, &hdr_code);

	while (1)
	{
		int res = ogg_stream_flush(p_VF->oss, p_VF->ope);
		if (res == 0)
			break;
		fwrite(p_VF->ope->header, p_VF->ope->header_len, 1, p_VF->f_Ogg);
		fwrite(p_VF->ope->body, p_VF->ope->body_len, 1, p_VF->f_Ogg);
	}

	p_VF->p_FI = (FileInfo *)malloc(sizeof(FileInfo));
	memcpy(p_VF->p_FI, p_FI, sizeof(FileInfo));
}

/*!
 * This function writes a buffer of audio to a Ogg/Vorbis file opened being encoded
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @attention Will not work unless \c OggVorbis_SetFileInfo() has been called beforehand
 */
long OggVorbis_WriteBuffer(void *p_VorbisFile, BYTE *InBuffer, int nInBufferLen)
{
	OV_Intern *p_VF = (OV_Intern *)p_VorbisFile;
	bool eos = false;

	if (nInBufferLen <= 0)
	{
		vorbis_analysis_buffer(p_VF->vds, 0);
		vorbis_analysis_wrote(p_VF->vds, 0);
	}
	else
	{
		int bufflen = (nInBufferLen / 2) / p_VF->p_FI->Channels;
		float **buff = vorbis_analysis_buffer(p_VF->vds, bufflen);
		short *IB = (short *)InBuffer;

		for (int i = 0; i < bufflen; i++)
		{
			for (int j = 0; j < p_VF->p_FI->Channels; j++)
			{
				buff[j][i] = ((float)IB[i * p_VF->p_FI->Channels + j]) / 32768.0F;
			}
		}

		vorbis_analysis_wrote(p_VF->vds, bufflen);
	}

	while (vorbis_analysis_blockout(p_VF->vds, p_VF->vb) == 1)
	{
		vorbis_analysis(p_VF->vb, NULL);
		vorbis_bitrate_addblock(p_VF->vb);

		while (vorbis_bitrate_flushpacket(p_VF->vds, p_VF->opt) == 1)
		{
			ogg_stream_packetin(p_VF->oss, p_VF->opt);
			if (p_VF->opt->bytes == 1)
				continue; // Let's do this for speed, quicker than running the next while loop to find out!

			while (eos == false)
			{
				int res = ogg_stream_pageout(p_VF->oss, p_VF->ope);
				if (res == 0)
					break; // Nothing to write?
				fwrite(p_VF->ope->header, p_VF->ope->header_len, 1, p_VF->f_Ogg);
				fwrite(p_VF->ope->body, p_VF->ope->body_len, 1, p_VF->f_Ogg);

				if (ogg_page_eos(p_VF->ope) == 1)
					eos = true; // Handle End Of Stream.
			}
		}
	}

	if (eos == false)
		return nInBufferLen;
	else
	{
		// Empty out buffers so that all the file has been writen.
		while (1)
		{
			int res = ogg_stream_flush(p_VF->oss, p_VF->ope);
			if (res == 0)
				break;
			fwrite(p_VF->ope->header, p_VF->ope->header_len, 1, p_VF->f_Ogg);
			fwrite(p_VF->ope->body, p_VF->ope->body_len, 1, p_VF->f_Ogg);
		}
		return -2;
	}
}

/*!
 * Closes an open Ogg/Vorbis file
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenW()
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_VorbisFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 */
int OggVorbis_CloseFileW(void *p_VorbisFile)
{
	OV_Intern *p_VF = (OV_Intern *)p_VorbisFile;
	int ret = 0;

	if (p_VF->vc != NULL)
	{
		vorbis_comment_clear(p_VF->vc);
		free(p_VF->vc);
	}
	if (p_VF->vb != NULL)
	{
		vorbis_block_clear(p_VF->vb);
		free(p_VF->vb);
	}
	if (p_VF->vds != NULL)
	{
		vorbis_dsp_clear(p_VF->vds);
		free(p_VF->vds);
	}
	if (p_VF->vi != NULL)
	{
		vorbis_info_clear(p_VF->vi);
		free(p_VF->vi);
	}

	if (p_VF->opt != NULL)
	{
		ogg_packet_clear(p_VF->opt);
		free(p_VF->opt);
	}
	if (p_VF->ope != NULL)
		free(p_VF->ope);
	if (p_VF->oss != NULL)
	{
		ogg_stream_clear(p_VF->oss);
		free(p_VF->oss);
	}

	ret = fclose(p_VF->f_Ogg);
	free(p_VF->p_FI);
	free(p_VF);
	return ret;
}
