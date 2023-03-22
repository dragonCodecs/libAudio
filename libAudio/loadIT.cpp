// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2009-2023 Rachel Mant <git@dragonmux.network>
#include "libAudio.h"
#include "genericModule/genericModule.h"
#include "console.hxx"

using substrate::make_unique_nothrow;

namespace libAudio::it
{
	constexpr static std::array<char, 4> magic{{'I', 'M', 'P', 'M'}};
}

modIT_t::modIT_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleIT, std::move(fd)} { }

modIT_t *modIT_t::openR(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<modIT_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isIT(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	info.channels = 2;
	try { ctx.mod = make_unique_nothrow<ModuleFile>(*file); }
	catch (const ModuleLoaderError &e)
	{
		console.error(e.error());
		return nullptr;
	}
	info.title = ctx.mod->title();
	info.artist = ctx.mod->author();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			file->player(make_unique_nothrow<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192U, info));
		ctx.mod->InitMixer(info);
	}
	return file.release();
}

void *itOpenR(const char *fileName) { return modIT_t::openR(fileName); }
bool isIT(const char *fileName) { return modIT_t::isIT(fileName); }

bool modIT_t::isIT(const int32_t fd) noexcept
{
	std::array<char, 4> itMagic;
	return
		fd != -1 &&
		read(fd, itMagic.data(), itMagic.size()) == itMagic.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		itMagic == libAudio::it::magic;
}

bool modIT_t::isIT(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	return file.valid() && isIT(file);
}
