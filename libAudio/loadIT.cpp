#include <stdio.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modIT_t::modIT_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleIT, std::move(fd)} { }

modIT_t *modIT_t::openR(const char *const fileName) noexcept
{
	auto file = makeUnique<modIT_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!file || !file->valid() || !isIT(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	info.channels = 2;
	try { ctx.mod = makeUnique<ModuleFile>(*file); }
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
			file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}
	return file.release();
}

void *IT_OpenR(const char *fileName) { return modIT_t::openR(fileName); }
const fileInfo_t *IT_GetFileInfo(void *itFile) { return audioFileInfo(itFile); }
long IT_FillBuffer(void *itFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(itFile, OutBuffer, nOutBufferLen); }
int IT_CloseFileR(void *itFile) { return audioCloseFile(itFile); }
void IT_Play(void *itFile) { audioPlay(itFile); }
void IT_Pause(void *itFile) { audioPause(itFile); }
void IT_Stop(void *itFile) { audioStop(itFile); }
bool Is_IT(const char *fileName) { return modIT_t::isIT(fileName); }

bool modIT_t::isIT(const int32_t fd) noexcept
{
	char itSig[4];
	if (fd == -1 ||
		read(fd, itSig, 4) != 4 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
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
