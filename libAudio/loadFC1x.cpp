// SPDX-License-Identifier: BSD-3-Clause
#include "libAudio.h"
#include "genericModule/genericModule.h"
#include "console.hxx"

namespace libAudio::fc1x
{
	constexpr static std::array<char, 4> magicSMOD{{'S', 'M', 'O', 'D'}};
	constexpr static std::array<char, 4> magicFC14{{'F', 'C', '1', '4'}};
}

modFC1x_t::modFC1x_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleFC1x, std::move(fd)} { }

modFC1x_t *modFC1x_t::openR(const char *const fileName) noexcept
{
	auto file{makeUnique<modFC1x_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isFC1x(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	try { ctx.mod = makeUnique<ModuleFile>(*file); }
	catch (const ModuleLoaderError &e)
	{
		console.error(e.error());
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

void *fc1xOpenR(const char *fileName) { return modFC1x_t::openR(fileName); }
bool isFC1x(const char *fileName) { return modFC1x_t::isFC1x(fileName); }

bool modFC1x_t::isFC1x(const int32_t fd) noexcept
{
	std::array<char, 4> fc1xMagic;
	if (fd == -1 ||
		read(fd, fc1xMagic.data(), fc1xMagic.size()) != fc1xMagic.size() ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		(fc1xMagic != libAudio::fc1x::magicSMOD && fc1xMagic != libAudio::fc1x::magicFC14))
		return false;
	return true;
}

bool modFC1x_t::isFC1x(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isFC1x(file);
}
