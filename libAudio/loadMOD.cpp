#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modMOD_t::modMOD_t(fd_t &&fd) noexcept : moduleFile_t(audioType_t::moduleIT, std::move(fd)) { }

modMOD_t *modMOD_t::openR(const char *const fileName) noexcept
{
	auto modFile = makeUnique<modMOD_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!modFile || !modFile->valid() || !isMOD(modFile->_fd))
		return nullptr;
	lseek(modFile->_fd, 0, SEEK_SET);
	return modFile.release();
}

void *MOD_OpenR(const char *FileName)
{
	std::unique_ptr<modMOD_t> ret{modMOD_t::openR(FileName)};
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

	if (ToPlayback)
	{
		if (ExternalPlayback == 0)
			ret->player(makeUnique<playback_t>(ret.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}

	return ret.release();
}

FileInfo *MOD_GetFileInfo(void *p_MODFile)
	{ return audioFileInfo(p_MODFile); }
long MOD_FillBuffer(void *p_MODFile, uint8_t *OutBuffer, int nOutBufferLen)
	{ return audioFillBuffer(p_MODFile, OutBuffer, nOutBufferLen); }
int MOD_CloseFileR(void *p_MODFile) { return audioCloseFile(p_MODFile); }
void MOD_Play(void *p_MODFile) { audioPlay(p_MODFile); }
void MOD_Pause(void *p_MODFile) { audioPause(p_MODFile); }
void MOD_Stop(void *p_MODFile) { audioStop(p_MODFile); }
bool Is_MOD(const char *FileName) { return modMOD_t::isMOD(FileName); }

bool modMOD_t::isMOD(const int32_t fd) noexcept
{
	constexpr const uint32_t seekOffset = (30 * 31) + 150;
	char MODMagic[4];
	if (fd == -1 ||
		lseek(fd, seekOffset, SEEK_SET) != seekOffset ||
		read(fd, MODMagic, 4) != 4)
		return false;
	else if (strncmp(MODMagic, "M.K.", 4) == 0 ||
		strncmp(MODMagic, "M!K!", 4) == 0 ||
		strncmp(MODMagic, "M&K!", 4) == 0 ||
		strncmp(MODMagic, "N.T.", 4) == 0 ||
		strncmp(MODMagic, "CD81", 4) == 0 ||
		strncmp(MODMagic, "OKTA", 4) == 0 ||
		(strncmp(MODMagic, "FLT", 3) == 0 &&
		MODMagic[3] >= '4' && MODMagic[3] <= '9') ||
		(strncmp(MODMagic + 1, "CHN", 3) == 0 &&
		MODMagic[0] >= '4' && MODMagic[0] <= '9') ||
		(strncmp(MODMagic + 2, "CH", 2) == 0 &&
		(MODMagic[0] == '1' || MODMagic[0] == '2' || MODMagic[0] == '3') &&
		MODMagic[1] >= '0' && MODMagic[1] <= '9') ||
		(strncmp(MODMagic, "TDZ", 3) == 0 &&
		MODMagic[3] >= '4' && MODMagic[3] <= '9') ||
		strncmp(MODMagic, "16CN", 4) == 0 ||
		strncmp(MODMagic, "32CN", 4) == 0)
		return true;
	else // Probbaly not a MOD, but just as likely with the above tests that it is..
		return false; // so we can't do old ProTracker MODs with 15 samples and can't take the auto-detect tests 100% seriously.
}

bool modMOD_t::isMOD(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isMOD(file);
}

API_Functions MODDecoder =
{
	MOD_OpenR,
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
