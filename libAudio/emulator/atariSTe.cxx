// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <string_view>
#include <type_traits>
#include "atariSTe.hxx"
#include "ram.hxx"
#include "sound/ym2149.hxx"
#include "unitsHelpers.hxx"
#include "sndh/iceDecrunch.hxx"
#include "console.hxx"

using namespace std::literals::string_view_literals;
using namespace libAudio::console;

using stRAM_t = ram_t<uint32_t, 8_MiB>;

template<typename T> struct isUniquePtr : std::false_type { };
template<typename T> struct isUniquePtr<std::unique_ptr<T>> : std::true_type { };
template<typename T> constexpr inline bool isUniquePtr_v = isUniquePtr<T>::value;

template<typename T> struct isClockedPeripheral :
	std::is_base_of<clockedPeripheral_t<uint32_t>, T> { };
template<typename T> struct isClockedPeripheral<std::unique_ptr<T>> :
	isClockedPeripheral<T> { };
template<typename T> constexpr inline bool isClockedPeripheral_v = isClockedPeripheral<T>::value;

atariSTe_t::atariSTe_t() noexcept
{
	const auto addClockedPeripheral
	{
		[this](const memoryRange_t<uint32_t> addressRange, auto peripheral)
		{
			// Make sure we're invoked only with a std::unique_ptr<> of some kind of clockedPeripheral_t
			static_assert(isUniquePtr_v<decltype(peripheral)> && isClockedPeripheral_v<decltype(peripheral)>);

			// Extract a pointer to the device
			auto *const ptr{peripheral.get()};

			// Add the device to the clocking map according to the clocking ratio set up
			clockedPeripherals[ptr] = {systemClockFrequency, peripheral->clockFrequency()};
			// Add the device to the address map
			addressMap[addressRange] = std::move(peripheral);

			// Return that pointer for use externally
			return ptr;
		}
	};

	// Build the system memory map
	addressMap[{0x000000U, 0x800000U}] = std::make_unique<stRAM_t>();
	// TODO: TOS ROM needs to go at 0xe00000U, it is in a 1MiB window
	// Cartridge ROM at 0xfa0000, 128KiB
	// pre-TOS 2.0 OS ROMs at 0xfc0000, 128KiB
	psg = addClockedPeripheral({0xff8800U, 0xff8804U}, std::make_unique<ym2149_t>(2_MHz));
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
	cpu.writeDataRegister(0U, subtune);
	// And run the init entrypoint to return
	return cpu.executeToReturn(0x001000U, 0x800000U, false);
}

bool atariSTe_t::exit() noexcept
	{ return cpu.executeToReturn(0x001004U, 0x800000U, false); }

bool atariSTe_t::advanceClock() noexcept
{
	// The machine runs on a 32MHz (ish) clock, advance a cycle and run
	// any events on any hardware that needs it

	// For each peripheral in the clocked set, see if that peripheral should have a cycle run
	for (auto &[peripheral, clockManager] : clockedPeripherals)
	{
		if (clockManager.advanceCycle())
		{
			// We should try to advance a cycle, check to see if that worked
			if (!peripheral->clockCycle())
				return false;
		}
	}

	// See if the triggering timer is ready

	// CPU ratio is 32:8, aka 4, so use a simple AND mask to maintain that
	timeSinceLastCPUCycle = (timeSinceLastCPUCycle + 1U) & 3U;
	// If the CPU should run a cycle, have it try to progress
	if (timeSinceLastCPUCycle == 0U)
	{
		// If the CPU isn't halted, run another instruction and see how many cycles that advanced us by
		if (cpu.readProgramCounter() != 0xffffffffU)
		{
			// Grab the program counter at the start and run an instruction
			const auto programCounter{cpu.readProgramCounter()};
			const auto result{cpu.step()};
			// Check that something bad didn't happen
			if (result.trap || !result.validInsn)
			{
				// Something bad happened, so display the program counter at the faulting instruction
				console.debug("Bad instruction at "sv, asHex_t<6U, '0'>{programCounter});
				return false;
			}
		}
		// XXX: Not actually correct
		else
			// Set up to execute the play routine afresh
			cpu.executeFrom(0x001008U, 0x80000000U, false);
	}

	return true;
}

bool atariSTe_t::sampleReady() const noexcept
	{ return psg->sampleReady(); }

void atariSTe_t::displayCPUState() const noexcept
	{ cpu.displayRegs(); }
