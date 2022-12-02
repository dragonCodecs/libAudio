// SPDX-License-Identifier: BSD-3-Clause
#include "libAudio.h"
#include "libAudio.hxx"

namespace libAudio::sid
{
	constexpr static std::array<char, 4> psidMagic{{'P', 'S', 'I', 'D'}};
}

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
