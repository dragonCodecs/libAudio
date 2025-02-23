// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_ATARI_STE_HXX
#define EMULATOR_ATARI_STE_HXX

#include <map>
#include "memoryMap.hxx"
#include "cpu/m68k.hxx"
#include "sound/ym2149.hxx"
#include "timing/mc68901.hxx"
#include "unitsHelpers.hxx"
#include "sndh/iceDecrunch.hxx"

// M68k has a 24-bit address bus, but we can't directly represent that, so use a 32-bit address value instead.
struct atariSTe_t : protected memoryMap_t<uint32_t, 0x00ffffffU>
{
private:
	constexpr static auto systemClockFrequency{32_MHz};
	motorola68000_t cpu{*this, 8_MHz};
	ym2149_t *psg{nullptr};
	mc68901_t *mfp{nullptr};

	uint32_t timeSinceLastCPUCycle{0U};
	std::map<clockedPeripheral_t<uint32_t> *, clockManager_t> clockedPeripherals{};

public:
	constexpr static uint32_t sampleRate{48_kHz};

	atariSTe_t() noexcept;

	[[nodiscard]] bool copyToRAM(sndhDecruncher_t &data) noexcept;
	[[nodiscard]] bool init(uint16_t subtune) noexcept;
	[[nodiscard]] bool exit() noexcept;

	[[nodiscard]] bool advanceClock() noexcept;
	[[nodiscard]] bool sampleReady() const noexcept;
	[[nodiscard]] int16_t readSample() noexcept;

	void displayCPUState() const noexcept;
};

#endif /*EMULATOR_ATARI_STE_HXX*/
