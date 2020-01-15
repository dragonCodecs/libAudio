#include <stdio.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modFC1x_t::modFC1x_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleFC1x, std::move(fd)} { }

modFC1x_t *modFC1x_t::openR(const char *const fileName) noexcept
{
	auto file = makeUnique<modFC1x_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!file || !file->valid() || !isFC1x(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	try { ctx.mod = makeUnique<ModuleFile>(*file); }
	catch (const ModuleLoaderError &e)
	{
		printf("%s\n", e.error());
		return nullptr;
	}
	info.title = ctx.mod->title();
	info.channels = ctx.mod->channels();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}
	return file.release();
}

void *FC1x_OpenR(const char *fileName) { return modFC1x_t::openR(fileName); }
const fileInfo_t *FC1x_GetFileInfo(void *fc1xFile) { return audioFileInfo(fc1xFile); }
long FC1x_FillBuffer(void *fc1xFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(&fc1xFile, OutBuffer, nOutBufferLen); }
int FC1x_CloseFileR(void *fc1xFile) { return audioCloseFile(fc1xFile); }
void FC1x_Play(void *fc1xFile) { return audioPlay(fc1xFile); }
void FC1x_Pause(void *fc1xFile) { return audioPause(fc1xFile); }
void FC1x_Stop(void *fc1xFile) { return audioStop(fc1xFile); }
bool Is_FC1x(const char *fileName) { return modFC1x_t::isFC1x(fileName); }

bool modFC1x_t::isFC1x(const int32_t fd) noexcept
{
	char fc1xMagic[4];
	if (fd == -1 ||
		read(fd, fc1xMagic, 4) != 4 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		(memcmp(fc1xMagic, "SMOD", 4) != 0 &&
		memcmp(fc1xMagic, "FC14", 4) != 0))
		return false;
	return true;
}

bool modFC1x_t::isFC1x(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isFC1x(file);
}

API_Functions FC1xDecoder =
{
	FC1x_OpenR,
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
