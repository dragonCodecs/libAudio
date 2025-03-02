// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_MEMORY_MAP_HXX
#define EMULATOR_MEMORY_MAP_HXX

#include <memory>
#include <unordered_map>
#include <limits>
#include <substrate/span>
#include <substrate/buffer_utils>

using namespace substrate::buffer_utils;

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

// A generic clockless peripheral
template<typename address_t> struct peripheral_t
{
private:
	peripheral_t(const peripheral_t &) noexcept = delete;
	peripheral_t(peripheral_t &&) noexcept = delete;
	peripheral_t &operator =(const peripheral_t &) noexcept = delete;
	peripheral_t &operator =(peripheral_t &&) noexcept = delete;

protected:
	peripheral_t() noexcept = default;

public:
	virtual ~peripheral_t() noexcept = default;

	virtual void readAddress(address_t address, substrate::span<uint8_t> data) const noexcept = 0;
	virtual void writeAddress(address_t address, const substrate::span<uint8_t> &data) noexcept = 0;
};

// A peripheral that requires clocking at some frequency
template<typename address_t> struct clockedPeripheral_t : public peripheral_t<address_t>
{
private:
	clockedPeripheral_t(const clockedPeripheral_t &) noexcept = delete;
	clockedPeripheral_t(clockedPeripheral_t &&) noexcept = delete;
	clockedPeripheral_t &operator =(const clockedPeripheral_t &) noexcept = delete;
	clockedPeripheral_t &operator =(clockedPeripheral_t &&) noexcept = delete;

	uint32_t _clockFrequency;

protected:
	clockedPeripheral_t(const uint32_t clockFrequency) noexcept :
		peripheral_t<address_t>{}, _clockFrequency{clockFrequency} { }

public:
	~clockedPeripheral_t() noexcept override = default;

	[[nodiscard]] uint32_t clockFrequency() const noexcept { return _clockFrequency; }
	[[nodiscard]] virtual bool clockCycle() noexcept = 0;
};

template<typename address_t, address_t validAddressMask = std::numeric_limits<address_t>::max()> struct memoryMap_t
{
protected:
	// Use 16 buckets at minimum.
	std::unordered_map<memoryRange_t<address_t>, std::unique_ptr<peripheral_t<address_t>>> addressMap{16U};

public:
	template<typename value_t> value_t readAddress(const address_t address) const noexcept
	{
		const auto adjustedAddres{address & validAddressMask};
		// Try to find a peripheral mapped for the address
		for (const auto &[mapping, peripheral] : addressMap)
		{
			// If the peripheral's mapping contains this address
			if (mapping.inRange(adjustedAddres))
			{
				// Convert the address to a relative one
				const auto relativeAddress{mapping.relative(adjustedAddres)};
				std::array<uint8_t, sizeof(value_t)> value{};
				// Read the data associated with that address in the peripheral
				peripheral->readAddress(relativeAddress, value);

				// Now we have data, convert it endian-appropriately to a value_t
				if constexpr (sizeof(value_t) == 1U)
					return static_cast<value_t>(value[0]);
				else
					return readBE<value_t>(value);
			}
		}

		// If we couldn't find the address in any known peripheral, synthesise a value
		return {};
	}

	template<typename value_t> void writeAddress(const address_t address, const value_t data) noexcept
	{
		const auto adjustedAddres{address & validAddressMask};
		// Try to find a peripheral to write the value to
		for (auto &[mapping, peripheral] : addressMap)
		{
			// If the peripheral's mapping contains this address
			if (mapping.inRange(adjustedAddres))
			{
				// Convert the address to a relative one
				const auto relativeAddress{mapping.relative(adjustedAddres)};
				std::array<uint8_t, sizeof(value_t)> value{};
				// Convert the data to write in an endian-appropriate manner from a value_t
				if constexpr (sizeof(value_t) == 1U)
					value[0] = static_cast<uint8_t>(data);
				else
					writeBE(data, value);

				// Now write the data to the peripheral and get done
				peripheral->writeAddress(relativeAddress, value);
				break;
			}
		}
	}
};

struct clockManager_t
{
private:
	uint32_t baseFrequency;
	uint32_t targetFrequency;
	uint32_t cycleCounter{0U};

public:
	// Construct an invalid manager with infinite ratio
	clockManager_t() noexcept : baseFrequency{0U}, targetFrequency{0U} { }
	// Construct a valid manager from the base and target clock frequencies
	clockManager_t(uint32_t baseClockFrequency, uint32_t targetClockFrequency) noexcept;
	// Returns true if the clock being managed by this should advance a cycle, false otherwise
	bool advanceCycle() noexcept;
};

#endif /*EMULATOR_MEMORY_MAP_HXX*/
