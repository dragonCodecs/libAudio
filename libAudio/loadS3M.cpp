// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2012-2023 Rachel Mant <git@dragonmux.network>
#include "libAudio.h"
#include "genericModule/genericModule.h"
#include "console.hxx"

using substrate::make_unique_nothrow;

namespace libAudio::s3m
{
	constexpr static char s3mMagic1{'\x1A'};
	constexpr static std::array<char, 4> s3mMagic2{{'S', 'C', 'R', 'M'}};
}

modS3M_t::modS3M_t(fd_t &&fd) noexcept : moduleFile_t{audioType_t::moduleS3M, std::move(fd)} { }

modS3M_t *modS3M_t::openR(const char *const fileName) noexcept
{
	auto file{make_unique_nothrow<modS3M_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isS3M(file->_fd))
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

void *s3mOpenR(const char *fileName) { return modS3M_t::openR(fileName); }
bool isS3M(const char *fileName) { return modS3M_t::isS3M(fileName); }

bool modS3M_t::isS3M(const int32_t fd) noexcept
{
	constexpr static size_t seekOffset1 = 28;
	constexpr static size_t seekOffset2 = seekOffset1 + 16;
	char s3mMagic1;
	std::array<char, 4> s3mMagic2;
	return
		fd != -1 &&
		lseek(fd, seekOffset1, SEEK_SET) == seekOffset1 &&
		read(fd, &s3mMagic1, 1) == 1 &&
		lseek(fd, seekOffset2, SEEK_SET) == seekOffset2 &&
		read(fd, s3mMagic2.data(), s3mMagic2.size()) == s3mMagic2.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		s3mMagic1 == libAudio::s3m::s3mMagic1 &&
		s3mMagic2 == libAudio::s3m::s3mMagic2;
}

bool modS3M_t::isS3M(const char *const fileName) noexcept
{
	fd_t file{fileName, O_RDONLY | O_NOCTTY};
	return file.valid() && isS3M(file);
}
