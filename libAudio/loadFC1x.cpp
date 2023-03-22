// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2013-2023 Rachel Mant <git@dragonmux.network>
#include "libAudio.h"
#include "genericModule/genericModule.h"
#include "console.hxx"

using substrate::make_unique_nothrow;

namespace libAudio::fc1x
{
	constexpr static std::array<char, 4> magicSMOD{{'S', 'M', 'O', 'D'}};
	constexpr static std::array<char, 4> magicFC14{{'F', 'C', '1', '4'}};
}

modFC1x_t::modFC1x_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleFC1x, std::move(fd)} { }

modFC1x_t *modFC1x_t::openR(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<modFC1x_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isFC1x(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	try { ctx.mod = make_unique_nothrow<ModuleFile>(*file); }
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
			file->player(make_unique_nothrow<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192U, info));
		ctx.mod->InitMixer(info);
	}
	return file.release();
}

void *fc1xOpenR(const char *fileName) { return modFC1x_t::openR(fileName); }
bool isFC1x(const char *fileName) { return modFC1x_t::isFC1x(fileName); }

bool modFC1x_t::isFC1x(const int32_t fd) noexcept
{
	std::array<char, 4> fc1xMagic;
	return
		fd != -1 &&
		read(fd, fc1xMagic.data(), fc1xMagic.size()) == fc1xMagic.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		(fc1xMagic == libAudio::fc1x::magicSMOD || fc1xMagic == libAudio::fc1x::magicFC14);
}

bool modFC1x_t::isFC1x(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	return file.valid() && isFC1x(file);
}
