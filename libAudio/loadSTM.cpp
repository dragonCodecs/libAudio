#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modSTM_t::modSTM_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleSTM, std::move(fd)} { }

modSTM_t *modSTM_t::openR(const char *const fileName) noexcept
{
	auto stmFile = makeUnique<modSTM_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!stmFile || !stmFile->valid() || !isSTM(stmFile->_fd))
		return nullptr;
	lseek(stmFile->_fd, 0, SEEK_SET);
	return stmFile.release();
}

void *STM_OpenR(const char *FileName)
{
	std::unique_ptr<modSTM_t> ret{modSTM_t::openR(FileName)};
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
		printf("%s\n", e.GetError());
		return nullptr;
	}
	info.title = ctx.mod->title();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			ret->player(makeUnique<playback_t>(ret.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}

	return ret.release();
}

const fileInfo_t *STM_GetFileInfo(void *p_STMFile) { return audioFileInfo(p_STMFile); }
long STM_FillBuffer(void *p_STMFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(p_STMFile, OutBuffer, nOutBufferLen); }
int STM_CloseFileR(void *p_STMFile) { return audioCloseFile(p_STMFile); }
void STM_Play(void *p_STMFile) { audioPlay(p_STMFile); }
void STM_Pause(void *p_STMFile) { audioPause(p_STMFile); }
void STM_Stop(void *p_STMFile) { audioStop(p_STMFile); }
bool Is_STM(const char *FileName) { return modSTM_t::isSTM(FileName); }

bool modSTM_t::isSTM(const int32_t fd) noexcept
{
	constexpr uint32_t offset = 20;
	char STMMagic[9];
	if (fd == -1 ||
		lseek(fd, offset, SEEK_SET) != offset ||
		read(fd, STMMagic, 9) != 9 ||
		memcmp(STMMagic, "!Scream!\x1A", 9) != 0)
		return false;
	else
		return true;
}

bool modSTM_t::isSTM(const char *const fileName) noexcept
{
	fd_t file{fileName, O_RDONLY | O_NOCTTY};
	if (!file.valid())
		return false;
	return isSTM(file);
}

API_Functions STMDecoder =
{
	STM_OpenR,
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
