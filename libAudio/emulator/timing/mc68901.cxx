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
	MC68901 *mc68901_new(const mc68901_t *, uint32_t);
	void mc68901_drop(MC68901 *);

	void mc68901_read_address(const MC68901 *, uint32_t, slice_t<uint8_t>);
	void mc68901_write_address(MC68901 *, uint32_t, const slice_t<uint8_t>);
	bool mc68901_configure_timer(MC68901 *, size_t, uint8_t, uint8_t);
	bool mc68901_clock_cycle(MC68901 *);
	void mc68901_fire_dma_event(MC68901 *);
	uint8_t mc68901_irq_cause(MC68901 *);

	void mc68901_request_interrupt(mc68901_t *self)
	{
		self->requestCPUInterrupt();
	}
}

mc68901_t::mc68901_t(const uint32_t clockFrequency, motorola68000_t &cpu, const uint8_t level) noexcept :
	clockedPeripheral_t<uint32_t>{clockFrequency}, m68k::irqRequester_t{cpu, level},
	self{mc68901_new(this, clockFrequency)} { }

mc68901_t::~mc68901_t() noexcept
{
	if (self)
		mc68901_drop(self);
}

void mc68901_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	mc68901_read_address(self, address, {data.data(), data.size_bytes()});
}

void mc68901_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
	mc68901_write_address(self, address, {data.data(), data.size_bytes()});
}

void mc68901_t::configureTimer(const size_t timerIndex, const uint8_t reloadValue, const uint8_t mode) noexcept
{
	mc68901_configure_timer(self, timerIndex, reloadValue, mode);
}

bool mc68901_t::clockCycle() noexcept
{
	return mc68901_clock_cycle(self);
}

void mc68901_t::requestCPUInterrupt() noexcept
{
	requestInterrupt();
}

uint8_t mc68901_t::irqCause() noexcept
{
	return mc68901_irq_cause(self);
}
