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
