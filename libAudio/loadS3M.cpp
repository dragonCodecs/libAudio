#include <stdio.h>
#include <malloc.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modS3M_t::modS3M_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleS3M, std::move(fd)} { }

modS3M_t *modS3M_t::openR(const char *const fileName) noexcept
{
	auto file = makeUnique<modS3M_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!file || !file->valid() || !isS3M(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	try { ctx.mod = makeUnique<ModuleFile>(*file); }
	catch (const ModuleLoaderError &e)
	{
		printf("%s\n", e.GetError());
		return nullptr;
	}
	info.title = ctx.mod->title();
	info.channels = ctx.mod->channels();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}
	return file.release();
}

void *S3M_OpenR(const char *FileName) { return modS3M_t::openR(FileName); }
const fileInfo_t *S3M_GetFileInfo(void *p_S3MFile) { return audioFileInfo(p_S3MFile); }
long S3M_FillBuffer(void *p_S3MFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(p_S3MFile, OutBuffer, nOutBufferLen); }
int S3M_CloseFileR(void *p_S3MFile) { return audioCloseFile(p_S3MFile); }
void S3M_Play(void *p_S3MFile) { audioPlay(p_S3MFile); }
void S3M_Pause(void *p_S3MFile) { audioPause(p_S3MFile); }
void S3M_Stop(void *p_S3MFile) { audioStop(p_S3MFile); }
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
		lseek(fd, 0, SEEK_SET) != 0 ||
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
