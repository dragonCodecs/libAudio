// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include "atariSTe.hxx"
#include "unitsHelpers.hxx"
#include "sndh/loader.hxx"
#include "console.hxx"

using namespace libAudio::console;

extern "C"
{
	AtariSTe *atari_ste_new();
	void atari_ste_drop(AtariSTe *);

	void atari_ste_configure_timer(AtariSTe *, char, uint16_t);
	bool atari_ste_copy_to_ram(AtariSTe *, SndhDecruncher *);
	bool atari_ste_init(AtariSTe *, uint16_t);
	bool atari_ste_exit(AtariSTe *);
	bool atari_ste_advance_clock(AtariSTe *);
	bool atari_ste_sample_ready(const AtariSTe *);
	int16_t atari_ste_read_sample(AtariSTe *);
	void atari_ste_display_cpu_state(const AtariSTe *);

	void console_debug(const char *message)
	{
		console.debug(message);
	}
}

atariSTe_t::atariSTe_t() noexcept : self{atari_ste_new()} { }
atariSTe_t::~atariSTe_t() noexcept
	{ atari_ste_drop(self); }

void atariSTe_t::configureTimer(const char timer, const uint16_t timerFrequency) noexcept
	{ atari_ste_configure_timer(self, timer, timerFrequency); }

// Copy the contents of a decrunched SNDH into the ST's RAM
bool atariSTe_t::copyToRAM(SndhDecruncher &data) noexcept
	{ return atari_ste_copy_to_ram(self, &data); }

bool atariSTe_t::init(const uint16_t subtune) noexcept
	{ return atari_ste_init(self, subtune); }

bool atariSTe_t::exit() noexcept
	{ return atari_ste_exit(self); }

bool atariSTe_t::advanceClock() noexcept
	{ return atari_ste_advance_clock(self); }

bool atariSTe_t::sampleReady() const noexcept
	{ return atari_ste_sample_ready(self); }

int16_t atariSTe_t::readSample() noexcept
	{ return atari_ste_read_sample(self); }

void atariSTe_t::displayCPUState() const noexcept
	{ atari_ste_display_cpu_state(self); }
