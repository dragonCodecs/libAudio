// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <type_traits>
#include "atariSTe.hxx"
#include "ram.hxx"
#include "sound/ym2149.hxx"
#include "unitsHelpers.hxx"
#include "sndh/iceDecrunch.hxx"

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

			// Add the device to the clocking map according to the clocking ratio set up
			clockedPeripherals[peripheral.get()] = {systemClockFrequency, peripheral->clockFrequency()};
			// Add the device to the address map
			addressMap[addressRange] = std::move(peripheral);
		}
	};

	// Build the system memory map
	addressMap[{0x000000U, 0x800000U}] = std::make_unique<stRAM_t>();
	// TODO: TOS ROM needs to go at 0xe00000U, it is in a 1MiB window
	// Cartridge ROM at 0xfa0000, 128KiB
	// pre-TOS 2.0 OS ROMs at 0xfc0000, 128KiB
	addClockedPeripheral({0xff8800U, 0xff8804U}, std::make_unique<ym2149_t>(2_MHz));
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

bool atariSTe_t::play() noexcept
	{ return cpu.executeToReturn(0x001008U, 0x800000U, false); }

void atariSTe_t::displayCPUState() const noexcept
	{ cpu.displayRegs(); }
