// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2023 Rachel Mant <git@dragonmux.network>
#ifndef SNDH_LOADER_HXX
#define SNDH_LOADER_HXX

#include <cstdint>
#include <substrate/span>
#include <substrate/fd>
#include <substrate/fixed_vector>
#include "emulator/atariSTe.hxx"
#include "rustAPI.hxx"

using substrate::fd_t;
using substrate::fixedVector_t;

struct SndhLoader;
struct SndhMetadata;

struct sndhMetadata_t final
{
	stringSlice_t title;
	stringSlice_t artist;
	stringSlice_t ripper;
	stringSlice_t converter;
	uint8_t tuneCount;
	uint8_t defaultTune{1U};
	uint16_t timerFrequency;
	uint32_t year;
	char timer; // A, B, C, D or V(BL).
	slice_t<uint16_t> tuneTimes;
	slice_t<uint32_t> tuneFrameCounts;
	fixedVector_t<stringSlice_t> tuneNames;

	sndhMetadata_t() = default;
	sndhMetadata_t(const SndhMetadata *metadata);
};

struct sndhLoader_t final
{
private:
	SndhLoader *_loader{nullptr};
	SndhDecruncher *_data{nullptr};
	sndhMetadata_t _metadata{};

public:
	sndhLoader_t(const fd_t &file);
	~sndhLoader_t();
	[[nodiscard]] const sndhMetadata_t &metadata() const noexcept { return _metadata; }
	[[nodiscard]] bool copyToRAM(atariSTe_t &emulator) noexcept;

	// NB: for test use only
	[[nodiscard]] SndhLoader *loader() const noexcept { return _loader; }
};

#endif /*SNDH_LOADER_HXX*/
