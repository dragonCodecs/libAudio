#include <cstring>
#include "loader.hxx"

sndhLoader_t::sndhLoader_t(const fd_t &file) : _entryPoints{}
{
	std::array<char, 4> sndhSig{};
	if (!file.readBE(_entryPoints.init) ||
		!file.readBE(_entryPoints.exit) ||
		!file.readBE(_entryPoints.play) ||
		!file.read(sndhSig))
		throw std::exception{};
	else if (memcmp(sndhSig.data(), "SNDH", sndhSig.size()) != 0)
		throw std::exception{};
}
