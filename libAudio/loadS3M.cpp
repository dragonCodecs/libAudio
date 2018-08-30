#include <stdio.h>
#include <malloc.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

modS3M_t::modS3M_t() noexcept : modS3M_t{fd_t{}} { }
modS3M_t::modS3M_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleS3M, std::move(fd)} { }

void *S3M_OpenR(const char *FileName)
{
	S3M_Intern *const ret = new (std::nothrow) S3M_Intern();
	if (ret == NULL)
		return ret;

	FILE *const f_S3M = fopen(FileName, "rb");
	if (f_S3M == NULL)
	{
		delete ret;
		return f_S3M;
	}
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

	if (ToPlayback)
	{
		if (ExternalPlayback == 0)
			p_SF->p_Playback = new Playback(ret, S3M_FillBuffer, p_SF->buffer, 8192, p_S3MFile);
		p_SF->p_File->InitMixer(ret);
	}

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
	delete p_SF;
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

bool Is_S3M(const char *FileName) { return modS3M_t::isS3M(FileName); }

bool modS3M_t::isS3M(const int32_t fd) noexcept
{
	constexpr const uint32_t seekOffset1 = 28;
	constexpr const uint32_t seekOffset2 = seekOffset1 + 16;
	char S3MMagic1, S3MMagic2[4];
	if (fd == -1 ||
		lseek(fd, seekOffset1, SEEK_SET) != seekOffset1 ||
		read(fd, &S3MMagic1, 1) != 1 ||
		lseek(fd, seekOffset2, SEEK_SET) != seekOffset2 ||
		read(fd, S3MMagic2, 4) != 4 ||
		S3MMagic1 != 0x1A ||
		strncmp(S3MMagic2, "SCRM", 4) != 0)
		return false;
	else
		return true;
}

bool modS3M_t::isS3M(const char *const fileName) noexcept
{
	fd_t file{fileName, O_RDONLY | O_NOCTTY};
	if (!file.valid())
		return false;
	return isS3M(file);
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
