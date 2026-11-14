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

struct MemoryMap;

extern "C"
{
	int8_t memory_map_read_address_i8(MemoryMap *, uint32_t);
	uint8_t memory_map_read_address_u8(MemoryMap *, uint32_t);
	int16_t memory_map_read_address_i16(MemoryMap *, uint32_t);
	uint16_t memory_map_read_address_u16(MemoryMap *, uint32_t);
	int32_t memory_map_read_address_i32(MemoryMap *, uint32_t);
	uint32_t memory_map_read_address_u32(MemoryMap *, uint32_t);

	void memory_map_write_address_i8(MemoryMap *, uint32_t, int8_t);
	void memory_map_write_address_u8(MemoryMap *, uint32_t, uint8_t);
	void memory_map_write_address_i16(MemoryMap *, uint32_t, int16_t);
	void memory_map_write_address_u16(MemoryMap *, uint32_t, uint16_t);
	void memory_map_write_address_i32(MemoryMap *, uint32_t, int32_t);
	void memory_map_write_address_u32(MemoryMap *, uint32_t, uint32_t);
}

template<typename value_t> struct peripheralValue_t
{
	static value_t readAddress(MemoryMap *self, uint32_t address);
	static void writeAddress(MemoryMap *self, uint32_t address, value_t data);
};

template<> struct peripheralValue_t<int8_t>
{
	static int8_t readAddress(MemoryMap *self, uint32_t address)
		{ return memory_map_read_address_i8(self, address); }
	static void writeAddress(MemoryMap *self, uint32_t address, int8_t data)
		{ memory_map_write_address_i8(self, address, data); }
};

template<> struct peripheralValue_t<uint8_t>
{
	static uint8_t readAddress(MemoryMap *self, uint32_t address)
		{ return memory_map_read_address_u8(self, address); }
	static void writeAddress(MemoryMap *self, uint32_t address, uint8_t data)
		{ memory_map_write_address_u8(self, address, data); }
};

template<> struct peripheralValue_t<int16_t>
{
	static int16_t readAddress(MemoryMap *self, uint32_t address)
		{ return memory_map_read_address_i16(self, address); }
	static void writeAddress(MemoryMap *self, uint32_t address, int16_t data)
		{ memory_map_write_address_i16(self, address, data); }
};

template<> struct peripheralValue_t<uint16_t>
{
	static uint16_t readAddress(MemoryMap *self, uint32_t address)
		{ return memory_map_read_address_u16(self, address); }
	static void writeAddress(MemoryMap *self, uint32_t address, uint16_t data)
		{ memory_map_write_address_u16(self, address, data); }
};

template<> struct peripheralValue_t<int32_t>
{
	static int32_t readAddress(MemoryMap *self, uint32_t address)
		{ return memory_map_read_address_i32(self, address); }
	static void writeAddress(MemoryMap *self, uint32_t address, int32_t data)
		{ memory_map_write_address_i32(self, address, data); }
};

template<> struct peripheralValue_t<uint32_t>
{
	static uint32_t readAddress(MemoryMap *self, uint32_t address)
		{ return memory_map_read_address_u32(self, address); }
	static void writeAddress(MemoryMap *self, uint32_t address, uint32_t data)
		{ memory_map_write_address_u32(self, address, data); }
};

template<typename address_t, address_t validAddressMask = std::numeric_limits<address_t>::max()> struct memoryMap_t
{
private:
	MemoryMap *_self{nullptr};

public:
	memoryMap_t(MemoryMap *self) : _self{self} { }

	template<typename value_t> value_t readAddress(const address_t address) const noexcept
		{ return peripheralValue_t<value_t>::readAddress(_self, address); }

	template<typename value_t> void writeAddress(const address_t address, const value_t data) noexcept
		{ peripheralValue_t<value_t>::writeAddress(_self, address, data); }
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
