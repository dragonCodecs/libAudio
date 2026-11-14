// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <substrate/span>
#include <substrate/index_sequence>
#include "mc68901.hxx"
#include "../unitsHelpers.hxx"
#include "rustAPI.hxx"

extern "C"
{
	MC68901 mc68901_new(uint32_t, motorola68000_t *, uint8_t);
	void mc68901_drop(MC68901);

	void mc68901_read_address(const MC68901 *, uint32_t, slice_t<uint8_t>);
	void mc68901_write_address(MC68901 *, uint32_t, const slice_t<uint8_t>);
	bool mc68901_configure_timer(MC68901 *, size_t, uint8_t, uint8_t);
	bool mc68901_clock_cycle(MC68901 *);
	void mc68901_fire_dma_event(MC68901 *);
	uint8_t mc68901_irq_level(const MC68901 *);
	uint8_t mc68901_irq_cause(MC68901 *);

	void cpu_request_interrupt(motorola68000_t *cpu, uint8_t level)
	{
		cpu->requestInterrupt(level);
	}
}

MC68901IRQRequester *MC68901::irqRequester(motorola68000_t &cpu) noexcept
{
	return new MC68901IRQRequester{cpu, *this};
}

MC68901IRQRequester::MC68901IRQRequester(motorola68000_t &cpu, MC68901 self) noexcept :
	m68k::irqRequester_t{cpu, mc68901_irq_level(&self)}, _self{self} { }

MC68901IRQRequester::~MC68901IRQRequester() noexcept
	{ mc68901_drop(_self); }

uint8_t MC68901IRQRequester::irqCause() noexcept
	{ return mc68901_irq_cause(&_self); }

mc68901_t::mc68901_t(const uint32_t clockFrequency, motorola68000_t &cpu, const uint8_t level) noexcept :
	clockedPeripheral_t<uint32_t>{clockFrequency}, m68k::irqRequester_t{cpu, level},
	self{mc68901_new(clockFrequency, &cpu, level)} { }

mc68901_t::~mc68901_t() noexcept
{
	mc68901_drop(self);
}

void mc68901_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	mc68901_read_address(&self, address, {data.data(), data.size_bytes()});
}

void mc68901_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
	mc68901_write_address(&self, address, {data.data(), data.size_bytes()});
}

void mc68901_t::configureTimer(const size_t timerIndex, const uint8_t reloadValue, const uint8_t mode) noexcept
{
	mc68901_configure_timer(&self, timerIndex, reloadValue, mode);
}

bool mc68901_t::clockCycle() noexcept
{
	return mc68901_clock_cycle(&self);
}

void mc68901_t::fireDMAEvent() noexcept
{
	mc68901_fire_dma_event(&self);
}

uint8_t mc68901_t::irqCause() noexcept
{
	return mc68901_irq_cause(&self);
}
