// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_CPU_M68K_IRQ_HXX
#define EMULATOR_CPU_M68K_IRQ_HXX

#include <cstdint>
#include "m68k.hxx"

namespace m68k
{
	struct irqRequester_t
	{
	private:
		motorola68000_t &_cpu;
		uint8_t _level;

	protected:
		irqRequester_t(motorola68000_t &cpu, const uint8_t level) noexcept : _cpu{cpu}, _level{level}
			{ }

		void requestInterrupt() noexcept
			{ _cpu.requestInterrupt(_level); }

	public:
		virtual uint8_t irqCause() noexcept = 0;
	};
}

#endif /**/
