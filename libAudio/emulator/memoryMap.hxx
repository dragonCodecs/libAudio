// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_MEMORY_MAP_HXX
#define EMULATOR_MEMORY_MAP_HXX

#include <memory>
#include <unordered_map>
#include <substrate/span>

template<typename address_t> struct memoryRange_t
{
private:
	address_t _begin;
	address_t _end;

public:
	memoryRange_t(const address_t begin, const address_t end) : _begin{begin}, _end{end} { }

	// Check if a given address is in this particular address range
	bool inRange(const address_t address) const
	{
		return _begin <= address && address < _end;
	}

	// Convert the address to one relative to the start of the range
	address_t relative(const address_t address) const
	{
		return address - _begin;
	}

	address_t begin() const noexcept { return _begin; }
	address_t end() const noexcept { return _end; }

	bool operator ==(const memoryRange_t &other) const noexcept
		{ return _begin == other._begin && _end == other._end; }
};

namespace std
{
	template<typename address_t> struct hash<memoryRange_t<address_t>>
	{
		size_t operator ()(const memoryRange_t<address_t> &range) const noexcept
		{
			return static_cast<size_t>(range.begin());
		}
	};
}

template<typename address_t> struct peripheral_t
{
	virtual void readAddress(address_t address, substrate::span<uint8_t> data) const noexcept = 0;
	virtual void writeAddress(address_t address, const substrate::span<uint8_t> &data) noexcept = 0;
};

template<typename address_t> struct memoryMap_t
{
protected:
	// Use 16 buckets at minimum.
	std::unordered_map<memoryRange_t<address_t>, std::unique_ptr<peripheral_t<address_t>>> addressMap{16U};

	template<typename value_t> value_t readAddress(const address_t address) const noexcept
	{
		// Try to find a peripheral mapped for the address
		for (const auto &[mapping, peripheral] : addressMap)
		{
			// If the peripheral's mapping contains this address
			if (mapping.inRange(address))
			{
				// Convert the address to a relative one
				const auto relativeAddress{mapping.relative(address)};
				std::array<uint8_t, sizeof(value_t)> value{};
				// Read the data associated with that address in the peripheral
				peripheral.readAddress(relativeAddress, value);

				// Now we have data, convert it endian-appropriately to a value_t
			}
		}

		// If we couldn't find the address in any known peripheral, synthesise a value
		return {};
	}

	template<typename value_t> void writeAddress(const address_t address, const value_t data) noexcept
	{
		// Try to find a peripheral to write the value to
		for (auto &[mapping, peripheral] : addressMap)
		{
			// If the peripheral's mapping contains this address
			if (mapping.inRange(address))
			{
				// Convert the address to a relative one
				const auto relativeAddress{mapping.relative(address)};
				std::array<uint8_t, sizeof(value_t)> value{};
				// Convert the data to write in an endian-appropriate manner from a value_t

				// Now write the data to the peripheral and get done
				peripheral.writeAddress(relativeAddress, value);
				break;
			}
		}
	}
};

#endif /*EMULATOR_MEMORY_MAP_HXX*/
