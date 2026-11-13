// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_TIMING_MC68901_HXX
#define EMULATOR_TIMING_MC68901_HXX

#include <cstdint>
#include <array>
#include <substrate/span>
#include "../memoryMap.hxx"
#include "../cpu/m68kIRQ.hxx"

struct MC68901 final
{
	void *inner{nullptr};
};

extern "C"
{
	uint32_t mc68901_timer_prescaling_for(uint8_t);
}

struct mc68901_t final : public clockedPeripheral_t<uint32_t>, m68k::irqRequester_t
{
private:
	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept final;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept final;
	uint8_t irqCause() noexcept final;

	MC68901 self{};

public:
	mc68901_t(uint32_t clockFrequency, motorola68000_t &cpu, const uint8_t level) noexcept;
	mc68901_t(const mc68901_t &) noexcept = delete;
	mc68901_t(mc68901_t &&) noexcept = delete;
	mc68901_t &operator =(const mc68901_t &) noexcept = delete;
	mc68901_t &operator =(mc68901_t &&) noexcept = delete;
	~mc68901_t() noexcept final;

	[[nodiscard]] bool clockCycle() noexcept final;
	void fireDMAEvent() noexcept;

	void configureTimer(size_t timerIndex, uint8_t reloadValue, uint8_t mode) noexcept;

	MC68901 inner() noexcept { return self; }
};

#endif /*EMULATOR_TIMING_MC68901_HXX*/
