// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2012-2023 Rachel Mant <git@dragonmux.network>
#include "libAudio.h"
#include "genericModule/genericModule.h"
#include "console.hxx"

using substrate::make_unique_nothrow;

namespace libAudio::stm
{
	constexpr static std::array<char, 9> magic{{'!', 'S', 'c', 'r', 'e', 'a', 'm', '!', '\x1A'}};
}

modSTM_t::modSTM_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleSTM, std::move(fd)} { }

modSTM_t *modSTM_t::openR(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<modSTM_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isSTM(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	info.bitRate(44100U);
	info.bitsPerSample(16U);
	info.channels(2U);
	try { ctx.mod = make_unique_nothrow<ModuleFile>(*file); }
	catch (const ModuleLoaderError &e)
	{
		console.error(e.error());
		return nullptr;
	}
	info.title = ctx.mod->title();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			file->player(make_unique_nothrow<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192U, info));
		ctx.mod->InitMixer(info);
	}
	return file.release();
}

void *stmOpenR(const char *fileName) { return modSTM_t::openR(fileName); }
bool isSTM(const char *fileName) { return modSTM_t::isSTM(fileName); }

bool modSTM_t::isSTM(const int32_t fd) noexcept
{
	constexpr uint32_t offset = 20;
	std::array<char, 9> stmMagic;
	return
		fd != -1 &&
		lseek(fd, offset, SEEK_SET) == offset &&
		read(fd, stmMagic.data(), stmMagic.size()) == stmMagic.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		stmMagic == libAudio::stm::magic;
}

bool modSTM_t::isSTM(const char *const fileName) noexcept
{
	fd_t file{fileName, O_RDONLY | O_NOCTTY};
	return file.valid() && isSTM(file);
}
