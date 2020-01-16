#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modMOD_t::modMOD_t(fd_t &&fd) noexcept : moduleFile_t(audioType_t::moduleIT, std::move(fd)) { }

modMOD_t *modMOD_t::openR(const char *const fileName) noexcept
{
	auto file = makeUnique<modMOD_t>(fd_t{fileName, O_RDONLY | O_NOCTTY});
	if (!file || !file->valid() || !isMOD(file->_fd) || file->_fd.seek(0, SEEK_SET))
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

	if (ToPlayback)
	{
		if (ExternalPlayback == 0)
			file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}
	return file.release();
}

void *MOD_OpenR(const char *fileName) { return modMOD_t::openR(fileName); }
bool Is_MOD(const char *fileName) { return modMOD_t::isMOD(fileName); }

bool modMOD_t::isMOD(const int32_t fd) noexcept
{
	constexpr const uint32_t seekOffset = (30 * 31) + 150;
	char MODMagic[4];
	if (fd == -1 ||
		lseek(fd, seekOffset, SEEK_SET) != seekOffset ||
		read(fd, MODMagic, 4) != 4 ||
		lseek(fd, 0, SEEK_SET) != 0)
		return false;
	return
		memcmp(MODMagic, "M.K.", 4) == 0 ||
		memcmp(MODMagic, "M!K!", 4) == 0 ||
		memcmp(MODMagic, "M&K!", 4) == 0 ||
		memcmp(MODMagic, "N.T.", 4) == 0 ||
		memcmp(MODMagic, "CD81", 4) == 0 ||
		memcmp(MODMagic, "OKTA", 4) == 0 ||
		(memcmp(MODMagic, "FLT", 3) == 0 &&
		MODMagic[3] >= '4' && MODMagic[3] <= '9') ||
		(memcmp(MODMagic + 1, "CHN", 3) == 0 &&
		MODMagic[0] >= '4' && MODMagic[0] <= '9') ||
		(memcmp(MODMagic + 2, "CH", 2) == 0 &&
		(MODMagic[0] == '1' || MODMagic[0] == '2' || MODMagic[0] == '3') &&
		MODMagic[1] >= '0' && MODMagic[1] <= '9') ||
		(memcmp(MODMagic, "TDZ", 3) == 0 &&
		MODMagic[3] >= '4' && MODMagic[3] <= '9') ||
		memcmp(MODMagic, "16CN", 4) == 0 ||
		memcmp(MODMagic, "32CN", 4) == 0;
	// Probbaly not a MOD, but just as likely with the above tests that it is..
	// so we can't do old ProTracker MODs with 15 samples and can't take the auto-detect tests 100% seriously.
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
