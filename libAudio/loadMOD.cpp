// SPDX-License-Identifier: BSD-3-Clause
#include <string_view>

#include "libAudio.h"
#include "genericModule/genericModule.h"
#include "console.hxx"

using namespace std::literals::string_view_literals;

namespace libAudio::mod
{
	constexpr static std::array<char, 4> modMagicMKOrig{{'M', '.', 'K', '.'}};
	constexpr static std::array<char, 4> modMagicMKMorePatternsPT2_3{{'M', '!', 'K', '!'}};
	constexpr static std::array<char, 4> modMagicMKAlt{{'M', '&', 'K', '!'}};
	constexpr static std::array<char, 4> modMagicNewTracker{{'N', '.', 'T', '.'}};
	constexpr static std::array<char, 4> modMagicCD81{{'C', 'D', '8', '1'}};
	constexpr static std::array<char, 4> modMagicOktamed{{'O', 'K', 'T', 'A'}};
	constexpr static std::array<char, 3> modMagicStartrekker{{'F', 'L', 'T'}};
	constexpr static std::array<char, 3> modMagicChn{{'C', 'H', 'N'}};
	constexpr static std::array<char, 2> modMagicCh{{'C', 'H'}};
	constexpr static std::array<char, 3> modMagicTDZ{{'T', 'D', 'Z'}};
	constexpr static std::array<char, 4> modMagic16Channel{{'1', '6', 'C', 'N'}};
	constexpr static std::array<char, 4> modMagic32Channel{{'3', '2', 'C', 'N'}};
} // namespace libAudio::mod

modMOD_t::modMOD_t(fd_t &&fd) noexcept : moduleFile_t(audioType_t::moduleIT, std::move(fd)) { }

modMOD_t *modMOD_t::openR(const char *const fileName) noexcept
{
	auto file{makeUnique<modMOD_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isMOD(file->_fd) || file->_fd.seek(0, SEEK_SET))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();

	info.bitRate = 44100;
	info.bitsPerSample = 16;
	info.channels = 2;
	try { ctx.mod = makeUnique<ModuleFile>(*file); }
	catch (const ModuleLoaderError &e)
	{
		console.error(e.error());
		return nullptr;
	}
	info.title = ctx.mod->title();

	if (ToPlayback)
	{
		if (ExternalPlayback == 0)
			file->player(makeUnique<playback_t>(file.get(), audioFillBuffer, ctx.playbackBuffer, 8192, info));
		ctx.mod->InitMixer(info);
	}
	return file.release();
}

void *modOpenR(const char *fileName) { return modMOD_t::openR(fileName); }
bool isMOD(const char *fileName) { return modMOD_t::isMOD(fileName); }

bool modMOD_t::isMOD(const int32_t fd) noexcept
{
	constexpr const uint32_t seekOffset = (30 * 31) + 150;
	std::array<char, 4> modMagic;
	if (fd == -1 ||
		lseek(fd, seekOffset, SEEK_SET) != seekOffset ||
		read(fd, modMagic.data(), modMagic.size()) != modMagic.size() ||
		lseek(fd, 0, SEEK_SET) != 0)
		return false;
	return
		modMagic == libAudio::mod::modMagicMKOrig ||
		modMagic == libAudio::mod::modMagicMKMorePatternsPT2_3 ||
		modMagic == libAudio::mod::modMagicMKAlt ||
		modMagic == libAudio::mod::modMagicNewTracker ||
		modMagic == libAudio::mod::modMagicCD81 ||
		modMagic == libAudio::mod::modMagicOktamed ||
		(
			std::equal(libAudio::mod::modMagicStartrekker.begin(), libAudio::mod::modMagicStartrekker.end(),
				modMagic.cbegin()) && modMagic[3] >= '4' && modMagic[3] <= '9'
		) ||
		(
			std::equal(libAudio::mod::modMagicChn.begin(), libAudio::mod::modMagicChn.end(), modMagic.cbegin() + 1) &&
			modMagic[0] >= '4' && modMagic[0] <= '9'
		) ||
		(
			std::equal(libAudio::mod::modMagicCh.begin(), libAudio::mod::modMagicCh.end(), modMagic.cbegin() + 2) &&
			modMagic[0] >= '1' && modMagic[0] <= '3' && modMagic[1] >= '0' && modMagic[1] <= '9'
		) ||
		(
			std::equal(libAudio::mod::modMagicTDZ.begin(), libAudio::mod::modMagicTDZ.end(), modMagic.cbegin()) &&
			modMagic[3] >= '4' && modMagic[3] <= '9'
		) ||
		modMagic == libAudio::mod::modMagic16Channel ||
		modMagic == libAudio::mod::modMagic32Channel;
	// Probbaly not a MOD, but just as likely with the above tests that it is..
	// so we can't do old ProTracker MODs with 15 samples and can't take the auto-detect tests 100% seriously.
}

bool modMOD_t::isMOD(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	return file.valid() && isMOD(file);
}
