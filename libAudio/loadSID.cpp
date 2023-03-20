// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2012-2023 Rachel Mant <git@dragonmux.network>
#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * Internal structure for holding the decoding context for a given SID file
 */
struct sid_t::decoderContext_t final
{
};

namespace libAudio::sid
{
	constexpr static std::array<char, 4> psidMagic{{'P', 'S', 'I', 'D'}};
}

sid_t::sid_t(fd_t &&fd) noexcept : audioFile_t{audioType_t::sid, std::move(fd)} { }

sid_t *sid_t::openR(const char *const fileName) noexcept
{
	return nullptr;
}

void *sidOpenR(const char *fileName) { return sid_t::openR(fileName); }

int64_t sid_t::fillBuffer(void *const, const uint32_t) { return -1; }

bool isSID(const char *fileName) { return sid_t::isSID(fileName); }

bool sid_t::isSID(const int32_t fd) noexcept
{
	std::array<char, 4> sidMagic;
	return
		fd != -1 &&
		read(fd, sidMagic.data(), sidMagic.size()) == sidMagic.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		sidMagic == libAudio::sid::psidMagic;
}

bool sid_t::isSID(const char *const fileName) noexcept
{
	fd_t file{fileName, O_RDONLY | O_NOCTTY};
	return file.valid() && isSID(file);
}
