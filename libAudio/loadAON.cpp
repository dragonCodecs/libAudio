// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2013-2023 Rachel Mant <git@dragonmux.network>
#include "libAudio.h"
#include "genericModule/genericModule.h"
#include "console.hxx"

using substrate::make_unique_nothrow;

namespace libAudio::aon
{
	constexpr static std::array<char, 3> magic1{{'A', 'O', 'N'}};
	constexpr static std::array<char, 42> magic2{{
		'a', 'r', 't', 'o', 'f', 'n', 'o', 'i', 's', 'e', ' ', 'b',
		'y', ' ', 'b', 'a', 's', 't', 'i', 'a', 'n', ' ', 's', 'p',
		'i', 'e', 'g', 'e', 'l', '(', 't', 'w', 'i', 'c', 'e', '/',
		'l', 'e', 'g', 'o', ')'
	}};
}

modAON_t::modAON_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleAON, std::move(fd)} { }

modAON_t *modAON_t::openR(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<modAON_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isAON(file->_fd))
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
	auto remark = ctx.mod->remark();
	if (remark)
		info.other.emplace_back(std::move(remark));
	//info.channels = ctx.mod->channels();

	if (ToPlayback)
	{
		if (!ExternalPlayback)
			file->player(make_unique_nothrow<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}
	return file.release();
}

void *aonOpenR(const char *fileName) { return modAON_t::openR(fileName); }
bool isAON(const char *fileName) { return modAON_t::isAON(fileName); }

bool modAON_t::isAON(const int32_t fd) noexcept
{
	std::array<char, 4> aonMagic1;
	std::array<char, 42> aonMagic2;
	return
		fd != -1 &&
		read(fd, aonMagic1.data(), aonMagic1.size()) == aonMagic1.size() &&
		read(fd, aonMagic2.data(), aonMagic2.size()) == aonMagic2.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		std::equal(libAudio::aon::magic1.begin(), libAudio::aon::magic1.end(), aonMagic1.cbegin()) &&
		(aonMagic1[3] == '4' || aonMagic1[3] == '8') &&
		aonMagic2 == libAudio::aon::magic2;
}

bool modAON_t::isAON(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	return file.valid() && isAON(file);
}
