// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <crunch++.h>
#include "emulator/atariSTe.hxx"

using m68kMemoryMap_t = memoryMap_t<uint32_t, 0x00ffffffU>;

class testAtariSTe final : public testsuite
{
	atariSTe_t emulator{};
	// Convert the emulation instance to grab access to the memory map to read memory
	m68kMemoryMap_t &mmio{reinterpret_cast<m68kMemoryMap_t &>(emulator)};

	void testMemoryMap()
	{
		// Check that the RAM is present and the GEMDOS trap handler has been configured properly
		assertEqual(mmio.readAddress<uint32_t>(0x000084U), 0x00e00000U);
		// Check the MFP is present and the vector register has been configured properly
		assertEqual(mmio.readAddress<uint8_t>(0xfffa17U), 0x40U);
		// Check the DMA DAC is present and defaulted to play 50066Hz mono
		assertEqual(mmio.readAddress<uint8_t>(0xff8921U), 0x83U);
		// Check the YM2149 is present and the mixer configured for silence
		mmio.writeAddress(0xff8800U, uint8_t{0x07U});
		assertEqual(mmio.readAddress<uint8_t>(0xff8800U), 0x3fU);
	}

public:
	void registerTests() final
	{
		CXX_TEST(testMemoryMap)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testAtariSTe>();
}
