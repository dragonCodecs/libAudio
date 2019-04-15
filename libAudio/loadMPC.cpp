#ifndef __NO_MPC__

#include <algorithm>
#include <mpc/mpcdec.h>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadMPC.cpp
 * @brief The implementation of the MPC decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2013
 */

struct mpc_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The MPC demultiplexer handle
	 */
	mpc_demux *demuxer;
	/*!
	 * @internal
	 * The MPC stream information handle which holds the metadata
	 * returned by \c mpc_demux_get_info()
	 */
	mpc_streaminfo streamInfo;
	/*!
	 * @internal
	 * The MPC frame handle which holds, among other things, the decoded
	 * MPC data returned by \c mpc_demux_decode()
	 */
	mpc_frame_info frameInfo;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	/*!
	 * @internal
	 * The MPC callbacks/reader information handle
	 */
	mpc_reader callbacks;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
};

/*!
 * @internal
 * Internal structure for holding the decoding context for a given MPC file
 */
typedef struct _MPC_Intern
{
	/*!
	 * @internal
	 * The decoded MPC buffer filled by \c mpc_demux_decode()
	 */
	MPC_SAMPLE_FORMAT framebuff[MPC_DECODER_BUFFER_LENGTH];
	/*!
	 * @internal
	 * The count of how much of the current decoded data buffer has been used
	 */
	uint32_t PCMUsed;

	mpc_t inner;

	_MPC_Intern(const char *const fileName) noexcept : inner(fd_t(fileName, O_RDONLY | O_NOCTTY)) { }
} MPC_Intern;

namespace libAudio
{
	namespace mpc
	{
		/*!
		* @internal
		* This function applies a simple conversion algorithm to convert the input
		* floating point MPC sample to a short for playback
		* @param Sample The floating point sample to convert
		* @return The converted floating point sample
		* @bug This function applies no noise shaping or dithering
		*   So the output is sub-par to what it could be. FIXME!
		*/
		int16_t floatToInt16(MPC_SAMPLE_FORMAT sample)
		{
			using limits = std::numeric_limits<int16_t>;
			if (sample <= -1.0F)
				return limits::min();
			else if (sample >= 1.0F)
				return limits::max();
			return int16_t(sample * limits::max());
		}

		/*!
		* @internal
		* \c read() is the internal read callback for MPC file decoding.
		* This prevents nasty things from happening on Windows thanks to the run-time mess there.
		* @param p_MPCFile Structure holding our own internal context pointer which
		*   holds the file to seek through
		* @param p_Buffer The buffer to read into
		* @param size The number of bytes to read into the buffer
		* @return The return result of \c fread()
		*/
		int32_t read(mpc_reader *reader, void *buffer, int bufferLen)
		{
			mpc_t *file = static_cast<mpc_t *>(reader->data);
			return file->fd().read(buffer, bufferLen, nullptr);
		}

		/*!
		* @internal
		* \c seek() is the internal seek callback for MPC file decoding.
		* This prevents nasty things from happening on Windows thanks to the run-time mess there.
		* @param p_MPCFile Structure holding our own internal context pointer which
		*   holds the file to seek through
		* @param offset The offset through the file to which to seek to
		* @return A truth value giving if the seek succeeded or not
		*/
		uint8_t seek(mpc_reader *reader, int offset)
		{
			mpc_t *file = static_cast<mpc_t *>(reader->data);
			return file->fd().seek(offset, SEEK_SET) == offset;
		}

		/*!
		* @internal
		* \c tell() is the internal read possition callback for MPC file decoding.
		* This prevents nasty things from happening on Windows thanks to the run-time mess there.
		* @param p_MPCFile Structure holding our own internal context pointer which
		*   holds the file to seek through
		* @return An integer giving the read possition of the file in bytes
		*/
		int32_t tell(mpc_reader *reader)
		{
			mpc_t *file = static_cast<mpc_t *>(reader->data);
			return file->fd().tell();
		}

		/*!
		* @internal
		* \c length() is the internal file length callback for MPC file decoding.
		* This prevents nasty things from happening on Windows thanks to the run-time mess there.
		* @param p_MPCFile Structure holding our own internal context pointer which
		*   holds the file to seek through
		* @return An integer giving the length of the file in bytes
		*/
		int32_t length(mpc_reader *reader)
		{
			mpc_t *file = static_cast<mpc_t *>(reader->data);
			return file->fd().length();
		}

		/*!
		* @internal
		* \c canSeek() is the internal callback for determining if an MPC file being
		* decoded can be seeked on or not. \n This does two things: \n
		* - It prevents nasty things from happening on Windows thanks to the run-time mess there
		* - It uses \c fseek() as a no-operation to determine if we can seek or not.
		*
		* @param p_MPCFile Structure holding our own internal context pointer which
		*   holds the file to seek through
		* @return A truth value giving if seeking can work or not
		*/
		uint8_t canSeek(mpc_reader *reader)
		{
			mpc_t *file = static_cast<mpc_t *>(reader->data);
			return file->fd().tell() != -1;
		}
	}
}

using namespace libAudio;

