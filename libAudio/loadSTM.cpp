#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

modSTM_t::modSTM_t() noexcept : modSTM_t{fd_t{}} { }
modSTM_t::modSTM_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleSTM, std::move(fd)} { }

void *STM_OpenR(const char *FileName)
{
	STM_Intern *const ret = new (std::nothrow) STM_Intern();
	if (ret == nullptr)
		return ret;

	FILE *const f_STM = fopen(FileName, "rb");
	if (f_STM == nullptr)
	{
		delete ret;
		return f_STM;
	}
	ret->f_Module = f_STM;

	fileInfo_t &info = ret->inner.fileInfo();
	info.bitRate = 44100;
	info.bitsPerSample = 16;
	info.channels = 2;

	try { ret->p_File = new ModuleFile(ret); }
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->GetError());
		delete e;
		fclose(f_STM);
		delete ret;
		return nullptr;
	}
	info.title = ret->p_File->title();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			ret->inner.player(makeUnique<Playback>(info, STM_FillBuffer, ret->buffer, 8192, const_cast<STM_Intern *>(ret)));
		ret->p_File->InitMixer(audioFileInfo(&ret->inner));
	}

	return ret;
}

FileInfo *STM_GetFileInfo(void *p_STMFile)
{
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;
	return audioFileInfo(&p_SF->inner);
}

long STM_FillBuffer(void *p_STMFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	int32_t ret = 0, Read;
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;
	if (p_SF->p_File == nullptr)
		return -1;
	do
	{
		Read = p_SF->p_File->Mix(p_SF->buffer, nOutBufferLen - ret);
		if (Read >= 0 && OutBuffer != p_SF->buffer)
			memcpy(OutBuffer + ret, p_SF->buffer, Read);
		if (Read >= 0)
			ret += Read;
	}
	while (ret < nOutBufferLen && Read >= 0);
	return (ret == 0 ? Read : ret);
}

int STM_CloseFileR(void *p_STMFile)
{
	int ret = 0;
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;
	if (p_SF == nullptr)
		return 0;

	delete p_SF->p_File;

	ret = fclose(p_SF->f_Module);
	delete p_SF;
	return ret;
}

void STM_Play(void *p_STMFile)
{
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;
	audioPlay(&p_SF->inner);
}

void STM_Pause(void *p_STMFile)
{
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;
	audioPause(&p_SF->inner);
}

void STM_Stop(void *p_STMFile)
{
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;
	audioStop(&p_SF->inner);
}

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
	STM_GetFileInfo,
	STM_FillBuffer,
	STM_CloseFileR,
	STM_Play,
	STM_Pause,
	STM_Stop
};
