// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <substrate/span>
#include "mc68901.hxx"
#include "../unitsHelpers.hxx"

mc68901_t::mc68901_t(const uint32_t clockFrequency) noexcept : clockedPeripheral_t<uint32_t>{clockFrequency},
	timers{{{clockFrequency}, {clockFrequency}, {clockFrequency}, {clockFrequency}}} { }

void mc68901_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
}

void mc68901_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
	// Extract how wide an access this is
	const auto accessWidth{data.size_bytes()};
	// Only admit 8- and 16-bit writes
	if (accessWidth > 2U)
		return;
	// Adjust the address to take into account u16 vs u8 accesses to odd bytes
	const auto adjustedAddress
	{
		[&]()
		{
			if (accessWidth == 1U)
				return address;
			return address + 1U;
		}()
	};
	// If the resulting address is not odd, we're done
	if ((adjustedAddress & 1U) == 0U)
		return;
	// Extract out the byte from the write to use
	const auto value
	{
		[&]()
		{
			if (accessWidth == 1U)
				return data[0U];
			return data[1U];
		}()
	};
	// Chop off the now unused bit of the address to make selection easier
	switch (adjustedAddress >> 1U)
	{
		case 0U:
			gpio = value;
			break;
		case 1U:
			activeEdge = value;
			break;
		case 2U:
			dataDirection = value;
			break;
		case 3U:
			// itrEnableA is the upper half of the enable register
			itrEnable &= 0x00ffU;
			itrEnable |= value << 8U;
			break;
		case 4U:
			// itrEnableB is the lower half of the enable register
			itrEnable &= 0xff00U;
			itrEnable |= value;
			break;
		case 5U:
			// itrPendingA is the upper half of the pending register
			itrPending &= 0x00ffU;
			itrPending |= value << 8U;
			break;
		case 6U:
			// itrPendingB is the lower half of the pending register
			itrPending &= 0xff00U;
			itrPending |= value;
			break;
		case 7U:
			// itrServicingA is the upper half of the servicing register
			itrServicing &= 0x00ffU;
			itrServicing |= value << 8U;
			break;
		case 8U:
			// itrServicingB is the lower half of the servicing register
			itrServicing &= 0xff00U;
			itrServicing |= value;
			break;
		case 9U:
			// itrMaskA is the upper half of the mask register
			itrMask &= 0x00ffU;
			itrMask |= value << 8U;
			break;
		case 10U:
			// itrMaskB is the lower half of the mask register
			itrMask &= 0xff00U;
			itrMask |= value;
			break;
		case 11U:
			// The bottom 3 bits of the vector register are unused (RAZ)
			vectorReg = value & 0xf8U;
			break;
		case 12U:
			// Only 5 bits used, mask off the upper 3
			timers[0].ctrl(value & 0x1fU);
			break;
		case 13U:
			// Only 5 bits used, mask off the upper 3
			timers[1].ctrl(value & 0x1fU);
			break;
		case 14U:
			// Write controls 2 timers, C's control value is in the upper nibble,
			// but the timer doesn't support event modes or reset output, so only 3 bits used
			timers[2].ctrl((value >> 4U) & 0x07U);
			// D's control value is in the lower nibble and has the same support restrictions
			timers[3].ctrl(value & 0x07U);
			break;
		case 15U:
		case 16U:
		case 17U:
		case 18U:
			// Update the given timer's counter value
			timers[(adjustedAddress >> 1U) - 15U].data(value);
			break;
		case 19U:
			syncChar = value;
			break;
		case 20U:
			usartCtrl = value;
			break;
		case 21U:
			rxStatus = value;
			break;
		case 22U:
			txStatus = value;
			break;
		case 23U:
			usartData = value;
			break;
	}
}

bool mc68901_t::clockCycle() noexcept
{
	return true;
}

namespace mc68901
{
	timer_t::timer_t(const uint32_t baseClockFrequency) noexcept :
		clockManager{baseClockFrequency, baseClockFrequency} { }

	uint8_t timer_t::ctrl() const noexcept { return control; }
	uint8_t timer_t::data() const noexcept { return counter; }

	void timer_t::ctrl(const uint8_t value) noexcept
		{ control = value; }

	void timer_t::data(const uint8_t value) noexcept
		{ counter = value; }
} // namespace mc68901
