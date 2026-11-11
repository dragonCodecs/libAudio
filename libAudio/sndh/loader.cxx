// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2023 Rachel Mant <git@dragonmux.network>
#include <cstring>
#include <string>
#include <type_traits>
#include "loader.hxx"
#include "../string.hxx"
#include "console.hxx"
#include <substrate/indexed_iterator>

extern "C"
{
	SndhLoader *sndh_loader_new(int32_t fd);
	void sndh_loader_drop(SndhLoader *);

	SndhDecruncher *sndh_loader_decruncher(SndhLoader *);
	sndhEntryPoints_t *sndh_loader_entry_points(SndhLoader *);
	const SndhMetadata *sndh_loader_metadata(SndhLoader *);

	size_t sndh_decruncher_length(const SndhDecruncher *);
	void sndh_decruncher_seek_start(SndhDecruncher *);
	bool sndh_decruncher_read(SndhDecruncher *, slice_t<uint8_t>);

	stringSlice_t sndh_metadata_title(const SndhMetadata *);
	stringSlice_t sndh_metadata_artist(const SndhMetadata *);
	stringSlice_t sndh_metadata_ripper(const SndhMetadata *);
	stringSlice_t sndh_metadata_converter(const SndhMetadata *);
	uint8_t sndh_metadata_tune_count(const SndhMetadata *);
	uint8_t sndh_metadata_default_tune(const SndhMetadata *);
	uint8_t sndh_metadata_timer_frequency(const SndhMetadata *);
	uint8_t sndh_metadata_year(const SndhMetadata *);
	char sndh_metadata_timer(const SndhMetadata *);
	slice_t<uint16_t> sndh_metadata_tune_times(const SndhMetadata *);
	slice_t<uint32_t> sndh_metadata_tune_frame_counts(const SndhMetadata *);
	size_t sndh_metadata_tune_names_count(const SndhMetadata *);
	stringSlice_t sndh_metadata_tune_name(const SndhMetadata *, size_t);

	void console_error(const char *message)
	{
		console.error(message);
	}
}

sndhLoader_t::sndhLoader_t(const fd_t &file)
{
	_loader = sndh_loader_new(file);
	if (_loader == nullptr)
		throw std::exception{};
	_data = sndh_loader_decruncher(_loader);
	_entryPoints = *sndh_loader_entry_points(_loader);
	_metadata = {sndh_loader_metadata(_loader)};
}

sndhLoader_t::~sndhLoader_t()
{
	if (_loader)
		sndh_loader_drop(_loader);
}

size_t SndhDecruncher::length() const
{
	return sndh_decruncher_length(this);
}

bool SndhDecruncher::seek_start()
{
	sndh_decruncher_seek_start(this);
	return true;
}

bool SndhDecruncher::read(substrate::span<uint8_t> span)
{
	slice_t<uint8_t> slice{span.data(), span.size()};
	return sndh_decruncher_read(this, slice);
}

sndhMetadata_t::sndhMetadata_t(const SndhMetadata *metadata)
{
	// Extract all the metadata from the Rust side
	title = sndh_metadata_title(metadata);
	artist = sndh_metadata_artist(metadata);
	ripper = sndh_metadata_ripper(metadata);
	converter = sndh_metadata_converter(metadata);
	tuneCount = sndh_metadata_tune_count(metadata);
	defaultTune = sndh_metadata_default_tune(metadata);
	timerFrequency = sndh_metadata_timer_frequency(metadata);
	year = sndh_metadata_year(metadata);
	timer = sndh_metadata_timer(metadata);
	tuneTimes = sndh_metadata_tune_times(metadata);
	tuneFrameCounts = sndh_metadata_tune_frame_counts(metadata);
	tuneNames = {sndh_metadata_tune_names_count(metadata)};
	for (auto [index, tuneName] : substrate::indexedIterator_t{tuneNames})
		tuneName = sndh_metadata_tune_name(metadata, index);
}

// Defer copying the SNDH data to the Atari ST's RAM to the emulation itself
bool sndhLoader_t::copyToRAM(atariSTe_t &emulator) noexcept
	{ return emulator.copyToRAM(*_data); }
