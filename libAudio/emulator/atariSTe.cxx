// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <string_view>
#include <type_traits>
#include "atariSTe.hxx"
#include "atariSTeROMs.hxx"
#include "ram.hxx"
#include "sound/ym2149.hxx"
#include "sound/steDAC.hxx"
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
	0x000134U,
	0x000120U,
	0x000114U,
	0x000110U,
}};

constexpr static uint32_t gpio7VectorAddress{0x00013cU};

// Private address we stick an RTE instruction at for vector returns and such
constexpr static uint32_t rteAddress{0x000700U};
// Private address we stick a STOP instruction at for unimplemented vectors
constexpr static uint32_t stopAddress{0x000704U};

constexpr static uint32_t stackBase{0x600000U};
constexpr static uint32_t stackSize{0x100000U};
constexpr static uint32_t stackTop{stackBase + stackSize};
constexpr static uint32_t heapBase{0x700000U};
constexpr static uint32_t heapSize{0x100000U};

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
	addressMap[{0xe00000U, 0xf00000U}] = std::make_unique<atariSTeROMs_t>
	(
		cpu, static_cast<memoryMap_t<uint32_t, 0x00ffffffU> &>(*this), heapBase, heapSize
	);
	// Cartridge ROM at 0xfa0000, 128KiB
	// pre-TOS 2.0 OS ROMs at 0xfc0000, 128KiB
	psg = addClockedPeripheral({0xff8800U, 0xff8804U}, std::make_unique<ym2149_t>(2_MHz, sampleRate));
	mfp = addClockedPeripheral({0xfffa00U, 0xfffa40U}, std::make_unique<mc68901_t>(2457600U, cpu, 6U));
	dac = addClockedPeripheral({0xff8900U, 0xff8926U}, std::make_unique<steDAC_t>(50_kHz + 66U, *mfp));

	// Set up our dummy RTE for vector handling
	writeAddress(rteAddress, uint16_t{0x4e73U});
	// Set up a dummy STOP for unimplemented vectors
	writeAddress(stopAddress, uint16_t{0x4e72U});
	writeAddress(stopAddress + 2U, uint16_t{0x2700U});

	// Set up the TRAP handlers using the dummy STOP for unused ones
	writeAddress(0x000080U, stopAddress);
	// Set up the GEMDOS TRAP handler so the ROM will handle it
	writeAddress(0x000084U, uint32_t{0xe00000U + atariSTeROMs_t::handlerAddressGEMDOS});
	writeAddress(0x000088U, stopAddress);
	writeAddress(0x00008cU, stopAddress);
	writeAddress(0x000090U, stopAddress);
	writeAddress(0x000094U, stopAddress);
	writeAddress(0x000098U, stopAddress);
	writeAddress(0x00009cU, stopAddress);
	writeAddress(0x0000a0U, stopAddress);
	writeAddress(0x0000a4U, stopAddress);
	writeAddress(0x0000a8U, stopAddress);
	writeAddress(0x0000acU, stopAddress);
	writeAddress(0x0000b0U, stopAddress);
	writeAddress(0x0000b4U, stopAddress);
	writeAddress(0x0000b8U, stopAddress);
	writeAddress(0x0000bcU, stopAddress);

	// Make all timer handlers RTEs for now
	for (const auto &address : timerVectorAddresses)
		writeAddress(address, rteAddress);
	// And the GPIO7 handler too
	writeAddress(gpio7VectorAddress, rteAddress);
	// Tell the MFP its vector number information and that we're in automatic mode
	writeAddress(0xfffa17U, uint8_t{0x40U});
	// Register it as an interrupt source
	cpu.registerInterruptRequester(*mfp);
}

void atariSTe_t::configureTimer(const char timer, const uint16_t timerFrequency) noexcept
{
	// Convert the timer's letter code into a timer index
	if (timer < 'A' || timer > 'D')
		return;
	const auto index{static_cast<size_t>(timer - 'A')};
	// Set the selected timer's vector slot to a branch to the play routine
	const auto vectorAddress{timerVectorAddresses[index]};
	// And write the address of the SNDH play routine to that location
	writeAddress(vectorAddress, uint32_t{0x010008U});

	// Now the vector slot is properly set up, enable the timer and set the call frequency
	playRoutineManager = {systemClockFrequency, timerFrequency};
}

// Copy the contents of a decrunched SNDH into the ST's RAM
bool atariSTe_t::copyToRAM(sndhDecruncher_t &data) noexcept
{
	stRAM_t &systemRAM{*dynamic_cast<stRAM_t *>(addressMap[{0x000000U, 0x800000U}].get())};
	// Get a span that's past the end of the system variables space, and the length of the decrunched SNDH file
	// But that also excludes the heap and stack spaces
	auto destination{systemRAM.subspan(0U, stackBase).subspan(0x010000U, data.length())};
	// Now make sure we're at the start of the data and copy it all in
	return data.head() && data.read(destination);
}

bool atariSTe_t::init(const uint16_t subtune) noexcept
{
	// Set up the cookie jar at 0x000600 so replay routines work properly
	writeAddress(0x0005a0U, uint32_t{0x000600U});
	writeAddress(0x000600U, substrate::buffer_utils::readBE<uint32_t>({cookieSND}));
	// Bit 0 -> PSG, bit 1 -> DMA sound, bit 2 -> CODEC, bit 3 -> DSP
	writeAddress(0x000604U, uint32_t{0x00000003U});
	writeAddress(0x000608U, substrate::buffer_utils::readBE<uint32_t>({cookieMCH}));
	// Set the machine type to an STe
	writeAddress(0x00060cU, uint32_t{0x00010000U});
	// Sentinel that ends the cookie jar
	writeAddress(0x000610U, uint32_t{0U});

	// Set up the calling context
	cpu.writeDataRegister(0U, subtune + 1U);
	// And run the init entrypoint to return
	return cpu.executeToReturn(0x010000U, stackTop, false);
}

bool atariSTe_t::exit() noexcept
	{ return cpu.executeToReturn(0x010004U, stackTop, false); }

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
		// Check to see that the last call to it actually finished, and force it to if it hasn't
		while (cpu.readProgramCounter() != 0xffffffffU)
		{
			// Grab the program counter at the start
			const auto programCounter{cpu.readProgramCounter()};
			// Try to execute the next instruction
			const auto result{cpu.step()};
			// If the instruction was invalid or caused a trap, bail
			if (!result.validInsn || result.trap)
			{
				// Display the program counter at the faulting instruction
				console.debug("Bad instruction at "sv, asHex_t<6U, '0'>{programCounter});
				return false;
			}
		}
		cpu.executeFrom(0x010008U, stackTop, false);
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
{
	// Extract the sample from the PSG
	const auto psgSample{psg->sample()};
	// Extract the sample from the STe DAC
	const auto dacSample{dac->sample(*this)};
	// Combine the samples to generate the input to the scaling
	const auto sample
	{
		[](const int32_t sample) -> int16_t
		{
			// Clamp the resulting sample to keep it in range
			if (sample > INT16_MAX)
				return INT16_MAX;
			if (sample < INT16_MIN)
				return INT16_MIN;
			return static_cast<int16_t>(sample);
		}(psgSample + dacSample)
	};
	// Scale the sample by the output level set via the DAC block LMC1992 and return
	// NB: the max output level is 64, allowing this to be simplified by the compiler and fast
	return (sample * dac->outputLevel()) / 64;
}

void atariSTe_t::displayCPUState() const noexcept
	{ cpu.displayRegs(); }
