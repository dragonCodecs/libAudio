// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <crunch++.h>
#include "emulator/memoryMap.hxx"
#include "emulator/cpu/m68k.hxx"
#include "emulator/atariSTeROMs.hxx"
#include "emulator/ram.hxx"
#include "emulator/unitsHelpers.hxx"

using m68kMemoryMap_t = memoryMap_t<uint32_t, 0x00ffffffU>;

class testAtariSTeROMs final : public testsuite, m68kMemoryMap_t
{
	constexpr static uint32_t heapBase{0x080000U};
	constexpr static uint32_t heapSize{0x080000U};
	motorola68000_t cpu{*this, 8_MHz};

	void testAllocator()
	{
		// Set up to allocate 4KiB with Malloc()
		writeAddress(0x000100U, uint16_t{0x2f3cU});
		writeAddress(0x000102U, uint32_t{0x00001000U}); // move.l #4096, -(sp)
		writeAddress(0x000106U, uint16_t{0x3f3cU});
		writeAddress(0x000108U, uint16_t{0x0048U}); // move.w #$48, -(sp)
		writeAddress(0x00010aU, uint16_t{0x4e41U}); // trap #1
		writeAddress(0x00010cU, uint16_t{0x5c8fU}); // addq.l #6, sp
		writeAddress(0x00010eU, uint16_t{0x4e75U}); // rts
		// Set the CPU to execute this sequence and run it
		assertTrue(cpu.executeToReturn(0x00000100U, 0x00080000U, false));
		// Extract the allocation address returned and check it's correct
		assertEqual(cpu.readDataRegister(0U), 0x00080000U);

		// Set up to allocate 2KiB with Mxalloc()
		writeAddress(0x000100U, uint16_t{0x4267U}); // clr.w -(sp)
		writeAddress(0x000102U, uint16_t{0x2f3cU});
		writeAddress(0x000104U, uint32_t{0x00000800U}); // move.l #2048, -(sp)
		writeAddress(0x000108U, uint16_t{0x3f3cU});
		writeAddress(0x00010aU, uint16_t{0x0044U}); // move.w #$44, -(sp)
		writeAddress(0x00010cU, uint16_t{0x4e41U}); // trap #1
		writeAddress(0x00010eU, uint16_t{0x508fU}); // addq.l #8, sp
		writeAddress(0x000110U, uint16_t{0x4e75U}); // rts
		// Set the CPU to execute this sequence and run it
		assertTrue(cpu.executeToReturn(0x00000100U, 0x00080000U, false));
		// Extract the allocation address returned and check it's correct
		assertEqual(cpu.readDataRegister(0U), 0x00081004U);

		// Check how much heap memory is free after that
		writeAddress(0x000100U, uint16_t{0x2f3cU});
		writeAddress(0x000102U, uint32_t{0xffffffffU}); // move.l #-1, -(sp)
		writeAddress(0x000106U, uint16_t{0x3f3cU});
		writeAddress(0x000108U, uint16_t{0x0048U}); // move.w #$48, -(sp)
		writeAddress(0x00010aU, uint16_t{0x4e41U}); // trap #1
		writeAddress(0x00010cU, uint16_t{0x5c8fU}); // addq.l #6, sp
		writeAddress(0x00010eU, uint16_t{0x4e75U}); // rts
		// Set the CPU to execute this sequence and run it
		assertTrue(cpu.executeToReturn(0x00000100U, 0x00080000U, false));
		// Extract the largest free block size returned and check it's correct
		assertEqual(cpu.readDataRegister(0U), 0x0007e7f8U);

		// Try to allocate 0 bytes and check that fails properly
		writeAddress(0x000100U, uint16_t{0x2f3cU});
		writeAddress(0x000102U, uint32_t{0x00000000U}); // move.l #0, -(sp)
		writeAddress(0x000106U, uint16_t{0x3f3cU});
		writeAddress(0x000108U, uint16_t{0x0048U}); // move.w #$48, -(sp)
		writeAddress(0x00010aU, uint16_t{0x4e41U}); // trap #1
		writeAddress(0x00010cU, uint16_t{0x5c8fU}); // addq.l #6, sp
		writeAddress(0x00010eU, uint16_t{0x4e75U}); // rts
		// Set the CPU to execute this sequence and run it
		assertTrue(cpu.executeToReturn(0x00000100U, 0x00080000U, false));
		// Extract the allocation address returned and check it's correct
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);

