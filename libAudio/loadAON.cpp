#include <stdio.h>
#include <malloc.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modAON_t::modAON_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleAON, std::move(fd)} { }

modAON_t *modAON_t::openR(const char *const fileName) noexcept
{
	auto file = makeUnique<modAON_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!file || !file->valid() || !isAON(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	info.channels = 2;
	try { ctx.mod = makeUnique<ModuleFile>(*file); }
	catch (const ModuleLoaderError &e)
	{
		printf("%s\n", e.error());
		return nullptr;
	}
	info.title = ctx.mod->title();
	info.artist = ctx.mod->author();
	auto remark = ctx.mod->remark();
	if (remark)
		info.other.emplace_back(std::move(remark));
	//info.channels = ctx.mod->channels();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}
	return file.release();
}

void *AON_OpenR(const char *fileName) { return modAON_t::openR(fileName); }
const fileInfo_t *AON_GetFileInfo(void *aonFile) { return audioFileInfo(aonFile); }
long AON_FillBuffer(void *aonFile, void *const buffer, const uint32_t length)
	{ return audioFillBuffer(aonFile, buffer, length); }
int AON_CloseFileR(void *aonFile) { return audioCloseFile(aonFile); }
void AON_Play(void *aonFile) { audioPlay(aonFile); }
void AON_Pause(void *aonFile) { audioPause(aonFile); }
void AON_Stop(void *aonFile) { audioStop(aonFile); }
bool Is_AON(const char *fileName) { return modAON_t::isAON(fileName); }

bool modAON_t::isAON(const int32_t fd) noexcept
{
	char aonMagic1[4], aonMagic2[42];
	if (fd == -1 ||
		read(fd, aonMagic1, 4) != 4 ||
		read(fd, aonMagic2, 42) != 42 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		memcmp(aonMagic1, "AON", 3) != 0 ||
		memcmp(aonMagic2, "artofnoise by bastian spiegel (twice/lego)", 42) != 0 ||
		(aonMagic1[3] != '4' && aonMagic1[3] != '8'))
		return false;
	return true;
}

bool modAON_t::isAON(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isAON(file);
}

API_Functions AONDecoder =
{
	AON_OpenR,
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
