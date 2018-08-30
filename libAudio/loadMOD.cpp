#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

modMOD_t::modMOD_t() noexcept : modMOD_t(fd_t()) { }
modMOD_t::modMOD_t(fd_t &&fd) noexcept : moduleFile_t(audioType_t::moduleIT, std::move(fd)) { }

void *MOD_OpenR(const char *FileName)
{
	MOD_Intern *const ret = new (std::nothrow) MOD_Intern();
	if (ret == nullptr)
		return nullptr;

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
	info.channels = 2;
	try { ret->p_File = new ModuleFile(ret->inner); }
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
		printf("%s\n", e.error());
		fclose(ret->f_Module);
		delete ret;
		return nullptr;
	}
	info.title = ret->p_File->title();

	if (ToPlayback)
	{
		if (ExternalPlayback == 0)
			ret->inner.player(makeUnique<Playback>(info, MOD_FillBuffer, ret->buffer, 8192, const_cast<MOD_Intern *>(ret)));
		ret->p_File->InitMixer(audioFileInfo(&ret->inner));
	}

	return ret;
}

FileInfo *MOD_GetFileInfo(void *p_MODFile)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	return audioFileInfo(&p_MF->inner);
}

long MOD_FillBuffer(void *p_MODFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	if (p_MF->p_File == NULL)
		return -1;
	return p_MF->p_File->Mix(OutBuffer, nOutBufferLen);
}

int MOD_CloseFileR(void *p_MODFile)
{
	int ret = 0;
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	if (p_MF == NULL)
		return 0;

	delete p_MF->p_File;

	ret = fclose(p_MF->f_Module);
	delete p_MF;
	return ret;
}

void MOD_Play(void *p_MODFile)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	audioPlay(&p_MF->inner);
}

void MOD_Pause(void *p_MODFile)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	audioPause(&p_MF->inner);
}

void MOD_Stop(void *p_MODFile)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	audioStop(&p_MF->inner);
}

bool Is_MOD(const char *FileName) { return modMOD_t::isMOD(FileName); }

bool modMOD_t::isMOD(const int32_t fd) noexcept
{
	constexpr const uint32_t seekOffset = (30 * 31) + 150;
	char MODMagic[4];
	if (fd == -1 ||
		lseek(fd, seekOffset, SEEK_CUR) != seekOffset ||
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
	MOD_GetFileInfo,
	MOD_FillBuffer,
	MOD_CloseFileR,
	MOD_Play,
	MOD_Pause,
	MOD_Stop
};
