// SPDX-License-Identifier: BSD-3-Clause
#include <random>
#include <limits>
#include "oggVorbis.hxx"

/*!
 * @internal
 * @file saveOggVorbis.cpp
 * @brief The implementation of the Ogg|Vorbis encoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2010-2020
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
	auto file{makeUnique<oggVorbis_t>(fd_t{fileName, O_RDWR | O_CREAT | O_TRUNC, substrate::normalMode},
		audioModeWrite_t{})};
	if (!file || !file->valid())
		return nullptr;
	return file.release();
}

/*!
 * This function opens the file given by \c fileName for writing and returns a pointer
 * to the context of the opened file which must be used only by OggVorbis_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
 */
void *oggVorbisOpenW(const char *fileName) { return oggVorbis_t::openW(fileName); }

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

vorbis_comment copyComments(const fileInfo_t &info) noexcept
{
	vorbis_comment tags;
	vorbis_comment_init(&tags);
	if (info.title)
		vorbis_comment_add_tag(&tags, "Title", info.title.get());
	if (info.artist)
		vorbis_comment_add_tag(&tags, "Artist", info.artist.get());
	if (info.album)
		vorbis_comment_add_tag(&tags, "Album", info.album.get());
	for (const auto &other : info.other)
		vorbis_comment_add(&tags, other.get());
	return tags;
}

/*!
 * This function sets the \c FileInfo structure for a Ogg/Vorbis file being encoded
 * @param p_VorbisFile A pointer to a file opened with \c oggVorbisOpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file
 * @warning This function must be called before using \c OggVorbis_WriteBuffer()
 */
bool oggVorbis_t::fileInfo(const fileInfo_t &info)
{
	auto &ctx = *encoderContext();
	ogg_packet packetHeader, packetComments, packetMode;

	vorbis_encode_init_vbr(&ctx.vorbisInfo, info.channels, info.bitRate, 0.75F);
	vorbis_encode_setup_init(&ctx.vorbisInfo);

	vorbis_analysis_init(&ctx.encoderState, &ctx.vorbisInfo);
	vorbis_block_init(&ctx.encoderState, &ctx.blockState);

	vorbis_comment tags = copyComments(info);
	vorbis_analysis_headerout(&ctx.encoderState, &tags, &packetHeader, &packetComments, &packetMode);
	vorbis_comment_clear(&tags);
	ogg_stream_packetin(&ctx.streamState, &packetHeader);
	ogg_stream_packetin(&ctx.streamState, &packetComments);
	ogg_stream_packetin(&ctx.streamState, &packetMode);

	if (!ctx.writePage(fd(), true))
		return false;
	fileInfo() = info;
	return true;
}

template<typename T> uint32_t fillFrame(oggVorbis_t &file, const void *const bufferPtr,
	const uint32_t length)
{
	const fileInfo_t &info = file.fileInfo();
	auto &ctx = *file.encoderContext();
	const uint32_t sampleCount = length / (sizeof(T) * info.channels);
	const auto encoderBuffer = vorbis_analysis_buffer(&ctx.encoderState, sampleCount);
	const auto buffer = static_cast<const T *>(bufferPtr);
	using limits = std::numeric_limits<T>;
	for (uint32_t index = 0, i = 0; i < sampleCount; ++i)
	{
		for (uint8_t channel = 0; channel < info.channels; ++channel)
			encoderBuffer[channel][i] = float(buffer[index++]) / limits::max();
	}
	return sampleCount;
}

/*!
 * This function writes a buffer of audio to a Ogg/Vorbis file opened being encoded
 * @param p_VorbisFile A pointer to a file opened with \c oggVorbisOpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @attention Will not work unless \c OggVorbis_SetFileInfo() has been called beforehand
 */
int64_t oggVorbis_t::writeBuffer(const void *const bufferPtr, const int64_t rawLength)
{
	const fileInfo_t &info = fileInfo();
	auto &ctx = *encoderContext();
	uint32_t sampleCount = 0;
	if (rawLength <= 0)
		return rawLength;
	const uint32_t length = uint32_t(rawLength);
	if (info.bitsPerSample == 8)
		sampleCount = fillFrame<int8_t>(*this, bufferPtr, length);
	else if (info.bitsPerSample == 16)
		sampleCount = fillFrame<int16_t>(*this, bufferPtr, length);

	vorbis_analysis_wrote(&ctx.encoderState, sampleCount);
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
