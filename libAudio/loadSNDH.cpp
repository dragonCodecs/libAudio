// SPDX-License-Identifier: BSD-3-Clause
#include "libAudio.h"
#include "libAudio.hxx"
#include "console.hxx"
#include "sndh/loader.hxx"

/*!
 * @internal
 * @file loadSNDH.cpp
 * @brief The implementation of the SNDH decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2019-2020
 */

struct sndh_t::decoderContext_t final
{
	uint8_t buffer[8192];
};

using libAudio::console::asHex_t;
using libAudio::console::operator ""_s;

sndh_t::sndh_t(fd_t &&fd) noexcept : audioFile_t{audioType_t::sndh, std::move(fd)}, ctx{makeUnique<decoderContext_t>()} { }

void loadFileInfo(fileInfo_t &info, sndhMetadata_t &metadata) noexcept
{
	info.title.swap(metadata.title);
	info.artist.swap(metadata.artist);
}

sndh_t *sndh_t::openR(const char *const fileName) noexcept try
{
	std::unique_ptr<sndh_t> file{makeUnique<sndh_t>(fd_t{fileName, O_RDONLY | O_NOCTTY})};
	if (!file || !file->valid() || !isSNDH(file->_fd))
		return nullptr;
	//auto &ctx = *file->context();
	fileInfo_t &info = file->fileInfo();
	sndhLoader_t loader{file->_fd}; //, file->context()

	auto &entryPoints = loader.entryPoints();
	console.debug("Read SNDH entry points"_s);
	console.debug(" -> init = "_s, asHex_t<8, '0'>{entryPoints.init}, ", exit = "_s,
		asHex_t<8, '0'>{entryPoints.exit}, ", play = "_s, asHex_t<8, '0'>{entryPoints.play});
	auto &metadata = loader.metadata();
	console.debug("Read SNDH metadata"_s);
	console.debug(" -> title: "_s, metadata.title);
	console.debug(" -> composer: "_s, metadata.artist);
	console.debug(" -> converter: "_s, metadata.converter);
	console.debug(" -> using timer "_s, metadata.timer, " at "_s, metadata.timerFrequency, "Hz"_s);

	loadFileInfo(info, metadata);
	return file.release();
}
catch (const std::exception &)
{
	console.error("Failed to load SNDH file"_s);
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
	char icePackSig[4], sndhSig[4];
	if (fd == -1 ||
		read(fd, icePackSig, 4) != 4 ||
		lseek(fd, 8, SEEK_CUR) != 12 ||
		read(fd, sndhSig, 4) != 4 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		// All packed SNDH files begin with "ICE!" and this is the test
		// that the Linux/Unix Magic Numbers system does too, so
		// it will always work. All unpacked SNDH files start with 'SDNH' at offset 12.
		!(//memcmp(icePackSig, "ICE!", 4) == 0 ||
		memcmp(sndhSig, "SNDH", 4) == 0))
		return false;
	return true;
}

bool sndh_t::isSNDH(const char *const fileName) noexcept
{
	fd_t file{fileName, O_RDONLY | O_NOCTTY};
	if (!file.valid())
		return false;
	return isSNDH(file);
}
