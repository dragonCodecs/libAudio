// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2023 Rachel Mant <git@dragonmux.network>
#include <substrate/utility>
#include "libAudio.h"
#include "libAudio.hxx"
#include "console.hxx"
#include "sndh/loader.hxx"
#include "emulator/atariSTe.hxx"

/*!
 * @internal
 * @file loadSNDH.cpp
 * @brief The implementation of the SNDH decoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2019-2020
 */

using namespace std::literals::string_view_literals;
using substrate::make_unique_nothrow;

struct sndh_t::decoderContext_t final
{
	uint8_t buffer[8192];
	atariSTe_t emulator{};
};

using libAudio::console::asHex_t;

namespace libAudio::sndh
{
	constexpr static std::array<char, 4> icePackMagic{{'I', 'C', 'E', '!'}};
	constexpr static std::array<char, 4> sndhMagic{{'S', 'N', 'D', 'H'}};
}

sndh_t::sndh_t(fd_t &&fd) noexcept : audioFile_t{audioType_t::sndh, std::move(fd)},
	ctx{make_unique_nothrow<decoderContext_t>()} { }

void loadFileInfo(fileInfo_t &info, sndhMetadata_t &metadata) noexcept
{
	info.title(std::move(metadata.title));
	info.artist(std::move(metadata.artist));
}

sndh_t *sndh_t::openR(const char *const fileName) noexcept try
{
	std::unique_ptr<sndh_t> file{make_unique_nothrow<sndh_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isSNDH(file->_fd))
		return nullptr;
	auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();
	sndhLoader_t loader{file->_fd};

	auto &entryPoints = loader.entryPoints();
	console.debug("Read SNDH entry points"sv);
	console.debug(" -> init = "sv, asHex_t<8, '0'>{entryPoints.init}, ", exit = "sv,
		asHex_t<8, '0'>{entryPoints.exit}, ", play = "sv, asHex_t<8, '0'>{entryPoints.play});
	auto &metadata = loader.metadata();
	console.debug("Read SNDH metadata"sv);
	console.debug(" -> title: "sv, metadata.title);
	console.debug(" -> composer: "sv, metadata.artist);
	console.debug(" -> converter: "sv, metadata.converter);
	console.debug(" -> using timer "sv, metadata.timer, " at "sv, metadata.timerFrequency, "Hz"sv);

	// Copy the metadata for this SNDH into the fileInfo_t, and then copy the decrunched SNDH into emulator memory
	loadFileInfo(info, metadata);
	if (!loader.copyToRAM(ctx.emulator) ||
		// Having done this, set up to play the first subtune in the file
		!ctx.emulator.init(0U))
	{
		console.error("Error while setting up emulator for SNDH file"sv);
		return nullptr;
	}
	return file.release();
}
catch (const std::exception &)
{
	console.error("Failed to load SNDH file"sv);
	return nullptr;
}

void *sndhOpenR(const char *fileName) { return sndh_t::openR(fileName); }

int64_t sndh_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	(void)bufferPtr;
	(void)length;
	return -2;
}

bool isSNDH(const char *fileName) { return sndh_t::isSNDH(fileName); }

bool sndh_t::isSNDH(const int32_t fd) noexcept
{
	std::array<char, 4> icePackMagic;
	std::array<char, 4> sndhMagic;
	return
		fd != -1 &&
		static_cast<size_t>(read(fd, icePackMagic.data(), icePackMagic.size())) == icePackMagic.size() &&
		lseek(fd, 8, SEEK_CUR) == 12 &&
		static_cast<size_t>(read(fd, sndhMagic.data(), sndhMagic.size())) == sndhMagic.size() &&
		lseek(fd, 0, SEEK_SET) == 0 &&
		// All packed SNDH files begin with "ICE!" and this is the test
		// that the Linux/Unix Magic Numbers system does too, so
		// it will always work. All unpacked SNDH files start with 'SDNH' at offset 12.
		(icePackMagic == libAudio::sndh::icePackMagic || sndhMagic == libAudio::sndh::sndhMagic);
}

bool sndh_t::isSNDH(const char *const fileName) noexcept
{
	fd_t file{fileName, O_RDONLY | O_NOCTTY};
	return file.valid() && isSNDH(file);
}
