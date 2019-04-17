#include <stdio.h>
#include <malloc.h>

#include "libAudio.h"
#include "genericModule/genericModule.h"

modAON_t::modAON_t() noexcept : moduleFile_t{audioType_t::moduleAON, {}} { }

void *AON_OpenR(const char *FileName)
{
	std::unique_ptr<AON_Intern> ret = makeUnique<AON_Intern>();
	if (!ret)
		return nullptr;

	auto &ctx = *ret->inner.context();
	fileInfo_t &info = ret->inner.fileInfo();

	ret->f_Module = fopen(FileName, "rb");
	if (!ret->f_Module)
		return nullptr;

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	info.channels = 2;
	try { ctx.mod = makeUnique<ModuleFile>(ret.get()); }
	catch (const ModuleLoaderError &e)
	{
		printf("%s\n", e.error());
		return nullptr;
	}
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->error());
		delete e;
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
		if (ExternalPlayback == 0)
			ret->inner.player(makeUnique<playback_t>(ret.get(), AON_FillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(AON_GetFileInfo(ret.get()));
	}

	return ret.release();
}

FileInfo *AON_GetFileInfo(void *p_AONFile)
{
	AON_Intern *p_AF = (AON_Intern *)p_AONFile;
	return audioFileInfo(&p_AF->inner);
}

long AON_FillBuffer(void *p_AONFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	int32_t ret = 0, Read;
	AON_Intern *p_AF = (AON_Intern *)p_AONFile;
	if (p_AF->p_File == NULL)
		return -1;
	do
	{
		Read = p_AF->p_File->Mix(p_AF->buffer, nOutBufferLen - ret);
		if (Read >= 0 && OutBuffer != p_AF->buffer)
			memcpy(OutBuffer + ret, p_AF->buffer, Read);
		if (Read >= 0)
			ret += Read;
	}
	while (ret < nOutBufferLen && Read >= 0);
	return (ret == 0 ? Read : ret);
}

int AON_CloseFileR(void *p_AONFile)
{
	AON_Intern *p_AF = (AON_Intern *)p_AONFile;
	if (p_AF == NULL)
		return 0;
	const int ret = fclose(p_AF->f_Module);
	delete p_AF;
	return ret;
}

void AON_Play(void *p_AONFile)
{
	AON_Intern *p_AF = (AON_Intern *)p_AONFile;
	p_AF->inner.play();
}

void AON_Pause(void *p_AONFile)
{
	AON_Intern *p_AF = (AON_Intern *)p_AONFile;
	p_AF->inner.pause();
}

void AON_Stop(void *p_AONFile)
{
	AON_Intern *p_AF = (AON_Intern *)p_AONFile;
	p_AF->inner.stop();
}

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
	AON_GetFileInfo,
	AON_FillBuffer,
	AON_CloseFileR,
	AON_Play,
	AON_Pause,
	AON_Stop
};
