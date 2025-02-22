// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_TIMING_MC68901_HXX
#define EMULATOR_TIMING_MC68901_HXX

#include <cstdint>
#include <array>
#include <substrate/span>
#include "../memoryMap.hxx"

namespace mc68901
{
	struct timer_t final
	{
	private:
		uint8_t control{0U};
		uint8_t counter{0U};
		uint8_t reloadValue{0U};
		clockManager_t clockManager;

	public:
		timer_t(uint32_t baseClockFrequency) noexcept;

		[[nodiscard]] uint8_t ctrl() const noexcept;
		void ctrl(uint8_t value, uint32_t baseClockFrequency) noexcept;
		[[nodiscard]] uint8_t data() const noexcept;
		void data(uint8_t value) noexcept;

		void clockCycle() noexcept;
	};
} // namespace mc68901

struct mc68901_t final : public clockedPeripheral_t<uint32_t>
{
private:
	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept override;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept override;

	uint8_t gpio;
	uint8_t activeEdge;
	uint8_t dataDirection;
	uint16_t itrEnable;
	uint16_t itrPending;
	uint16_t itrServicing;
	uint16_t itrMask;
	uint8_t vectorReg;
	uint8_t syncChar;
	uint8_t usartCtrl;
	uint8_t rxStatus;
	uint8_t txStatus;
	uint8_t usartData;

	std::array<mc68901::timer_t, 4> timers;

public:
	mc68901_t(uint32_t clockFrequency) noexcept;
	mc68901_t(const mc68901_t &) = default;
	mc68901_t(mc68901_t &&) = default;
	mc68901_t &operator =(const mc68901_t &) = default;
	mc68901_t &operator =(mc68901_t &&) = default;
	~mc68901_t() noexcept override = default;

	[[nodiscard]] bool clockCycle() noexcept override;
};

#endif /*EMULATOR_TIMING_MC68901_HXX*/
