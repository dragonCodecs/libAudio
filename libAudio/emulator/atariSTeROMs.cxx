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
		case 0x44U: // mxalloc (we ignore the mode value.. sp + 14U)
		case 0x48U: // malloc
		{
			// Set D0 in case of failure to a NULL pointer for the platform
			_cpu.writeDataRegister(0U, 0U);
			// Extract how large and allocation is being requested
			const auto amount{_peripherals.readAddress<int32_t>(stackPointer + 10U)};
			// If the amount requested is the special value -1, return the largest free block
			if (amount == -1)
				_cpu.writeDataRegister(0U, allocator.largestFreeBlock());
			// Otherwise if it's a non-zero positive amount to allocate, actually try to
			else if (amount > 0)
			{
				const auto result{allocator.alloc(static_cast<size_t>(amount))};
				// Check if it succeeded, and if so then write the new pointer into D0
				if (result != std::nullopt)
					_cpu.writeDataRegister(0U, *result);
			}
			break;
		}
		case 0x49U: // free
		{
			// Extract the pointer for the allocation being released
			const auto ptr{_peripherals.readAddress<int32_t>(stackPointer + 10U)};
			// Execute on that with our actual allocator
			if (allocator.free(ptr))
				// Deallocation succeeded, so return 0 (E_OK)
				_cpu.writeDataRegister(0U, 0U);
			else
				// Deallocation failed, signal this with a failure code
				_cpu.writeDataRegister(0U, UINT16_MAX);
			break;
		}
		default:
			// Unimplemented GEMDOS operation, explode
			// NOLINTNEXTLINE(clang-diagnostic-exceptions)
			throw std::exception{};
	}
}
