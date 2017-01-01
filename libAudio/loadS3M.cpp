#include <stdio.h>
#include <malloc.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

void *S3M_OpenR(const char *FileName)
{
	S3M_Intern *ret = NULL;
	FILE *f_S3M = NULL;

	ret = (S3M_Intern *)malloc(sizeof(S3M_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(S3M_Intern));

	f_S3M = fopen(FileName, "rb");
	if (f_S3M == NULL)
		return f_S3M;
	ret->f_Module = f_S3M;

	return ret;
}

FileInfo *S3M_GetFileInfo(void *p_S3MFile)
{
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;
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
	try
	{
		p_SF->p_File = new ModuleFile(p_SF);
	}
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->GetError());
		return NULL;
	}
	ret->Title = p_SF->p_File->title().release();
	ret->Channels = p_SF->p_File->channels();

	if (ExternalPlayback == 0)
		p_SF->p_Playback = new Playback(ret, S3M_FillBuffer, p_SF->buffer, 8192, p_S3MFile);
	p_SF->p_File->InitMixer(ret);

	return ret;
}

long S3M_FillBuffer(void *p_S3MFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	int32_t ret = 0, Read;
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;
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

int S3M_CloseFileR(void *p_S3MFile)
{
	int ret = 0;
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;
	if (p_SF == NULL)
		return 0;

	delete p_SF->p_Playback;
	delete p_SF->p_File;

	ret = fclose(p_SF->f_Module);
	free(p_SF);
	return ret;
}

void S3M_Play(void *p_S3MFile)
{
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;

	p_SF->p_Playback->Play();
}

void S3M_Pause(void *p_S3MFile)
{
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;

	p_SF->p_Playback->Pause();
}

void S3M_Stop(void *p_S3MFile)
{
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;

	p_SF->p_Playback->Stop();
}

bool Is_S3M(const char *FileName)
{
	FILE *f_S3M = fopen(FileName, "rb");
	char S3MMagic1, S3MMagic2[4];
	if (f_S3M == NULL)
		return false;

	fseek(f_S3M, 28, SEEK_CUR);
	fread(&S3MMagic1, 1, 1, f_S3M);
	fseek(f_S3M, 15, SEEK_CUR);
	fread(S3MMagic2, 4, 1, f_S3M);
	fclose(f_S3M);

	if (S3MMagic1 == 0x1A && strncmp(S3MMagic2, "SCRM", 4) == 0)
		return true;
	else
		return false;
}

API_Functions S3MDecoder =
{
	S3M_OpenR,
	S3M_GetFileInfo,
	S3M_FillBuffer,
	S3M_CloseFileR,
	S3M_Play,
	S3M_Pause,
	S3M_Stop
};
