#include <random>
#include "oggVorbis.hxx"

/*!
 * @internal
 * @file saveOggVorbis.cpp
 * @brief The implementation of the Ogg/Vorbis encoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2019
 */

oggVorbis_t::oggVorbis_t(fd_t &&fd, audioModeWrite_t) noexcept :
	audioFile_t(audioType_t::oggVorbis, std::move(fd)), encoderCtx(makeUnique<encoderContext_t>()) { }
oggVorbis_t::encoderContext_t::encoderContext_t() noexcept : encoderState{}, blockState{},
	streamState{}, vorbisInfo{}, eos{false}
{
	std::random_device randomDev;
	vorbis_info_init(&vorbisInfo);
	ogg_stream_init(&streamState, randomDev());
}

oggVorbis_t *oggVorbis_t::openW(const char *const fileName) noexcept
{
	auto ovFile = makeUnique<oggVorbis_t>(fd_t(fileName, O_RDWR | O_CREAT | O_TRUNC, normalMode),
		audioModeWrite_t{});
	if (!ovFile || !ovFile->valid())
		return nullptr;
	return ovFile.release();
}

/*!
 * This function opens the file given by \c FileName for writing and returns a pointer
 * to the context of the opened file which must be used only by OggVorbis_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *OggVorbis_OpenW(const char *FileName) { return oggVorbis_t::openW(FileName); }

bool oggVorbis_t::encoderContext_t::writePage(const fd_t &fd, const bool force) noexcept
{
	do
	{
		ogg_page page{};
		const auto result = force ? ogg_stream_flush(&streamState, &page) :
			ogg_stream_pageout(&streamState, &page);
		if (result == 0)
			return true;
		else if (fd.write(page.header, page.header_len) != page.header_len ||
			fd.write(page.body, page.body_len) != page.body_len)
			return false;
		eos = ogg_page_eos(&page);
	}
	while (!eos);
	return true;
}

vorbis_comment copyComments(const FileInfo &info) noexcept
{
	vorbis_comment tags;
	vorbis_comment_init(&tags);
	if (info.Title)
		vorbis_comment_add_tag(&tags, "Title", info.Title);
	if (info.Artist)
		vorbis_comment_add_tag(&tags, "Artist", info.Artist);
	if (info.Album)
		vorbis_comment_add_tag(&tags, "Album", info.Album);
	for (const auto other : info.OtherComments)
		vorbis_comment_add(&tags, other);
	return tags;
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
	{ return audioFileInfo(p_VorbisFile, p_FI); }

bool oggVorbis_t::fileInfo(const FileInfo &fileInfo)
{
	auto &ctx = *encoderContext();
	fileInfo_t &info = this->fileInfo();
	ogg_packet packetHeader, packetComments, packetMode;

	vorbis_encode_init_vbr(&ctx.vorbisInfo, fileInfo.Channels, fileInfo.BitRate, 0.75F);
	vorbis_encode_setup_init(&ctx.vorbisInfo);

	vorbis_analysis_init(&ctx.encoderState, &ctx.vorbisInfo);
	vorbis_block_init(&ctx.encoderState, &ctx.blockState);

	vorbis_comment tags = copyComments(fileInfo);
	vorbis_analysis_headerout(&ctx.encoderState, &tags, &packetHeader, &packetComments, &packetMode);
	vorbis_comment_clear(&tags);
	ogg_stream_packetin(&ctx.streamState, &packetHeader);
	ogg_stream_packetin(&ctx.streamState, &packetComments);
	ogg_stream_packetin(&ctx.streamState, &packetMode);

	if (!ctx.writePage(fd(), true))
		return false;
	info.totalTime = fileInfo.TotalTime;
	info.bitsPerSample = fileInfo.BitsPerSample;
	info.bitRate = fileInfo.BitRate;
	info.channels = fileInfo.Channels;
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
	{ return audioWriteBuffer(p_VorbisFile, InBuffer, nInBufferLen); }

int64_t oggVorbis_t::writeBuffer(const void *const buffer, const uint32_t length)
{
	fileInfo_t &info = fileInfo();
	auto &ctx = *encoderContext();

	if (length <= 0)
	{
		vorbis_analysis_buffer(&ctx.encoderState, 0);
		vorbis_analysis_wrote(&ctx.encoderState, 0);
	}
	else
	{
		uint32_t bufflen = (length / 2) / info.channels;
		float **buff = vorbis_analysis_buffer(&ctx.encoderState, bufflen);
		const auto IB = static_cast<const int16_t *>(buffer);

		for (uint32_t i = 0; i < bufflen; i++)
		{
			for (uint8_t j = 0; j < info.channels; j++)
				buff[j][i] = float(IB[i * info.channels + j]) / 32768.0F;
		}

		vorbis_analysis_wrote(&ctx.encoderState, bufflen);
	}

	while (vorbis_analysis_blockout(&ctx.encoderState, &ctx.blockState) == 1)
	{
		vorbis_analysis(&ctx.blockState, nullptr);
		vorbis_bitrate_addblock(&ctx.blockState);

		ogg_packet packet{};
		while (vorbis_bitrate_flushpacket(&ctx.encoderState, &packet) == 1)
		{
			ogg_stream_packetin(&ctx.streamState, &packet);
			if (packet.bytes == 1)
				continue; // Let's do this for speed, quicker than running the next while loop to find out!
			else if (!ctx.writePage(fd(), false))
				return -2;
		}
	}

	if (!ctx.eos)
		return length;
	else if (!ctx.writePage(fd(), true))
		return -2;
	return length;
}

oggVorbis_t::encoderContext_t::~encoderContext_t() noexcept
{
	vorbis_dsp_clear(&encoderState);
	vorbis_block_clear(&blockState);
	ogg_stream_clear(&streamState);
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
int OggVorbis_CloseFileW(void *p_VorbisFile) { return audioCloseFile(p_VorbisFile); }
