// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <substrate/span>
#include <substrate/buffer_utils>
#include "atariSTeROMs.hxx"
#include "cpu/m68k.hxx"
#include "rustAPI.hxx"

extern "C"
{
	AtariSTeROMs *atari_ste_roms_new(motorola68000_t *, memoryMap_t<uint32_t, 0x00ffffffU> *,
		uint32_t, uint32_t);
	void atari_ste_roms_drop(AtariSTeROMs *);

	void atari_ste_roms_read_address(const AtariSTeROMs *, uint32_t, slice_t<uint8_t>);
	void atari_ste_roms_write_address(AtariSTeROMs *, uint32_t, const slice_t<uint8_t>);

	uint32_t cpu_read_addr_register(motorola68000_t *cpu, size_t reg)
	{
		return cpu->readAddrRegister(reg);
	}

	void cpu_write_data_register(motorola68000_t *cpu, size_t reg, uint32_t value)
	{
		cpu->writeDataRegister(reg, value);
	}

	uint16_t peripherals_read_address_u16(memoryMap_t<uint32_t, 0x00ffffffU> *peripherals, uint32_t address)
	{
		return peripherals->readAddress<uint16_t>(address);
	}

	int32_t peripherals_read_address_i32(memoryMap_t<uint32_t, 0x00ffffffU> *peripherals, uint32_t address)
	{
		return peripherals->readAddress<int32_t>(address);
	}

	uint32_t peripherals_read_address_u32(memoryMap_t<uint32_t, 0x00ffffffU> *peripherals, uint32_t address)
	{
		return peripherals->readAddress<uint32_t>(address);
	}
}

atariSTeROMs_t::atariSTeROMs_t(motorola68000_t &cpu, memoryMap_t<uint32_t, 0x00ffffffU> &peripherals,
	const uint32_t heapBase, const uint32_t heapSize) noexcept : peripheral_t<uint32_t>{},
	self{atari_ste_roms_new(&cpu, &peripherals, heapBase, heapSize)} { }

atariSTeROMs_t::~atariSTeROMs_t() noexcept
	{ atari_ste_roms_drop(self); }

void atariSTeROMs_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	atari_ste_roms_read_address(self, address, {data.data(), data.size_bytes()});
}

void atariSTeROMs_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
	atari_ste_roms_write_address(self, address, {data.data(), data.size_bytes()});
}
