// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_ATARI_STE_HXX
#define EMULATOR_ATARI_STE_HXX

#include "memoryMap.hxx"
#include "cpu/m68k.hxx"
#include "unitsHelpers.hxx"

struct SndhDecruncher;
struct AtariSTe;

// M68k has a 24-bit address bus, but we can't directly represent that, so use a 32-bit address value instead.
struct atariSTe_t
{
private:
	AtariSTe *self{nullptr};

public:
	constexpr static auto sampleRate{static_cast<uint32_t>(48_kHz)};

	atariSTe_t() noexcept;
	~atariSTe_t() noexcept;

	void configureTimer(char timer, uint16_t timerFrequency) noexcept;
	[[nodiscard]] bool copyToRAM(SndhDecruncher &data) noexcept;
	[[nodiscard]] bool init(uint16_t subtune) noexcept;
	[[nodiscard]] bool exit() noexcept;

	[[nodiscard]] bool advanceClock() noexcept;
	[[nodiscard]] bool sampleReady() const noexcept;
	[[nodiscard]] int16_t readSample() noexcept;

	void displayCPUState() const noexcept;
};

#endif /*EMULATOR_ATARI_STE_HXX*/
