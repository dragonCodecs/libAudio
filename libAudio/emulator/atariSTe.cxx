// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <string_view>
#include <type_traits>
#include "atariSTe.hxx"
#include "ram.hxx"
#include "sound/ym2149.hxx"
#include "timing/mc68901.hxx"
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

constexpr static std::array<uint8_t, 4U> cookieSND{{'_', 'S', 'N', 'D'}};
constexpr static std::array<uint8_t, 4U> cookieMCH{{'_', 'M', 'C', 'H'}};

constexpr static std::array<uint32_t, 4U> timerVectorAddresses
{{
	0x000174U,
	0x000160U,
	0x000154U,
	0x000150U,
}};

atariSTe_t::atariSTe_t() noexcept :
	// Set up a dummy clock manager for the play routine
	playRoutineManager{systemClockFrequency, systemClockFrequency}
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
	psg = addClockedPeripheral({0xff8800U, 0xff8804U}, std::make_unique<ym2149_t>(2_MHz, sampleRate));
	// sound DMA at 0xff8900
	mfp = addClockedPeripheral({0xfffa00U, 0xfffa40U}, std::make_unique<mc68901_t>(2457600U));

	// Make all timer handlers RTEs for now
	for (const auto &address : timerVectorAddresses)
		writeAddress(address, uint16_t{0x4e73U});
}

void atariSTe_t::configureTimer(const char timer, const uint16_t timerFrequency) noexcept
{
	// Convert the timer's letter code into a timer index
	if (timer < 'A' || timer > 'D')
		return;
	const auto index{static_cast<size_t>(timer - 'A')};
	// Set the selected timer's vector slot to a branch to the play routine
	const auto vectorAddress{timerVectorAddresses[index]};
	// BRA.W
	writeAddress(vectorAddress + 0U, uint16_t{0x6000U});
	// to address 0x001008U (SNDH play routine)
	writeAddress(vectorAddress + 2U, static_cast<uint16_t>(0x1008U - (vectorAddress + 2U)));

	// Now the vector slot is properly set up, enable the timer and set the call frequency
	playRoutineManager = {systemClockFrequency, timerFrequency};
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
	// Set up the cookie jar at 0x000600 so replay routines work properly
	writeAddress(0x0005a0U, uint32_t{0x000600U});
	writeAddress(0x000600U, substrate::buffer_utils::readBE<uint32_t>({cookieSND}));
	// Bit 0 -> PSG, bit 1 -> DMA sound, bit 2 -> CODEC, bit 3 -> DSP
	writeAddress(0x000604U, uint32_t{0x00000001U});
	writeAddress(0x000608U, substrate::buffer_utils::readBE<uint32_t>({cookieMCH}));
	// Set the machine type to an STe
	writeAddress(0x00060cU, uint32_t{0x00010000U});
	// Sentinel that ends the cookie jar
	writeAddress(0x000610U, uint32_t{0U});

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

	// If the play routine should be run again, set that up
	if (playRoutineManager.advanceCycle())
	{
		// Check to see that the last call to it actually finished
		if (cpu.readProgramCounter() != 0xffffffffU)
			// It did not, error
			return false;
		cpu.executeFrom(0x001008U, 0x800000U, false);
	}

	// Check if we got any pending interrupts from the timers
	const auto pendingIRQs{mfp->pendingInterrupts()};
	if (pendingIRQs)
	{
		// For each of the IRQs that stages, translate to a timer and stage an appropriate
		// exception frame for them - starting with Timer A
		if (pendingIRQs & (1U << 13U))
			cpu.stageIRQCall(timerVectorAddresses[0U]);
		// Timer B
		if (pendingIRQs & (1U << 8U))
			cpu.stageIRQCall(timerVectorAddresses[1U]);
		// Timer C
		if (pendingIRQs & (1U << 5U))
			cpu.stageIRQCall(timerVectorAddresses[2U]);
		// And finally Timer D
		if (pendingIRQs & (1U << 4U))
			cpu.stageIRQCall(timerVectorAddresses[3U]);
		// Now we've staged appropriate invocations for them, clear them
		mfp->clearInterrupts(pendingIRQs);
	}

	// CPU ratio is 32:8, aka 4, so use a simple AND mask to maintain that
	timeSinceLastCPUCycle = (timeSinceLastCPUCycle + 1U) & 3U;
	// If the CPU should run a cycle, have it try to progress
	if (timeSinceLastCPUCycle == 0U)
	{
		// If the CPU isn't halted, run another instruction and see how many cycles that advanced us by
		if (cpu.readProgramCounter() != 0xffffffffU)
		{
			// Grab the program counter at the start
			const auto programCounter{cpu.readProgramCounter()};
			// Try to advance the clock and check if the CPU is in a trap state
			if (!cpu.advanceClock() || cpu.trapped())
			{
				// Something bad happened, so display the program counter at the faulting instruction
				console.debug("Bad instruction at "sv, asHex_t<6U, '0'>{programCounter});
				return false;
			}
		}
	}

	return true;
}

bool atariSTe_t::sampleReady() const noexcept
	{ return psg->sampleReady(); }

int16_t atariSTe_t::readSample() noexcept
	{ return psg->sample(); }

void atariSTe_t::displayCPUState() const noexcept
	{ cpu.displayRegs(); }
