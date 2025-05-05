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

	void testConfigureTimer()
	{
		// Ask the STe to configure Timer C for 200Hz operation
		emulator.configureTimer('C', 200U);
		// Read back the interrupt vector address to make sure it points at the timer routine
		assertEqual(mmio.readAddress<uint32_t>(0x000114U), 0x001008U);
		// Now read back the MFP settings for Timer C to check they're correct
		assertEqual(mmio.readAddress<uint8_t>(0xfffa23U), 192U);
		assertEqual(mmio.readAddress<uint8_t>(0xfffa1dU), 0x50U);
		// Read back the settings area and make sure those values are correct
		assertEqual(mmio.readAddress<uint8_t>(0x001000U), 64U);
		assertEqual(mmio.readAddress<uint8_t>(0x001001U), 192U);
		assertEqual(mmio.readAddress<uint8_t>(0x001002U), 0U);
		assertEqual(mmio.readAddress<uint8_t>(0x001003U), 0U);

		// Turn the timer back off
		mmio.writeAddress(0xfffa1dU, uint8_t{0x00U});

		// Now ask the STe to reconfigure Timer C for 50Hz operation
		emulator.configureTimer('C', 50U);
		// Read back the interrupt vector address to make sure it (still) points at the timer routine
		assertEqual(mmio.readAddress<uint32_t>(0x000114U), 0x001008U);
		// Now read back the MFP settings for Timer C to check they're correct
		assertEqual(mmio.readAddress<uint8_t>(0xfffa23U), 245U);
		assertEqual(mmio.readAddress<uint8_t>(0xfffa1dU), 0x70U);
		// Read back the settings area and make sure those values are correct
		assertEqual(mmio.readAddress<uint8_t>(0x001000U), 200U);
		assertEqual(mmio.readAddress<uint8_t>(0x001001U), 245U);
		assertEqual(mmio.readAddress<uint8_t>(0x001002U), 152U);
		assertEqual(mmio.readAddress<uint8_t>(0x001003U), 0U);

		// Turn the timer back off
		mmio.writeAddress(0xfffa1dU, uint8_t{0x00U});
	}

public:
	void registerTests() final
	{
		CXX_TEST(testMemoryMap)
		CXX_TEST(testConfigureTimer)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testAtariSTe>();
}
