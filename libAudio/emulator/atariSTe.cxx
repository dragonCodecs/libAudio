// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <string_view>
#include <type_traits>
#include <substrate/index_sequence>
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
	addressMap[{0xe00000U, 0xf00000U}] = std::make_unique<atariSTeROMs_t>
	(
		cpu, static_cast<memoryMap_t<uint32_t, 0x00ffffffU> &>(*this), heapBase, heapSize
	);
	// Cartridge ROM at 0xfa0000, 128KiB
	// pre-TOS 2.0 OS ROMs at 0xfc0000, 128KiB
	psg = addClockedPeripheral({0xff8800U, 0xff8804U}, std::make_unique<ym2149_t>(static_cast<uint32_t>(2_MHz), sampleRate));
	mfp = addClockedPeripheral({0xfffa00U, 0xfffa40U}, std::make_unique<mc68901_t>(2457600U, cpu, uint8_t{6U}));
	dac = addClockedPeripheral({0xff8900U, 0xff8926U}, std::make_unique<steDAC_t>(static_cast<uint32_t>(50_kHz + 66U), *mfp));

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

struct udiv_t
{
	uint32_t quotient{0};
	uint32_t remainder{UINT32_MAX};

	udiv_t() noexcept = default;
	udiv_t(const uint32_t numerator, const uint32_t denominator) noexcept :
		quotient{numerator / denominator}, remainder{numerator % denominator} { }
};

