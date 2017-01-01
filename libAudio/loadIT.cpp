#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

modIT_t::modIT_t(/*fd_t &&fd*/) noexcept : moduleFile_t(audioType_t::moduleIT, fd_t()) { }

void *IT_OpenR(const char *FileName)
{
	IT_Intern *const ret = new (std::nothrow) IT_Intern();
	if (ret == nullptr)
		return nullptr;

	ret->inner.fd({FileName, O_RDONLY | O_NOCTTY});
	if (!ret->inner.fd().valid() || !ret->inner.context())
	{
		delete ret;
		return nullptr;
	}
	ret->f_Module = fdopen(ret->inner.fd(), "rb");

	fileInfo_t &info = ret->inner.fileInfo();
	info.bitRate = 44100;
	info.bitsPerSample = 16;
	info.channels = 2;
	try
	{
		ret->inner.context()->mod = makeUnique<ModuleFile>(ret->inner);
	}
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
	info.title.reset(const_cast<char *>(ret->inner.context()->mod->GetTitle()));
	info.artist.reset(const_cast<char *>(ret->inner.context()->mod->GetAuthor()));

	if (ExternalPlayback == 0)
		ret->inner.player(makeUnique<Playback>(info, IT_FillBuffer, ret->buffer, 8192, const_cast<IT_Intern *>(ret)));
	ret->inner.context()->mod->InitMixer(audioFileInfo(&ret->inner));

	return ret;
}

FileInfo *IT_GetFileInfo(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	return audioFileInfo(&p_IF->inner);
}

long IT_FillBuffer(void *p_ITFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	if (!p_IF->inner.context()->mod)
		return -1;
	return p_IF->inner.context()->mod->Mix(OutBuffer, nOutBufferLen);
}

int IT_CloseFileR(void *p_ITFile)
{
	int ret = 0;
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	if (!p_IF)
		return 0;

	ret = fclose(p_IF->f_Module);
	delete p_IF;
	return ret;
}

void IT_Play(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	if (p_IF)
		audioPlay(&p_IF->inner);
}

void IT_Pause(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	if (p_IF)
		audioPause(&p_IF->inner);
}

void IT_Stop(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	if (p_IF)
		audioStop(&p_IF->inner);
}

bool Is_IT(const char *FileName)
{
	FILE *f_IT = fopen(FileName, "rb");
	char ITSig[4];
	if (f_IT == NULL)
		return false;

	fread(ITSig, 4, 1, f_IT);
	fclose(f_IT);

	if (strncmp(ITSig, "IMPM", 4) != 0)
		return false;

	return true;
}

API_Functions ITDecoder =
{
	IT_OpenR,
	IT_GetFileInfo,
	IT_FillBuffer,
	IT_CloseFileR,
	IT_Play,
	IT_Pause,
	IT_Stop
};
