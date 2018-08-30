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
	if (ret == nullptr)
		return ret;

	ret->inner.fd({FileName, O_RDONLY | O_NOCTTY});
	if (!ret->inner.fd().valid())
	{
		delete ret;
		return nullptr;
	}
	ret->f_Module = fdopen(ret->inner.fd(), "rb");

	fileInfo_t &info = ret->inner.fileInfo();
	info.bitRate = 44100;
	info.bitsPerSample = 16;
	try { ret->p_File = new ModuleFile(ret); }
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->GetError());
		delete e;
		fclose(ret->f_Module);
		delete ret;
		return nullptr;
	}
	catch (const ModuleLoaderError &e)
	{
		printf("%s\n", e.GetError());
		fclose(ret->f_Module);
		delete ret;
		return nullptr;
	}
	info.title = ret->p_File->title();
	info.channels = ret->p_File->channels();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			ret->inner.player(makeUnique<Playback>(info, S3M_FillBuffer, ret->buffer, 8192, const_cast<S3M_Intern *>(ret)));
		ret->p_File->InitMixer(audioFileInfo(&ret->inner));
	}

	return ret;
}

FileInfo *S3M_GetFileInfo(void *p_S3MFile)
{
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;
	return audioFileInfo(&p_SF->inner);
}

long S3M_FillBuffer(void *p_S3MFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;
	if (p_SF->p_File == nullptr)
		return -1;
	return p_SF->p_File->Mix(OutBuffer, nOutBufferLen);
}

int S3M_CloseFileR(void *p_S3MFile)
{
	int ret = 0;
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;
	if (p_SF == nullptr)
		return 0;

	delete p_SF->p_File;

	ret = fclose(p_SF->f_Module);
	delete p_SF;
	return ret;
}

void S3M_Play(void *p_S3MFile)
{
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;
	audioPlay(&p_SF->inner);
}

void S3M_Pause(void *p_S3MFile)
{
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;
	audioPause(&p_SF->inner);
}

void S3M_Stop(void *p_S3MFile)
{
	S3M_Intern *p_SF = (S3M_Intern *)p_S3MFile;
	audioStop(&p_SF->inner);
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
