#include <stdio.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modFC1x_t::modFC1x_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleFC1x, std::move(fd)} { }

modFC1x_t *modFC1x_t::openR(const char *const fileName) noexcept
{
	auto fc1xFile = makeUnique<modFC1x_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!fc1xFile || !fc1xFile->valid() || !isFC1x(fc1xFile->_fd))
		return nullptr;
	return fc1xFile.release();
}

void *FC1x_OpenR(const char *FileName)
{
	std::unique_ptr<modFC1x_t> ret{modFC1x_t::openR(FileName)};
	if (!ret)
		return nullptr;

	auto &ctx = *ret->context();
	fileInfo_t &info = ret->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	try { ctx.mod = makeUnique<ModuleFile>(*ret); }
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
			ret->player(makeUnique<playback_t>(ret.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}

	return ret.release();
}

const fileInfo_t *FC1x_GetFileInfo(void *p_FC1xFile) { return audioFileInfo(p_FC1xFile); }
long FC1x_FillBuffer(void *p_FC1xFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(&p_FC1xFile, OutBuffer, nOutBufferLen); }
int FC1x_CloseFileR(void *p_FC1xFile) { return audioCloseFile(p_FC1xFile); }
void FC1x_Play(void *p_FC1xFile) { return audioPlay(p_FC1xFile); }
void FC1x_Pause(void *p_FC1xFile) { return audioPause(p_FC1xFile); }
void FC1x_Stop(void *p_FC1xFile) { return audioStop(p_FC1xFile); }
bool Is_FC1x(const char *FileName) { return modFC1x_t::isFC1x(FileName); }

bool modFC1x_t::isFC1x(const int32_t fd) noexcept
{
	char fc1xMagic[4];
	if (fd == -1 ||
		read(fd, fc1xMagic, 4) != 4 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		(strncmp(fc1xMagic, "SMOD", 4) != 0 &&
		strncmp(fc1xMagic, "FC14", 4) != 0))
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
