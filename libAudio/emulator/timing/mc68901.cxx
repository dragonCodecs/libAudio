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
	// Extract how wide an access this is
	const auto accessWidth{data.size_bytes()};
	// Only admit 8- and 16-bit reads
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
	// Extract out the byte to read into
	auto &value
	{
		[&]() -> uint8_t &
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
			value = gpio;
			break;
		case 1U:
			value = activeEdge;
			break;
		case 2U:
			value = dataDirection;
			break;
		case 3U:
			// itrEnableA is the upper half of the enable register
			value = static_cast<uint8_t>(itrEnable >> 8U);
			break;
		case 4U:
			// itrEnableB is the lower half of the enable register
			value = static_cast<uint8_t>(itrEnable);
			break;
		case 5U:
			// itrPendingA is the upper half of the pending register
			value = static_cast<uint8_t>(itrPending >> 8U);
			break;
		case 6U:
			// itrPendingB is the lower half of the pending register
			value = static_cast<uint8_t>(itrPending);
			break;
		case 7U:
			// itrServicingA is the upper half of the servicing register
			value = static_cast<uint8_t>(itrServicing >> 8U);
			break;
		case 8U:
			// itrServicingB is the lower half of the servicing register
			value = static_cast<uint8_t>(itrServicing);
			break;
		case 9U:
			// itrMaskA is the upper half of the mask register
			value = static_cast<uint8_t>(itrMask >> 8U);
			break;
		case 10U:
			// itrMaskB is the lower half of the mask register
			value = static_cast<uint8_t>(itrMask);
			break;
		case 11U:
			value = vectorReg;
			break;
		case 12U:
			value = timers[0].ctrl();
			break;
		case 13U:
			value = timers[1].ctrl();
			break;
		case 14U:
			// Read is from 2 timers, C's control value is in the upper nibble,
			// and D's is in the lower - neither can have more than 3 bits set
			// so just combine
			value = (timers[2].ctrl() << 4U) | timers[3].ctrl();
			break;
		case 15U:
		case 16U:
		case 17U:
		case 18U:
			// Extract the given timer's counter value
			value = timers[(adjustedAddress >> 1U) - 15U].data();
			break;
		case 19U:
			value = syncChar;
			break;
		case 20U:
			value = usartCtrl;
			break;
		case 21U:
			value = rxStatus;
			break;
		case 22U:
			value = txStatus;
			break;
		case 23U:
			value = usartData;
			break;
	}
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
			timers[0].ctrl(value & 0x1fU, clockFrequency());
			break;
		case 13U:
			// Only 5 bits used, mask off the upper 3
			timers[1].ctrl(value & 0x1fU, clockFrequency());
			break;
		case 14U:
			// Write controls 2 timers, C's control value is in the upper nibble,
			// but the timer doesn't support event modes or reset output, so only 3 bits used
			timers[2].ctrl((value >> 4U) & 0x07U, clockFrequency());
			// D's control value is in the lower nibble and has the same support restrictions
			timers[3].ctrl(value & 0x07U, clockFrequency());
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
	// Go through each timer and try to advance them a clock cycle
	for (auto &timer : timers)
		timer.clockCycle();
	return true;
}

namespace mc68901
{
	timer_t::timer_t(const uint32_t baseClockFrequency) noexcept :
		clockManager{baseClockFrequency, baseClockFrequency} { }

	uint8_t timer_t::ctrl() const noexcept { return control; }
	uint8_t timer_t::data() const noexcept { return counter; }

	void timer_t::ctrl(const uint8_t value, const uint32_t baseClockFrequency) noexcept
	{
		control = value;
		// Turn the operation mode into a prescaling value
		const auto prescale
		{
			[&](const uint8_t mode)
			{
				switch (mode)
				{
					case 0U:
						return 1U;
					case 1U:
						return 4U;
					case 2U:
						return 10U;
					case 3U:
						return 16U;
					case 4U:
						return 50U;
					case 5U:
						return 64U;
					case 6U:
						return 100U;
					case 7U:
						return 200U;
				}
				// Should not be possible, but just in case
				return UINT32_MAX;
			}(value & 0x07U)
		};
		// Convert that into a clock ratio for the clock manager, and reinitialise the
		// clock manager accordingly so we get the right generated frequency
		clockManager = {baseClockFrequency, baseClockFrequency / prescale};
	}

	void timer_t::data(const uint8_t value) noexcept
	{
		// Always store the new value as the reload value
		reloadValue = value;
		// If the counter is stopped, also store it as the new counter value
		if ((control & 0x0fU) == 0U)
			counter = value;
	}

	void timer_t::clockCycle() noexcept
	{
		// Check if the timer is stopped
		if ((control & 0x0fU) == 0U)
			return;
		// If it is not, check if this is a clock advancement cycle
		if (!clockManager.advanceCycle())
			return;
		// Check if the timer is in a counting mode
		if ((control & 0x08U) == 0U)
		{
			// Apply a clock pulse to the counterm, and if that counter is now 0, reload it to the value in reloadValue
			if (--counter == 0U)
				counter = reloadValue;
		}
		else
		{
			// For now we don't support the event counting and PWM modes
		}
	}
} // namespace mc68901
