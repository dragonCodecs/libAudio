// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <substrate/span>
#include <substrate/index_sequence>
#include "mc68901.hxx"
#include "../unitsHelpers.hxx"

mc68901_t::mc68901_t(const uint32_t clockFrequency, motorola68000_t &cpu, const uint8_t level) noexcept :
	clockedPeripheral_t<uint32_t>{clockFrequency}, m68k::irqRequester_t{cpu, level},
	timers{{{clockFrequency}, {clockFrequency}, {clockFrequency}, {clockFrequency}}}
{
	// Timer C interrupts should be enabled and unmasked by default
	itrEnable |= 1U << 5U;
	itrMask |= 1U << 5U;
}

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
	for (const auto idx : substrate::indexSequence_t{timers.size()})
	{
		auto &timer{timers[idx]};
		// Translate the timer number into an interrupt register bit
		const auto itrBit
		{
			1U << [&]()
			{
				// Dispatch the timer to the bit number for it
				switch (idx)
				{
					case 0U:
						return 13U;
					case 1U:
						return 8U;
					case 2U:
						return 5U;
					case 3U:
						return 4U;
				}
				// This should never happen, but just in case.. this selects one past the end
				// of the itr registers so it's a safe no-op
				return 16U;
			}()
		};
		// If the clock cycle on the timer triggered an interrupt causing event
		if (timer.clockCycle())
		{
			// If interrupts are enabled for the timer, set the pending bit
			if (itrEnable & itrBit)
				itrPending |= itrBit;
		}
	}
	// If we have any pending IRQs as a result of this cycle, request handling by the CPU
	if (itrPending & itrMask)
		requestInterrupt();
	return true;
}

void mc68901_t::fireDMAEvent() noexcept
{
	// Try to mark timer A for external event
	timers[0].markExternalEvent();
	// Also try to mark GPIO7 as having had an external event
	if (itrEnable & (1U << 15U))
		itrPending |= (1U << 15U);
}

uint8_t mc68901_t::irqCause() noexcept
{
	// Compute which IRQs are not masked
	const auto interrupts{itrPending & itrMask};
	// Look through the pending IRQs and find the highest priority one
	const auto channel
	{
		[&]() -> size_t
		{
			// For each interrupt request bit
			for (const auto &irq : substrate::indexSequence_t{16U})
			{
				// Calculate the channel for this IRQ
				const auto irqChannel{15U - irq};
				// If the channel has a pending IRQ set, then use it as the cause
				if (interrupts & (1U << irqChannel))
					return irqChannel;
			}
			// This should never be possible to be reached, but just in case..
			return 16U;
		}()
	};
	// If there was no reason to call this function, indicate a spurious interrupt error
	if (channel >= 16U)
		return 0x18U;
	// Otherwise, clear pending on this IRQ and generate the vector number value
	itrPending &= ~(1U << channel);
	// Re-queue IRQ if there are still pending
	if (itrPending & itrMask)
		requestInterrupt();
	return (vectorReg & 0xf0) | channel;
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
		const auto prescale{prescalingFor(value & 0x07U)};
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

	void timer_t::markExternalEvent() noexcept
	{
		if ((control & 0x0f) == 0x08U)
			externalEvent = true;
	}

	bool timer_t::clockCycle() noexcept
	{
		// Check if the timer is stopped
		if ((control & 0x0fU) == 0U)
			return false;
		// If it is not, check if this is a clock advancement cycle
		if (!clockManager.advanceCycle())
			return false;
		// Check if the timer is in a counting mode
		if ((control & 0x08U) == 0U)
		{
			// Apply a clock pulse to the counter, and if that counter is now 0, reload it to the value in reloadValue
			if (--counter == 0U)
			{
				counter = reloadValue;
				// Signal that this was an interrupt generating cycle
				return true;
			}
		}
		// Check if the timer is in event count mode
		else if ((control & 0x0fU) == 0x08U)
		{
			// If there is a pending external event
			if (!externalEvent)
				return false;
			// Clear the event marker
			externalEvent = false;
			// Apply a clock pulse to the counter, and if that counter is now 0, reload it to the value in reloadValue
			if (--counter == 0U)
			{
				counter = reloadValue;
				// Signal that this was an interrupt generating cycle
				return true;
			}
		}
		else
		{
			// For now we don't support the PWM modes
		}
		// Signal that this was not an interrupt generating cycle
		return false;
	}

	uint32_t timer_t::prescalingFor(const uint8_t mode) noexcept
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
	}
} // namespace mc68901
