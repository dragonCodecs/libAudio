#include <cstring>
#include <type_traits>
#include "loader.hxx"
#include "../conversions.hxx"

using namespace libAudio::conversions;

constexpr std::array<char, 4> typeHeader{'S', 'N', 'D', 'H'};
constexpr std::array<char, 4> typeTitle{'T', 'I', 'T', 'L'};
constexpr std::array<char, 4> typeComposer{'C', 'O', 'M', 'M'};
constexpr std::array<char, 4> typeRipper{'R', 'I', 'P', 'P'};
constexpr std::array<char, 4> typeConverter{'C', 'O', 'N', 'V'};
constexpr std::array<char, 2> typeTuneCount{'#', '#'};
constexpr std::array<char, 2> typeTimerA{'T', 'A'};
constexpr std::array<char, 2> typeTimerB{'T', 'B'};
constexpr std::array<char, 2> typeTimerC{'T', 'C'};
constexpr std::array<char, 2> typeTimerD{'T', 'D'};
constexpr std::array<char, 2> typeTimerVBL{'!', 'V'};
constexpr std::array<char, 4> typeYear{'Y', 'E', 'A', 'R'};
constexpr std::array<char, 4> typeTime{'T', 'I', 'M', 'E'};
constexpr std::array<char, 4> typeEnd{'H', 'D', 'N', 'S'};

template<typename T, size_t sizeA, size_t sizeB, typename = typename std::enable_if<sizeA != sizeB>::type>
	bool operator ==(const std::array<T, sizeA> &a, const std::array<T, sizeB> &b) noexcept
	{ return memcmp(a.data(), b.data(), std::min(sizeA, sizeB)) == 0; }

sndhLoader_t::sndhLoader_t(const fd_t &file) : _entryPoints{}, _metadata{}
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
