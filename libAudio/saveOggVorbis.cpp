#include <stdlib.h>
#include <time.h>

#include "oggVorbis.hxx"

/*!
 * @internal
 * @file saveOggVorbis.cpp
 * @brief The implementation of the Ogg/Vorbis encoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2019
 */

/*!
 * @internal
 * Internal structure for holding the encoding context for a given Ogg/Vorbis file
 */
typedef struct _OV_Intern
{
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

	oggVorbis_t inner;

	_OV_Intern(const char *const fileName) noexcept :
		inner(fd_t(fileName, O_RDWR | O_CREAT | O_TRUNC, normalMode), audioModeWrite_t{}) { }
} OV_Intern;

oggVorbis_t::oggVorbis_t(fd_t &&fd, audioModeWrite_t) noexcept :
	audioFile_t(audioType_t::oggVorbis, std::move(fd)), encoderCtx(makeUnique<encoderContext_t>()) { }
oggVorbis_t::encoderContext_t::encoderContext_t() noexcept : vorbisInfo{}
{
	vorbis_info_init(&vorbisInfo);
}

/*!
 * This function opens the file given by \c FileName for writing and returns a pointer
 * to the context of the opened file which must be used only by OggVorbis_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *OggVorbis_OpenW(const char *FileName)
{
	std::unique_ptr<OV_Intern> ret = makeUnique<OV_Intern>(FileName);
	if (!ret || !ret->inner.encoderContext())
		return nullptr;

	ret->vds = (vorbis_dsp_state *)malloc(sizeof(vorbis_dsp_state));
	ret->vb = (vorbis_block *)malloc(sizeof(vorbis_block));
	ret->vc = (vorbis_comment *)malloc(sizeof(vorbis_comment));
	ret->opt = (ogg_packet *)malloc(sizeof(ogg_packet));
	ret->ope = (ogg_page *)malloc(sizeof(ogg_page));
	ret->oss = (ogg_stream_state *)malloc(sizeof(ogg_stream_state));

	vorbis_comment_init(ret->vc);

	return ret.release();
}

/*!
 * This function sets the \c FileInfo structure for a Ogg/Vorbis file being encoded
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file
 * @warning This function must be called before using \c OggVorbis_WriteBuffer()
 * @bug p_FI must not be \c NULL as no checking on the parameter is done. FIXME!
 *
 * @bug \p p_VorbisFile must not be \c NULL as no checking on the parameter is done. FIXME!
 */
bool OggVorbis_SetFileInfo(void *p_VorbisFile, FileInfo *p_FI)
{
	OV_Intern *p_VF = (OV_Intern *)p_VorbisFile;
	auto &ctx = *p_VF->inner.encoderContext();
	fileInfo_t &info = p_VF->inner.fileInfo();
	const fd_t &fd = p_VF->inner.fd();
	vorbis_info &vorbisInfo = ctx.vorbisInfo;
	ogg_packet hdr, hdr_comm, hdr_code;

	vorbis_encode_init(&vorbisInfo, p_FI->Channels, p_FI->BitRate, -1, 160000, -1);
	vorbis_encode_ctl(&vorbisInfo, OV_ECTL_RATEMANAGE2_SET, NULL);
	vorbis_encode_setup_init(&vorbisInfo);

	vorbis_analysis_init(p_VF->vds, &vorbisInfo);
	vorbis_block_init(p_VF->vds, p_VF->vb);

	srand((uint32_t)time(NULL));
	ogg_stream_init(p_VF->oss, rand());

	if (p_FI->Title != NULL)
		vorbis_comment_add_tag(p_VF->vc, "Title", p_FI->Title);
	if (p_FI->Album != NULL)
		vorbis_comment_add_tag(p_VF->vc, "Album", p_FI->Album);
	if (p_FI->Artist != NULL)
		vorbis_comment_add_tag(p_VF->vc, "Artist", p_FI->Artist);

	for (uint32_t i = 0; i < p_FI->nOtherComments; i++)
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
		else if (fd.write(p_VF->ope->header, p_VF->ope->header_len) != p_VF->ope->header_len ||
			fd.write(p_VF->ope->body, p_VF->ope->body_len) != p_VF->ope->body_len)
			return false;
	}

	info.totalTime = p_FI->TotalTime;
	info.bitsPerSample = p_FI->BitsPerSample;
	info.bitRate = p_FI->BitRate;
	info.channels = p_FI->Channels;
	return true;
}

/*!
 * This function writes a buffer of audio to a Ogg/Vorbis file opened being encoded
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @attention Will not work unless \c OggVorbis_SetFileInfo() has been called beforehand
 */
long OggVorbis_WriteBuffer(void *p_VorbisFile, uint8_t *InBuffer, int nInBufferLen)
{
	OV_Intern *p_VF = (OV_Intern *)p_VorbisFile;
	fileInfo_t &info = p_VF->inner.fileInfo();
	const fd_t &fd = p_VF->inner.fd();
	bool eos = false;

	if (nInBufferLen <= 0)
	{
		vorbis_analysis_buffer(p_VF->vds, 0);
		vorbis_analysis_wrote(p_VF->vds, 0);
	}
	else
	{
		uint32_t bufflen = (nInBufferLen / 2) / info.channels;
		float **buff = vorbis_analysis_buffer(p_VF->vds, bufflen);
		short *IB = (short *)InBuffer;

		for (uint32_t i = 0; i < bufflen; i++)
		{
			for (uint8_t j = 0; j < info.channels; j++)
				buff[j][i] = ((float)IB[i * info.channels + j]) / 32768.0F;
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
				else if (fd.write(p_VF->ope->header, p_VF->ope->header_len) != p_VF->ope->header_len ||
					fd.write(p_VF->ope->body, p_VF->ope->body_len) != p_VF->ope->body_len)
					return -2;

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
			else if (fd.write(p_VF->ope->header, p_VF->ope->header_len) != p_VF->ope->header_len ||
				fd.write(p_VF->ope->body, p_VF->ope->body_len) != p_VF->ope->body_len)
				return -2;
		}
		return -2;
	}
}

int64_t oggVorbis_t::writeBuffer(const void *const buffer, const uint32_t length) { return -1; }

oggVorbis_t::encoderContext_t::~encoderContext_t() noexcept
{
	vorbis_info_clear(&vorbisInfo);
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
	delete p_VF;
	return 0;
}
