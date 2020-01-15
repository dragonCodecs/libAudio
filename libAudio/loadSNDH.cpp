#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadSNDH.cpp
 * @brief The implementation of the SNDH decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2019
 */

struct sndh_t::decoderContext_t final
{
	uint8_t buffer[8192];
};

sndh_t::sndh_t(fd_t &&fd) noexcept : audioFile_t{audioType_t::sndh, std::move(fd)}, ctx{makeUnique<decoderContext_t>()} { }

sndh_t *sndh_t::openR(const char *const fileName) noexcept
{
	std::unique_ptr<sndh_t> file{makeUnique<sndh_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isSNDH(file->_fd))
		return nullptr;
	return file.release();
}

void *SNDH_OpenR(const char *fileName) { return sndh_t::openR(fileName); }
const fileInfo_t *SNDH_GetFileInfo(void *p_SNDHFile) { return audioFileInfo(p_SNDHFile); }
long SNDH_FillBuffer(void *p_SNDHFile, uint8_t *OutBuffer, int countBufferLen)
	{ return audioFillBuffer(p_SNDHFile, OutBuffer, countBufferLen); }

int64_t sndh_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	return -2;
}

int SNDH_CloseFileR(void *p_SNDHFile) { return audioCloseFile(p_SNDHFile); }
void SNDH_Play(void *p_SNDHFile) { audioPlay(p_SNDHFile); }
void SNDH_Pause(void *p_SNDHFile) { audioPause(p_SNDHFile); }
void SNDH_Stop(void *p_SNDHFile) { audioStop(p_SNDHFile); }

bool Is_SNDH(const char *fileName) { return sndh_t::isSNDH(fileName); }

bool sndh_t::isSNDH(const int32_t fd) noexcept
{
	char magic[4];
	if (fd == -1 ||
		read(fd, magic, 4) != 4 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		// All SNDH files begin with "ICE!" and this is the test
		// that the Linux/Unix Magic Numbers system does too, so
		// it will always work
		memcmp(magic, "ICE!", 4) != 0)
		return false;
	return true;
}

bool sndh_t::isSNDH(const char *const fileName) noexcept
{
	fd_t file{fileName, O_RDONLY | O_NOCTTY};
	if (!file.valid())
		return false;
	return isSNDH(file);
}

/*!
 * @internal
 * This structure controls decoding SNDH files when using the high-level API on them
 */
API_Functions SNDHDecoder =
{
	SNDH_OpenR,
	nullptr,
	audioFileInfo,
	nullptr,
	audioFillBuffer,
	nullptr,
	audioCloseFile,
	nullptr,
	audioPlay,
	audioPause,
	audioStop
};
