// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include "atariSTe.hxx"
#include "timing/mc68901.hxx"
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

	motorola68000_t *cpu_new(MemoryMap *memoryMap)
	{
		return new motorola68000_t{memoryMap_t<uint32_t, 0x00ffffffU>{memoryMap}, static_cast<uint32_t>(8_MHz)};
	}

	void cpu_drop(motorola68000_t *cpu)
	{
		delete cpu;
	}

	void cpu_register_interrupt_requester_mfp(motorola68000_t *cpu, MC68901 mfp)
	{
		cpu->registerInterruptRequester(*mfp.irqRequester(*cpu));
	}

	bool cpu_has_pending_interrupts(const motorola68000_t *cpu)
	{
		return cpu->hasPendingInterrupts();
	}

	bool cpu_trapped(const motorola68000_t *cpu)
	{
		return cpu->trapped();
	}

	void cpu_display_regs(const motorola68000_t *cpu)
	{
		cpu->displayRegs();
	}

	void cpu_write_data_register(motorola68000_t *cpu, size_t reg, uint32_t value)
	{
		cpu->writeDataRegister(reg, value);
	}

	uint32_t cpu_read_program_counter(const motorola68000_t *cpu)
	{
		return cpu->readProgramCounter();
	}

	bool cpu_execute_to_return(motorola68000_t *cpu, uint32_t entryAddress, uint32_t stackTop, bool asUser)
	{
		return cpu->executeToReturn(entryAddress, stackTop, asUser);
	}

	bool cpu_advance_clock(motorola68000_t *cpu)
	{
		return cpu->advanceClock();
	}

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
