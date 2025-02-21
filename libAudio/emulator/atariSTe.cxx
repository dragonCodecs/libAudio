// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include "atariSTe.hxx"
#include "ram.hxx"
#include "sound/ym2149.hxx"
#include "unitsHelpers.hxx"
#include "sndh/iceDecrunch.hxx"

using stRAM_t = ram_t<uint32_t, 8_MiB>;

atariSTe_t::atariSTe_t() noexcept
{
	// Build the system memory map
	addressMap[{0x000000U, 0x800000U}] = std::make_unique<stRAM_t>();
	// TODO: TOS ROM needs to go at 0xe00000U, it is in a 1MiB window
	// Cartridge ROM at 0xfa0000, 128KiB
	// pre-TOS 2.0 OS ROMs at 0xfc0000, 128KiB
	addressMap[{0xff8800U, 0xff8804U}] = std::make_unique<ym2149_t>(2_MHz);
	// sound DMA at 0xff8900
}

// Copy the contents of a decrunched SNDH into the ST's RAM
bool atariSTe_t::copyToRAM(sndhDecruncher_t &data) noexcept
{
	stRAM_t &systemRAM{*dynamic_cast<stRAM_t *>(addressMap[{0x000000U, 0x800000U}].get())};
	// Get a span that's past the end of the system variables space, and the length of the decrunched SNDH file
	auto destination{systemRAM.subspan(0x001000U, data.length())};
	// Now make sure we're at the start of the data and copy it all in
	return data.head() && data.read(destination);
}

bool atariSTe_t::init(const uint16_t subtune) noexcept
{
	// Set up the calling context
	writeAddress<uint32_t>(0x800000U - 4U, 0xffffffffU);
	cpu.writeDataRegister(0U, subtune);
	// And that we're going to run from the init entrypoint
	cpu.executeFrom(0x001000U, 0x800000U - 4U, false);
	// Now run till we RTS and hit the sentinel program counter state
	while (true)
	{
		// Try and run another instruction
		const auto result{cpu.step()};
		// Check that something bad didn't happen
		if (result.trap || !result.validInsn)
			return false;
		// Check for the sentinel program counter value
		if (cpu.readProgramCounter() == 0xffffffffU)
			break;
	}
	return true;
}