		// Try to allocate more heap memory than we have and check that fails properly
		writeAddress(0x000100U, uint16_t{0x2f3cU});
		writeAddress(0x000102U, uint32_t{0x00800000U}); // move.l #$00800000, -(sp)
		writeAddress(0x000106U, uint16_t{0x3f3cU});
		writeAddress(0x000108U, uint16_t{0x0048U}); // move.w #$48, -(sp)
		writeAddress(0x00010aU, uint16_t{0x4e41U}); // trap #1
		writeAddress(0x00010cU, uint16_t{0x5c8fU}); // addq.l #6, sp
		writeAddress(0x00010eU, uint16_t{0x4e75U}); // rts
		// Set the CPU to execute this sequence and run it
		assertTrue(cpu.executeToReturn(0x00000100U, 0x00080000U, false));
		// Extract the allocation address returned and check it's correct
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);

		// Try to allocate the entire heap in one shot and check that fails properly
		writeAddress(0x000100U, uint16_t{0x2f3cU});
		writeAddress(0x000102U, uint32_t{0x0007fff8U}); // move.l #$0007fff8, -(sp)
		writeAddress(0x000106U, uint16_t{0x3f3cU});
		writeAddress(0x000108U, uint16_t{0x0048U}); // move.w #$48, -(sp)
		writeAddress(0x00010aU, uint16_t{0x4e41U}); // trap #1
		writeAddress(0x00010cU, uint16_t{0x5c8fU}); // addq.l #6, sp
		writeAddress(0x00010eU, uint16_t{0x4e75U}); // rts
		// Set the CPU to execute this sequence and run it
		assertTrue(cpu.executeToReturn(0x00000100U, 0x00080000U, false));
		// Extract the allocation address returned and check it's correct
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);

		// Try to free our first allocation with Mfree()
		writeAddress(0x000100U, uint16_t{0x2f3cU});
		writeAddress(0x000102U, uint32_t{0x00080000U}); // move.l #0, -(sp)
		writeAddress(0x000106U, uint16_t{0x3f3cU});
		writeAddress(0x000108U, uint16_t{0x0049U}); // move.w #$49, -(sp)
		writeAddress(0x00010aU, uint16_t{0x4e41U}); // trap #1
		writeAddress(0x00010cU, uint16_t{0x5c8fU}); // addq.l #6, sp
		writeAddress(0x00010eU, uint16_t{0x4e75U}); // rts
		// Set the CPU to execute this sequence and run it
		assertTrue(cpu.executeToReturn(0x00000100U, 0x00080000U, false));
		// Check that the result was E_OK
		assertEqual(cpu.readDataRegister(0U), 0x00000000U);

		// Set up to allocate 2KiB with Malloc() to check it uses a block from the free list
		writeAddress(0x000100U, uint16_t{0x2f3cU});
		writeAddress(0x000102U, uint32_t{0x00000800U}); // move.l #2048, -(sp)
		writeAddress(0x000106U, uint16_t{0x3f3cU});
		writeAddress(0x000108U, uint16_t{0x0048U}); // move.w #$48, -(sp)
		writeAddress(0x00010aU, uint16_t{0x4e41U}); // trap #1
		writeAddress(0x00010cU, uint16_t{0x5c8fU}); // addq.l #6, sp
		writeAddress(0x00010eU, uint16_t{0x4e75U}); // rts
		// Set the CPU to execute this sequence and run it
		assertTrue(cpu.executeToReturn(0x00000100U, 0x00080000U, false));
		// Extract the allocation address returned and check it's correct
		assertEqual(cpu.readDataRegister(0U), 0x00080000U);

		// Set up to allocate 2KiB with Malloc() to check what happens when there's not enough free in
		// a non-empty free list
		writeAddress(0x000100U, uint16_t{0x2f3cU});
		writeAddress(0x000102U, uint32_t{0x00000800U}); // move.l #2048, -(sp)
		writeAddress(0x000106U, uint16_t{0x3f3cU});
		writeAddress(0x000108U, uint16_t{0x0048U}); // move.w #$48, -(sp)
		writeAddress(0x00010aU, uint16_t{0x4e41U}); // trap #1
		writeAddress(0x00010cU, uint16_t{0x5c8fU}); // addq.l #6, sp
		writeAddress(0x00010eU, uint16_t{0x4e75U}); // rts
		// Set the CPU to execute this sequence and run it
		assertTrue(cpu.executeToReturn(0x00000100U, 0x00080000U, false));
		// Extract the allocation address returned and check it's correct
		assertEqual(cpu.readDataRegister(0U), 0x00081808U);

		// Set up to allocate 1KiB with Malloc() to check it still uses a block from the free list
		writeAddress(0x000100U, uint16_t{0x2f3cU});
		writeAddress(0x000102U, uint32_t{0x00000400U}); // move.l #1024, -(sp)
		writeAddress(0x000106U, uint16_t{0x3f3cU});
		writeAddress(0x000108U, uint16_t{0x0048U}); // move.w #$48, -(sp)
		writeAddress(0x00010aU, uint16_t{0x4e41U}); // trap #1
		writeAddress(0x00010cU, uint16_t{0x5c8fU}); // addq.l #6, sp
		writeAddress(0x00010eU, uint16_t{0x4e75U}); // rts
		// Set the CPU to execute this sequence and run it
		assertTrue(cpu.executeToReturn(0x00000100U, 0x00080000U, false));
		// Extract the allocation address returned and check it's correct
		assertEqual(cpu.readDataRegister(0U), 0x00080804U);
	}

public:
	CRUNCH_VIS testAtariSTeROMs() noexcept : testsuite{}, m68kMemoryMap_t{}
	{
		// Register some memory and the ROMs for the tests to use
		addressMap[{0x000000U, 0x100000U}] = std::make_unique<ram_t<uint32_t, 1_MiB>>();
		addressMap[{0x100000U, 0x200000U}] = std::make_unique<atariSTeROMs_t>
		(
			cpu, static_cast<m68kMemoryMap_t &>(*this), heapBase, heapSize
		);

		// Set up the GEMDOS TRAP handler so we can use it in the tests
		writeAddress(0x000084U, uint32_t{0x100000U + atariSTeROMs_t::handlerAddressGEMDOS});
	}

	void registerTests() final
	{
		CXX_TEST(testAllocator)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testAtariSTeROMs>();
}
