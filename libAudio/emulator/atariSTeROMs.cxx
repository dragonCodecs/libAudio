// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <substrate/span>
#include <substrate/buffer_utils>
#include "atariSTeROMs.hxx"
#include "cpu/m68k.hxx"

atariSTeROMs_t::atariSTeROMs_t(motorola68000_t &cpu, memoryMap_t<uint32_t, 0x00ffffffU> &peripherals,
	const uint32_t heapBase, const uint32_t heapSize) noexcept : peripheral_t<uint32_t>{}, _cpu{cpu},
	_peripherals{peripherals}, allocator{heapBase, heapSize} { }

void atariSTeROMs_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	// Extract how wide an access this is
	const auto accessWidth{data.size_bytes()};
	// We only support u16 access to the "ROM"
	if (accessWidth != 2U)
		return;
	// If this is an access to GEMDOS via a `trap` instruction, dispatch that
	if (address == handlerAddressGEMDOS)
	{
		handleGEMDOSAccess();
		// And then load an RTE as the result of this read
		writeBE(uint16_t{0x4e73U}, data);
	}
}

void atariSTeROMs_t::writeAddress(const uint32_t, const substrate::span<uint8_t> &) noexcept
{
	// This function is intentionally a no-op as this is a ROM!
}

void atariSTeROMs_t::handleGEMDOSAccess() const noexcept
{
	// Extract the current stack pointer
	const auto stackPointer{_cpu.readAddrRegister(7U)};
	// Figure out where on the stack the opcode value is and extract it
	const auto opcode{_peripherals.readAddress<uint16_t>(stackPointer + 8U)};
	// Dispatch the opcode requested
	switch (opcode)
	{
		case 0x30U: // GEMDOS version number
			// Respond with being version 0.30 via D0
			_cpu.writeDataRegister(0U, 0x3000U);
			break;
		default:
			// Unimplemented GEMDOS operation, explode
			// NOLINTNEXTLINE(clang-diagnostic-exceptions)
			throw std::exception{};
	}
}
