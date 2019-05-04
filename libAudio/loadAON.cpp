#include <stdio.h>
#include <malloc.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modAON_t::modAON_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleAON, std::move(fd)} { }

modAON_t *modAON_t::openR(const char *const fileName) noexcept
{
	auto aonFile = makeUnique<modAON_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!aonFile || !aonFile->valid() || !isAON(aonFile->_fd))
		return nullptr;
	if (!aonFile->_fd.seek(0, SEEK_SET) != 0)
		return nullptr;
	return aonFile.release();
}

void *AON_OpenR(const char *FileName)
{
	std::unique_ptr<modAON_t> ret{modAON_t::openR(FileName)};
	if (!ret)
		return nullptr;

	auto &ctx = *ret->context();
	fileInfo_t &info = ret->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	info.channels = 2;
	try { ctx.mod = makeUnique<ModuleFile>(*ret); }
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
			ret->player(makeUnique<playback_t>(ret.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}

	return ret.release();
}

const fileInfo_t *AON_GetFileInfo(void *p_AONFile) { return audioFileInfo(p_AONFile); }
long AON_FillBuffer(void *p_AONFile, void *const buffer, const uint32_t length)
	{ return audioFillBuffer(p_AONFile, buffer, length); }
int AON_CloseFileR(void *p_AONFile) { return audioCloseFile(p_AONFile); }
void AON_Play(void *p_AONFile) { audioPlay(p_AONFile); }
void AON_Pause(void *p_AONFile) { audioPause(p_AONFile); }
void AON_Stop(void *p_AONFile) { audioStop(p_AONFile); }
bool Is_AON(const char *FileName) { return modAON_t::isAON(FileName); }

bool modAON_t::isAON(const int32_t fd) noexcept
{
	char aonMagic1[4], aonMagic2[42];
	if (fd == -1 ||
		read(fd, aonMagic1, 4) != 4 ||
		read(fd, aonMagic2, 42) != 42 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		strncmp(aonMagic1, "AON", 3) != 0 ||
		strncmp(aonMagic2, "artofnoise by bastian spiegel (twice/lego)", 42) != 0 ||
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
