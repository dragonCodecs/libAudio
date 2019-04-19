#include <stdio.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modFC1x_t::modFC1x_t() noexcept : moduleFile_t{audioType_t::moduleFC1x, {}} { }

void *FC1x_OpenR(const char *FileName)
{
	std::unique_ptr<FC1x_Intern> ret = makeUnique<FC1x_Intern>();
	if (!ret)
		return nullptr;

	auto &ctx = *ret->inner.context();
	fileInfo_t &info = ret->inner.fileInfo();

	ret->f_Module = fopen(FileName, "rb");
	if (!ret->f_Module)
		return nullptr;

	info.bitRate = 44100;
	info.bitsPerSample = 16;
#ifdef FC1x_EXPERIMENTAL
	try { ctx.mod = makeUnique<ModuleFile>(ret.get()); }
	catch (const ModuleLoaderError &e)
	{
		printf("%s\n", e.error());
		return nullptr;
	}
#else
	return nullptr;
#endif
	info.title = ctx.mod->title();
	info.channels = ctx.mod->channels();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			ret->inner.player(makeUnique<playback_t>(&ret->inner, audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}

	return ret.release();
}

const fileInfo_t *FC1x_GetFileInfo(void *p_FC1xFile)
{
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;
	return audioFileInfo(&p_FF->inner);
}

long FC1x_FillBuffer(void *p_FC1xFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;
	return audioFillBuffer(&p_FF->inner, OutBuffer, nOutBufferLen);
}

int FC1x_CloseFileR(void *p_FC1xFile)
{
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;
	if (!p_FF)
		return 0;
	const int ret = fclose(p_FF->f_Module);
	delete p_FF;
	return ret;
}

void FC1x_Play(void *p_FC1xFile)
{
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;
	p_FF->inner.play();
}

void FC1x_Pause(void *p_FC1xFile)
{
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;
	p_FF->inner.pause();
}

void FC1x_Stop(void *p_FC1xFile)
{
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;
	p_FF->inner.stop();
}

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
	FC1x_GetFileInfo,
	nullptr,
	FC1x_FillBuffer,
	nullptr,
	FC1x_CloseFileR,
	nullptr,
	FC1x_Play,
	FC1x_Pause,
	FC1x_Stop
};
