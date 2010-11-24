#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>
#include <string.h>

#include "libAudio.h"
#include "libAudio_Common.h"

#include <stdlib.h>
#include <time.h>

typedef struct _OV_Intern
{
	vorbis_info *vi;
	vorbis_dsp_state *vds;
	vorbis_block *vb;
	vorbis_comment *vc;
	ogg_packet *opt;
	ogg_page *ope;
	ogg_stream_state *oss;
	FILE *f_Ogg;
	FileInfo *p_FI;
} OV_Intern;

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
