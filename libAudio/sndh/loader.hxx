// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2023 Rachel Mant <git@dragonmux.network>
#ifndef SNDH_LOADER_HXX
#define SNDH_LOADER_HXX

#include <cstdint>
#include <memory>
#include <vector>
#include <substrate/fd>
#include <substrate/fixed_vector>
#include "iceDecrunch.hxx"

using substrate::fd_t;
using substrate::fixedVector_t;

struct sndhEntryPoints_t final
{
	uint32_t init;
	uint32_t exit;
	uint32_t play;
};

struct sndhMetadata_t final
{
	std::unique_ptr<char []> title;
	std::unique_ptr<char []> artist;
	std::unique_ptr<char []> ripper;
	std::unique_ptr<char []> converter;
	uint8_t tuneCount;
	char timer; // A, B, C, D or V(BL).
	uint16_t timerFrequency;
	uint32_t year;
	fixedVector_t<uint16_t> tuneTimes;
};

struct sndhLoader_t
{
private:
	sndhDecruncher_t _data;
	sndhEntryPoints_t _entryPoints;
	sndhMetadata_t _metadata;

	bool readMeta();

public:
	sndhLoader_t(const fd_t &file);
	const sndhEntryPoints_t &entryPoints() const noexcept { return _entryPoints; }
	sndhMetadata_t &metadata() noexcept { return _metadata; }
	const sndhMetadata_t &metadata() const noexcept { return _metadata; }
};

#endif /*SNDH_LOADER_HXX*/
