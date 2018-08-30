#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

modIT_t::modIT_t(fd_t &&fd) noexcept : moduleFile_t(audioType_t::moduleIT, std::move(fd)) { }

modIT_t *modIT_t::openR(const char *const fileName) noexcept
{
	auto itFile = makeUnique<modIT_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!itFile || !itFile->valid() || !isIT(itFile->_fd))
		return nullptr;

	lseek(itFile->_fd, 0, SEEK_SET);
	return itFile.release();
}

void *IT_OpenR(const char *FileName)
{
	std::unique_ptr<modIT_t> ret{modIT_t::openR(FileName)};
	if (!ret)
		return nullptr;

	auto &ctx = *ret->context();
	fileInfo_t &info = ret->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	info.channels = 2;
	try { ctx.mod = makeUnique<ModuleFile>(*ret); }
	catch (const ModuleLoaderError &e)
	{
		printf("%s\n", e.error());
		return nullptr;
	}
	info.title = ctx.mod->title();
	info.artist = ctx.mod->author();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			ret->player(makeUnique<Playback>(info, audioFillBuffer, ctx.playbackBuffer, 8192, ret.get()));
		ctx.mod->InitMixer(audioFileInfo(ret.get()));
	}

	return ret.release();
}

FileInfo *IT_GetFileInfo(void *p_ITFile)
	{ return audioFileInfo(p_ITFile); }

long IT_FillBuffer(void *p_ITFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(p_ITFile, OutBuffer, nOutBufferLen); }

int IT_CloseFileR(void *p_ITFile) { return audioCloseFileR(p_ITFile); }
void IT_Play(void *p_ITFile) { audioPlay(p_ITFile); }
void IT_Pause(void *p_ITFile) { audioPause(p_ITFile); }
void IT_Stop(void *p_ITFile) { audioStop(p_ITFile); }

bool Is_IT(const char *FileName) { return modIT_t::isIT(FileName); }

bool modIT_t::isIT(const int32_t fd) noexcept
{
	char itSig[4];
	if (fd == -1 ||
		read(fd, itSig, 4) != 4 ||
		memcmp(itSig, "IMPM", 4) != 0)
		return false;
	return true;
}

bool modIT_t::isIT(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isIT(file);
}

API_Functions ITDecoder =
{
	IT_OpenR,
	audioFileInfo,
	audioFillBuffer,
	audioCloseFileR,
	audioPlay,
	audioPause,
	audioStop
};