mpc_t::mpc_t(fd_t &&fd) noexcept : audioFile_t(audioType_t::musePack, std::move(fd)), ctx(makeUnique<decoderContext_t>()) { }
mpc_t::decoderContext_t::decoderContext_t() noexcept : demuxer{nullptr}, streamInfo{}, frameInfo{},
	callbacks{mpc::read, mpc::seek, mpc::tell, mpc::length, mpc::canSeek, nullptr} { }

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by MPC_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *MPC_OpenR(const char *FileName)
{
	std::unique_ptr<MPC_Intern> ret = makeUnique<MPC_Intern>(FileName);
	if (!ret || !ret->inner.context())
		return nullptr;
	auto &ctx = *ret->inner.context();
	fileInfo_t &info = ret->inner.fileInfo();

	ctx.callbacks.data = &ret->inner;
	ctx.demuxer = mpc_demux_init(&ctx.callbacks);
	ctx.frameInfo.buffer = ret->framebuff;
	mpc_demux_get_info(ctx.demuxer, &ctx.streamInfo);

	info.bitsPerSample = ctx.streamInfo.bitrate == 0 ? 16 : ctx.streamInfo.bitrate;
	info.bitRate = ctx.streamInfo.sample_freq;
	info.channels = ctx.streamInfo.channels;
	info.totalTime = ctx.streamInfo.samples / info.bitRate;

	if (ExternalPlayback == 0)
		ret->inner.player(makeUnique<playback_t>(ret.get(), MPC_FillBuffer, ctx.playbackBuffer, 8192, info));

	return ret.release();
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_MPCFile A pointer to a file opened with \c MPC_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c MPC_Play() or \c MPC_FillBuffer()
 * @bug \p p_MPCFile must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *MPC_GetFileInfo(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	return audioFileInfo(&p_MF->inner);
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_MPCFile A pointer to a file opened with \c MPC_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_MPCFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long MPC_FillBuffer(void *p_MPCFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	uint32_t offset = 0;
	const fileInfo_t &info = p_MF->inner.fileInfo();
	auto &ctx = *p_MF->inner.context();

	while (offset < nOutBufferLen)
	{
		int16_t *out = reinterpret_cast<int16_t *>(ctx.playbackBuffer + offset);
		int nOut = 0;

		if (p_MF->PCMUsed == 0)
		{
			if (mpc_demux_decode(ctx.demuxer, &ctx.frameInfo) != 0 || ctx.frameInfo.bits == -1)
				return -2;
		}

		for (uint32_t i = p_MF->PCMUsed; i < ctx.frameInfo.samples; i++)
		{
			short Sample = 0;

			if (info.channels == 1)
			{
				Sample = mpc::floatToInt16(ctx.frameInfo.buffer[i]);
				out[(nOut / 2)] = Sample;
				nOut += 2;
			}
			else if (info.channels == 2)
			{
				Sample = mpc::floatToInt16(ctx.frameInfo.buffer[(i * 2) + 0]);
				out[(nOut / 2) + 0] = Sample;

				Sample = mpc::floatToInt16(ctx.frameInfo.buffer[(i * 2) + 1]);
				out[(nOut / 2) + 1] = Sample;
				nOut += 4;
			}

			if ((offset + nOut) >= nOutBufferLen)
			{
				p_MF->PCMUsed = ++i;
				break;
			}
		}

		if ((offset + nOut) < nOutBufferLen)
			p_MF->PCMUsed = 0;

		if (OutBuffer != ctx.playbackBuffer)
			memcpy(OutBuffer + offset, out, nOut);
		offset += nOut;
	}

	return offset;
}

int64_t mpc_t::fillBuffer(void *const buffer, const uint32_t length) { return -1; }

mpc_t::decoderContext_t::~decoderContext_t() noexcept
	{ mpc_demux_exit(demuxer); }

/*!
 * Closes an opened audio file
 * @param p_MPCFile A pointer to a file opened with \c MPC_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_MPCFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_MPCFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int MPC_CloseFileR(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	delete p_MF;
	return 0;
}

/*!
 * Plays an opened MPC file using OpenAL on the default audio device
 * @param p_MPCFile A pointer to a file opened with \c MPC_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c MPC_OpenR() used to open the file at \p p_MPCFile,
 * this function will do nothing.
 * @bug \p p_MPCFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_MPCFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void MPC_Play(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	p_MF->inner.play();
}

void MPC_Pause(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	p_MF->inner.pause();
}

void MPC_Stop(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	p_MF->inner.stop();
}

/*!
 * Checks the file given by \p FileName for whether it is an MPC
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an MPC file or not
 */
bool Is_MPC(const char *FileName) { return mpc_t::isMPC(FileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a MPC
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a MPC file or not
 */
bool mpc_t::isMPC(const int32_t fd) noexcept
{
	char mpcSig[3];
	if (fd == -1 ||
		read(fd, mpcSig, 3) != 3 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		(strncmp(mpcSig, "MP+", 3) != 0 &&
		strncmp(mpcSig, "MPC", 3) != 0))
		return false;
	return true;
}

/*!
 * Checks the file given by \p fileName for whether it is a MPC
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a MPC file or not
 */
bool mpc_t::isMPC(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isMPC(file);
}

/*!
 * @internal
 * This structure controls decoding MPC files when using the high-level API on them
 */
API_Functions MPCDecoder =
{
	MPC_OpenR,
	MPC_GetFileInfo,
	MPC_FillBuffer,
	MPC_CloseFileR,
	MPC_Play,
	MPC_Pause,
	MPC_Stop
};

#endif /* __NO_MPC__ */

