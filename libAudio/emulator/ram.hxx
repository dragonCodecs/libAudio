// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_RAM_HXX
#define EMULATOR_RAM_HXX

#include <cstdint>
#include <cstddef>
#include <array>
#include <substrate/indexed_iterator>
#include <substrate/span>
#include "memoryMap.hxx"

template<typename address_t, size_t size> struct ram_t final : public peripheral_t<address_t>
{
private:
	std::array<uint8_t, size> memory{};

	void readAddress(address_t address, substrate::span<uint8_t> data) const noexcept override
	{
		for (auto [idx, byte] : substrate::indexedIterator_t{data})
			byte = memory[address + idx];
	}

	void writeAddress(address_t address, const substrate::span<uint8_t> &data) noexcept override
	{
		for (const auto &[idx, byte] : substrate::indexedIterator_t{data})
			memory[address + idx] = byte;
	}

public:
	ram_t() noexcept = default;
	ram_t(const ram_t &) = default;
	ram_t(ram_t &&) = default;
	ram_t &operator =(const ram_t &) = default;
	ram_t &operator =(ram_t &&) = default;
	~ram_t() noexcept override = default;

	substrate::span<uint8_t> subspan(const size_t offset = 0U, const size_t length = SIZE_MAX) noexcept
		{ return substrate::span{memory}.subspan(offset, length); }
};

#endif /*EMULATOR_RAM_HXX*/
