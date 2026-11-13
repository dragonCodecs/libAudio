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
#include "rustAPI.hxx"

struct STeRam;

struct u8Slice
{
	uint8_t *ptr;
	size_t len;
};

extern "C"
{
	STeRam *ste_ram_new();
	void ste_ram_drop(STeRam *);

	void ste_ram_read_address(const STeRam *, uint32_t, slice_t<uint8_t>);
	void ste_ram_write_address(STeRam *, uint32_t, const slice_t<uint8_t>);
	u8Slice ste_ram_slice(STeRam *, size_t, size_t);
}

template<typename address_t, size_t size> struct ram_t final : public peripheral_t<address_t>
{
private:
	STeRam *self{ste_ram_new()};

	void readAddress(address_t address, substrate::span<uint8_t> data) const noexcept override
	{
		ste_ram_read_address(self, address, {data.data(), data.size_bytes()});
	}

	void writeAddress(address_t address, const substrate::span<uint8_t> &data) noexcept override
	{
		ste_ram_write_address(self, address, {data.data(), data.size_bytes()});
	}

public:
	ram_t() noexcept = default;
	ram_t(const ram_t &) = default;
	ram_t(ram_t &&) = default;
	ram_t &operator =(const ram_t &) = default;
	ram_t &operator =(ram_t &&) = default;
	~ram_t() noexcept override { ste_ram_drop(self); };

	substrate::span<uint8_t> subspan(const size_t offset = 0U, const size_t length = SIZE_MAX) noexcept
	{
		auto slice = ste_ram_slice(self, offset, offset + length);
		return substrate::span{slice.ptr, slice.len};
	}
};

#endif /*EMULATOR_RAM_HXX*/
