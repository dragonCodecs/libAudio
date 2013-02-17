#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

void *STM_OpenR(const char *FileName)
{
	STM_Intern *ret = NULL;
	FILE *f_STM = NULL;

	ret = (STM_Intern *)malloc(sizeof(STM_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(STM_Intern));

	f_STM = fopen(FileName, "rb");
	if (f_STM == NULL)
		return f_STM;
	ret->f_STM = f_STM;

	return ret;
}

FileInfo *STM_GetFileInfo(void *p_STMFile)
{
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;
	FileInfo *ret = NULL;

	if (p_SF == NULL)
		return ret;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));
	p_SF->p_FI = ret;

	ret->BitRate = 44100;
	ret->BitsPerSample = 16;
	ret->Channels = 2;
	try
	{
		p_SF->p_File = new ModuleFile(p_SF);
	}
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->GetError());
		return NULL;
	}
	ret->Title = p_SF->p_File->GetTitle();

	if (ExternalPlayback == 0)
		p_SF->p_Playback = new Playback(ret, STM_FillBuffer, p_SF->buffer, 8192, p_STMFile);
	p_SF->p_File->InitMixer(ret);

	return ret;
}

long STM_FillBuffer(void *p_STMFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	int32_t ret = 0, Read;
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;
	if (p_SF->p_File == NULL)
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
	if (p_SF == NULL)
		return 0;

	delete p_SF->p_Playback;
	delete p_SF->p_File;

	ret = fclose(p_SF->f_STM);
	free(p_SF);
	return ret;
}

void STM_Play(void *p_STMFile)
{
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;

	p_SF->p_Playback->Play();
}

void STM_Pause(void *p_STMFile)
{
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;

	p_SF->p_Playback->Pause();
}

void STM_Stop(void *p_STMFile)
{
	STM_Intern *p_SF = (STM_Intern *)p_STMFile;

	p_SF->p_Playback->Stop();
}

bool Is_STM(const char *FileName)
{
	FILE *f_STM = fopen(FileName, "rb");
	char STMMagic[9];
	if (f_STM == NULL)
		return false;

	fseek(f_STM, 20, SEEK_CUR);
	fread(&STMMagic, 9, 1, f_STM);
	fclose(f_STM);

	if (strncmp(STMMagic, "!Scream!\x1A", 9) == 0)
		return true;
	else
		return false;
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
