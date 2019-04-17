#include <stdio.h>
#include <malloc.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modFC1x_t::modFC1x_t() noexcept : moduleFile_t{audioType_t::moduleFC1x, {}} { }

void *FC1x_OpenR(const char *FileName)
{
	FILE *f_FC1x = NULL;

	std::unique_ptr<FC1x_Intern> ret = makeUnique<FC1x_Intern>();
	if (ret == nullptr)
		return nullptr;

	f_FC1x = fopen(FileName, "rb");
	if (f_FC1x == NULL)
		return f_FC1x;
	ret->f_Module = f_FC1x;

	return ret.release();
}

FileInfo *FC1x_GetFileInfo(void *p_FC1xFile)
{
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;
	FileInfo *ret = NULL;

	if (p_FF == NULL)
		return ret;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));
	p_FF->p_FI = ret;

	ret->BitRate = 44100;
	ret->BitsPerSample = 16;
	try
	{
		p_FF->p_File = new ModuleFile(p_FF);
	}
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->GetError());
		return NULL;
	}
	ret->Title = p_FF->p_File->title().release();
	ret->Channels = p_FF->p_File->channels();

	if (ToPlayback)
	{
		if (ExternalPlayback == 0)
			p_FF->p_Playback = new playback_t(p_FC1xFile, FC1x_FillBuffer, p_FF->buffer, 8192, ret);
		p_FF->p_File->InitMixer(ret);
	}

	return ret;
}

long FC1x_FillBuffer(void *p_FC1xFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	int32_t ret = 0, Read;
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;
	if (p_FF->p_File == NULL)
		return -1;
	do
	{
		Read = p_FF->p_File->Mix(p_FF->buffer, nOutBufferLen - ret);
		if (Read >= 0 && OutBuffer != p_FF->buffer)
			memcpy(OutBuffer + ret, p_FF->buffer, Read);
		if (Read >= 0)
			ret += Read;
	}
	while (ret < nOutBufferLen && Read >= 0);
	return (ret == 0 ? Read : ret);
}

int FC1x_CloseFileR(void *p_FC1xFile)
{
	int ret = 0;
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;
	if (p_FF == NULL)
		return 0;

	delete p_FF->p_Playback;
	delete p_FF->p_File;

	ret = fclose(p_FF->f_Module);
	delete p_FF;
	return ret;
}

void FC1x_Play(void *p_FC1xFile)
{
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;

	p_FF->p_Playback->play();
}

void FC1x_Pause(void *p_FC1xFile)
{
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;

	p_FF->p_Playback->pause();
}

void FC1x_Stop(void *p_FC1xFile)
{
	FC1x_Intern *p_FF = (FC1x_Intern *)p_FC1xFile;

	p_FF->p_Playback->stop();
}

bool Is_FC1x(const char *FileName)
{
	FILE *f_FC1x = fopen(FileName, "rb");
	char FC1xMagic[4];
	if (f_FC1x == NULL)
		return false;

	// TODO: Improve this test!
	fread(FC1xMagic, 4, 1, f_FC1x);
	fclose(f_FC1x);

	if (strncmp(FC1xMagic, "SMOD", 4) == 0 || strncmp(FC1xMagic, "FC14", 4) == 0)
		return true;
	else
		return false;
}

API_Functions FC1xDecoder =
{
	FC1x_OpenR,
	FC1x_GetFileInfo,
	FC1x_FillBuffer,
	FC1x_CloseFileR,
	FC1x_Play,
	FC1x_Pause,
	FC1x_Stop
};
