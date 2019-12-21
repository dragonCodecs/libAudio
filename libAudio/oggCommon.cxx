#include <array>
#include <string.h>

#include "fd.hxx"
#include "oggCommon.hxx"

bool isOgg(const int32_t fd, ogg_packet &headerPacket) noexcept
{
	std::array<char, 58> header{};
	if (fd == -1 ||
		read(fd, header.data(), header.size()) != header.size() ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		memcmp(header.data(), "Oggs", 4) != 0)
		return false;
	return true;
}

bool isVorbis(const ogg_packet &headerPacket) noexcept
{
	return false;
}

bool isOpus(const ogg_packet &headerPacket) noexcept
{
	return false;
}