void atariSTe_t::configureTimer(const char timer, const uint16_t timerFrequency) noexcept
{
	// Convert the timer's letter code into a timer index
	if (timer < 'A' || timer > 'D')
		return;
	const auto index{static_cast<size_t>(timer - 'A')};
	// Extract where the IRQ handler is for the required timer
	const auto vectorAddress{timerVectorAddresses[index]};

	// Now the vector slot is properly set up, enable the timer and set the call frequency
	// Start by computing the integer number of cycles per second (and its error) the requested frequency
	// will beat against the MFP
	const udiv_t cycleRate{mfp->clockFrequency(), timerFrequency};
	// Now figure out what divider is necessary to make that fit the 8-bit counter
	auto mode
	{
		[&]() -> uint8_t
		{
			udiv_t scaledRate{};
			uint8_t scaleFactor{7U};
			for (const auto &factor : substrate::indexSequence_t{8U})
			{
				// Calculate if this amount of prescaling results in a value that fits the timer counter
				const udiv_t counter{cycleRate.quotient, mc68901::timer_t::prescalingFor(static_cast<uint8_t>(factor))};
				// If it does and the error is less than the previously calculated value,
				// note the factor and store the value
				if (counter.quotient < 256U && counter.remainder < scaledRate.remainder)
				{
					scaledRate = counter;
					scaleFactor = static_cast<uint8_t>(factor);
				}
			}
			// Having hopefully found the best possible scale factor, return it
			return scaleFactor;
		}()
	};

	// With a scale factor picked, write the counter value, prescaling, and error into memory for the
	// timer IRQ handler to use to adjust for the error and maintain frequency accuracy
	const auto prescaler{mc68901::timer_t::prescalingFor(mode)};
	const udiv_t counter{cycleRate.quotient, prescaler};
	writeAddress(0x001000U, static_cast<uint8_t>(prescaler));
	writeAddress(0x001001U, static_cast<uint8_t>(counter.quotient)); // Reload value
	writeAddress(0x001002U, static_cast<uint8_t>(counter.remainder)); // Residual error
	writeAddress(0x001003U, uint8_t{0U});

	// Write the IRQ handler routine into memory so it can be used to trampoline into the SNDH play routine
	// Stack the registers we clobber
	writeAddress(0x001008U, uint16_t{0x48e7U});
	writeAddress(0x00100aU, uint16_t{0xc080U}); // movem.l d0-d1,a0, -(sp)
	// Prepare d0 to accept the accumulated error value
	writeAddress(0x00100cU, uint16_t{0x4240U}); // clr.w d0
	// Grab the accumulated error value
	writeAddress(0x00100eU, uint16_t{0x103aU});
	writeAddress(0x001010U, uint16_t{0xfff3U}); // move.b $-13(pc), d0
	// Prepare d1 to accept the error residual value
	writeAddress(0x001012U, uint16_t{0x4241U}); // clr.w d1
	// Grab the error residual value
	writeAddress(0x001014U, uint16_t{0x123aU});
	writeAddress(0x001016U, uint16_t{0xffecU}); // move.b $-14(pc), d1
	// Add the two together to get a new accumulation
	writeAddress(0x001018U, uint16_t{0xd240U}); // add.w d0, d1
	// Grab the prescaling value to compare the accumulation against
	writeAddress(0x00101aU, uint16_t{0x103aU});
	writeAddress(0x00101cU, uint16_t{0xffe4U}); // move.b $-1c(pc), d0
	// Put the timer data register location in a0
	writeAddress(0x00101eU, uint16_t{0x3078U});
	writeAddress(0x001020U, static_cast<uint16_t>(0xfa1fU + (index << 1U))); // movea.w ($fa1f).w, a0
	// Compare to see if prescaler < accumulation
	writeAddress(0x001022U, uint16_t{0xb041U}); // cmp.w d1, d0
	// Skip to non-adjustment code if false
	writeAddress(0x001024U, uint16_t{0x6c06U}); // bge +$06 (0x00102c)
	// Otherwise, add one to the reload value
	writeAddress(0x001026U, uint16_t{0x5210U}); // addq.b #1, (a0)
	// And subtract the prescaler off the accumulated error
	writeAddress(0x001028U, uint16_t{0x9240U}); // sub d1, d0
	// Jump to where we store the accumulated error
	writeAddress(0x00102aU, uint16_t{0x6006}); // bra +$06 (0x001032)
	// If the accumulation has not yet exceeded the prescaler value, load the original reload value back
	writeAddress(0x00102cU, uint16_t{0x103aU});
	writeAddress(0x00102eU, uint16_t{0xffd3U}); // move.b $-2d(pc), d0
	writeAddress(0x001030U, uint16_t{0x1080U}); // move.b d0, (a0)
	// Figure out where the accumulated error value is
	writeAddress(0x001032U, uint16_t{0x41faU});
	writeAddress(0x001034U, uint16_t{0xffd1U}); // lea $-2f(pc), a0
	// Write the accumulated error back
	writeAddress(0x001032U, uint16_t{0x1081U});  // move.b d1, (a0)
	// Unstack the clobbered registers
	writeAddress(0x001034U, uint16_t{0x4cdfU});
	writeAddress(0x001036U, uint16_t{0x0103U}); // movew.l (sp)+, d0-d1,a0
	// Jump into the play routine
	writeAddress(0x001038U, uint16_t{0x207aU});
	writeAddress(0x00103aU, uint16_t{0xffcaU}); // movea $-36(pc), a0
	writeAddress(0x00103cU, uint16_t{0x4e90U}); // jsr (a0)
	// Finally, return from the interrupt handler
	writeAddress(0x00103eU, uint16_t{0x4e73U}); // rte

	// Configure the IRQ handler to our routine, and write the entry address in the SNDH where it can
	// get to it to `bsr` into the routine
	writeAddress(0x001004U, uint32_t{0x010008U});
	writeAddress(vectorAddress, uint32_t{0x001008U});

	// Write the initial timer settings to the timer
	mfp->configureTimer(index, static_cast<uint8_t>(counter.quotient), mode);
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

	// If the CPU isn't halted, run another instruction and see how many cycles that advanced us by
	if (cpu.readProgramCounter() != 0xffffffffU || cpu.hasPendingInterrupts())
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

	return true;
}

bool atariSTe_t::sampleReady() const noexcept
	{ return psg->sampleReady(); }

int16_t atariSTe_t::readSample() noexcept
{
	// Extract the sample from the PSG
	const auto psgSample{psg->sample()};
	// Extract the sample from the STe DMA DAC engine
	const auto dmaSample{dac->sample(*this)};
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
		}(psgSample + dmaSample)
	};
	// Scale the sample by the output level set via the DAC block LMC1992 and return
	// NB: the max output level is 64, allowing this to be simplified by the compiler and fast
	return (int32_t{sample} * dac->outputLevel()) / 64;
}

void atariSTe_t::displayCPUState() const noexcept
	{ cpu.displayRegs(); }
