// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <string_view>
#include <crunch++.h>
#include <substrate/fd>
#include <substrate/index_sequence>
#include "sndh/iceDecrunch.hxx"
#include "emulator/atariSTe.hxx"

using namespace std::literals::string_view_literals;

using m68kMemoryMap_t = memoryMap_t<uint32_t, 0x00ffffffU>;

constexpr static auto sndhFileName{"atariSTe.sndh"sv};

class testAtariSTe final : public testsuite
{
	atariSTe_t emulator{};
	// Convert the emulation instance to grab access to the memory map to read memory
	m68kMemoryMap_t &mmio{reinterpret_cast<m68kMemoryMap_t &>(emulator)};

	sndhDecruncher_t sndh;

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

	void testCopyToRAM()
	{
		// Start by trying to read all of the SNDH data into RAM, having made sure it's valid
		assertTrue(sndh.valid());
		assertTrue(emulator.copyToRAM(sndh));
		// Having done this, put the SNDH source data back to the start
		assertTrue(sndh.head());
		// And try to read each byte back out of RAM and compare it to the source file data
		for (const auto offset : substrate::indexSequence_t{sndh.length()})
		{
			// Read the source byte from the SNDH data
			const auto expectedByte
			{
				[this]()
				{
					uint8_t result{};
					assertTrue(sndh.read(result));
					return result;
				}()
			};
			// Read the matching byte back out from memory and check it
			assertEqual(mmio.readAddress<uint8_t>(0x010000U + offset), expectedByte);
		}
	}

public:
	CRUNCH_VIS testAtariSTe() noexcept : testsuite{},
		sndh
		{
			[this]()
			{
				// Open the SNDH file of test data, and read it all into memory
				const fd_t sndhFile{sndhFileName, O_RDONLY | O_NOCTTY};
				assertNotEqual(sndhFile, -1);
				assertGreaterThan(sndhFile.length(), 0);
				// Use the decruncher to get something valid to use with copyToRAM() etc
				return sndhDecruncher_t{sndhFile};
			}()
		}
	{
	}

	void registerTests() final
	{
		CXX_TEST(testMemoryMap)
		CXX_TEST(testConfigureTimer)
		CXX_TEST(testCopyToRAM)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testAtariSTe>();
}
