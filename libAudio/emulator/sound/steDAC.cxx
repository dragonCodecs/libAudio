// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <tuple>
#include <substrate/span>
#include <substrate/index_sequence>
#include <substrate/buffer_utils>
#include "steDAC.hxx"
#include "rustAPI.hxx"

extern "C"
{
	STeDac *ste_dac_new(uint32_t, MC68901);
	void ste_dac_drop(STeDac *);

	void ste_dac_read_address(const STeDac *, uint32_t, slice_t<uint8_t>);
	void ste_dac_write_address(STeDac *, uint32_t, const slice_t<uint8_t>);
	int16_t ste_dac_sample(STeDac *, const memoryMap_t<uint32_t, 0x00ffffffU> *);
	bool ste_dac_clock_cycle(STeDac *);
	uint8_t ste_dac_output_level(STeDac *);

	int8_t memory_map_read_address_i8(const memoryMap_t<uint32_t, 0x00ffffffU> *self, const uint32_t address)
	{
		return self->readAddress<int8_t>(address);
	}
}

/*
 * Implements the Atari STe's Sound DMA, Microwire and DAC interfaces
 *
 * References:
 * - Sound DMA is implemented by the GST Shifter chip, no official documentation exists however
 *   some RE has been done as detailed here: https://info-coach.fr/atari/hardware/STE-HW.php#shifter
 * - The Microwire interface is under-documented, but at heart provides access to the registers
 *   of various blocks on the STe, including the LMC1992 which provides volume control
 * - LMC1992 is a National Semi part through which all audio flows to get out the STe
 *   archived datasheet: https://www.tomek.cedro.info/files/electronics/doc/ic_various/LMC1992.PDF
 * - STe and other schematics: https://docs.dev-docs.org/htm/search.php?find=_s
 */

steDAC_t::steDAC_t(const uint32_t clockFrequency, mc68901_t &mfp) noexcept :
	clockedPeripheral_t<uint32_t>{clockFrequency}, self{ste_dac_new(clockFrequency, mfp.inner())} { }

steDAC_t::~steDAC_t() noexcept
{
	if (self)
		ste_dac_drop(self);
}

void steDAC_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	ste_dac_read_address(self, address, {data.data(), data.size_bytes()});
}

void steDAC_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
	ste_dac_write_address(self, address, {data.data(), data.size_bytes()});
}

bool steDAC_t::clockCycle() noexcept
{
	return ste_dac_clock_cycle(self);
}

int16_t steDAC_t::sample(const memoryMap_t<uint32_t, 0x00ffffffU> &memoryMap) const noexcept
{
	return ste_dac_sample(self, &memoryMap);
}

uint8_t steDAC_t::outputLevel() const noexcept
{
	return ste_dac_output_level(self);
}
