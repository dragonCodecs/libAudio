// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <crunch++.h>
#include <substrate/index_sequence>
#include "emulator/memoryMap.hxx"
#include "emulator/cpu/m68k.hxx"
#include "emulator/timing/mc68901.hxx"
#include "emulator/sound/steDAC.hxx"
#include "emulator/ram.hxx"
#include "emulator/unitsHelpers.hxx"

using m68kMemoryMap_t = memoryMap_t<uint32_t, 0x00ffffffU>;

template<typename T> void writeRegister(peripheral_t<uint32_t> &periph, const uint8_t reg, const T value) noexcept
{
	std::array<uint8_t, sizeof(T)> data{};
	if constexpr (sizeof(T) == 1)
		data[0] = value;
	else
		writeBE(value, data);
	periph.writeAddress(reg, data);
}

template<typename T> T readRegister(peripheral_t<uint32_t> &periph, uint8_t reg) noexcept
{
	std::array<uint8_t, sizeof(T)> result{};
	periph.readAddress(reg, result);
	if constexpr (sizeof(T) == 1)
		return result[0U];
	else
		return readBE<T>(result);
}

class testSTeDAC final : public testsuite, m68kMemoryMap_t
{
	motorola68000_t cpu{*this, 8_MHz};
	mc68901_t mfp{2457600U, cpu, 6U};
	steDAC_t dac{50_kHz + 66U, mfp};

	void runMicrowireCycle(uint16_t mask)
	{
		// Microwire cycles take 16 reads of the mask register
		for (const auto &cycle : substrate::indexSequence_t{16U})
		{
			// Compute what the next mask value should be
			mask = (mask << 1U) | (mask >> 15U);
			// Read the mask register (which actions the transaction)
			assertEqual(readRegister<uint16_t>(dac, 0x24U), mask);
		}
	}

	void testMicrowire()
	{
		// Set up a write to the main volume register
		writeRegister(dac, 0x24U, uint16_t{0x0ffeU});
		assertEqual(readRegister<uint16_t>(dac, 0x24U), 0x0ffeU);
		// Set the volume to 0
		writeRegister(dac, 0x22U, uint16_t{0x0981U});
		assertEqual(readRegister<uint16_t>(dac, 0x22U), 0x0981U);
		// Execute the transaction
		runMicrowireCycle(0x0ffeU);
		// Read the main volume to check what it's now set to (should be 0)
		assertEqual(dac.outputLevel(), 0x00U);

		// Set up another write to the main volume register to set the volume to max (40 internally, 64 externally)
		writeRegister(dac, 0x22U, uint16_t{0x09d1U});
		assertEqual(readRegister<uint16_t>(dac, 0x22U), 0x09d1U);
		// Execute the transaction
		runMicrowireCycle(0x0ffeU);
		// Read the main volume to check what it's now set to (should be 40)
		assertEqual(dac.outputLevel(), 0x40U);

		// Set up another write to the main volume register to set the volume to mid (20 internally, 32 externally)
		writeRegister(dac, 0x22U, uint16_t{0x09a9U});
		assertEqual(readRegister<uint16_t>(dac, 0x22U), 0x09a9U);
		// Execute the transaction
		runMicrowireCycle(0x0ffeU);
		// Read the main volume to check what it's now set to (should be 40)
		assertEqual(dac.outputLevel(), 0x20U);

		// Set up another write to the main volume register to over-set the volume to 63
		writeRegister(dac, 0x22U, uint16_t{0x09ffU});
		assertEqual(readRegister<uint16_t>(dac, 0x22U), 0x09ffU);
		// Execute the transaction
		runMicrowireCycle(0x0ffeU);
		// Read the main volume to check what it's now set to (should be 64)
		assertEqual(dac.outputLevel(), 0x40U);

		// Set up a write to the left front fader register
		writeRegister(dac, 0x22U, uint16_t{0x0aa9U});
		assertEqual(readRegister<uint16_t>(dac, 0x22U), 0x0aa9U);
		// Execute the transaction
		runMicrowireCycle(0x0ffeU);
		// Read the main volume to check what it's now set to (should be 64)
		assertEqual(dac.outputLevel(), 0x40U);

		// Set up a write to the right front fader register
		writeRegister(dac, 0x22U, uint16_t{0x0a29U});
		assertEqual(readRegister<uint16_t>(dac, 0x22U), 0x0a29U);
		// Execute the transaction
		runMicrowireCycle(0x0ffeU);
		// Read the main volume to check what it's now set to (should be 64)
		assertEqual(dac.outputLevel(), 0x40U);
	}

	void testDMARegisterIO()
	{
		writeRegister(dac, 0x00U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x00U), 0x0003U);
		writeRegister(dac, 0x02U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x02U), 0x00ffU);
		writeRegister(dac, 0x04U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x04U), 0x00ffU);
		writeRegister(dac, 0x06U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x06U), 0x00feU);
		writeRegister(dac, 0x08U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x08U), 0x00ffU);
		writeRegister(dac, 0x0aU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x0aU), 0x00ffU);
		writeRegister(dac, 0x0cU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x0cU), 0x00feU);
		writeRegister(dac, 0x0eU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x0eU), 0x00ffU);
		writeRegister(dac, 0x10U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x10U), 0x00ffU);
		writeRegister(dac, 0x12U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x12U), 0x00feU);
		writeRegister(dac, 0x20U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(dac, 0x20U), 0x0083U);
	}

	void testBadDMARegisterIO()
	{
		// 8-bit writes to even numbered registers should be discarded as all 8-bit registers here are odd byte
		writeRegister(dac, 0x00U, uint8_t{0xffU});
		assertEqual(readRegister<uint16_t>(dac, 0x00U), 0x0003U);
		// 16-bit writes to odd addresses should be inadmissable
		writeRegister(dac, 0x01U, uint16_t{0x0000U});
		assertEqual(readRegister<uint8_t>(dac, 0x01U), 0x03U);
		// 32-bit writes similarly are inadmissable and should be discarded
		writeRegister(dac, 0x00U, uint32_t{0x00000000U});
		assertEqual(readRegister<uint8_t>(dac, 0x01U), 0x03U);
		assertEqual(readRegister<uint8_t>(dac, 0x03U), 0xffU);
		// 32-bit reads should also be inadmissable and discarded
		assertEqual(readRegister<uint32_t>(dac, 0x00U), 0x00000000U);
		// Likewise 16-bit odd address reads
		assertEqual(readRegister<uint16_t>(dac, 0x01U), 0x0000U);
		// And 8-bit even reads
		assertEqual(readRegister<uint8_t>(dac, 0x00U), 0x00U);
	}

	void test50kSampleLoop()
	{
		// Set the DMA controller up to stream the first 254 bytes of RAM out in a loop at
		// the fastest playback rate (50066Hz, ~50kHz), marked as stopped initially
		writeRegister(dac, 0x01U, uint8_t{0x02U});
		writeRegister(dac, 0x03U, uint8_t{0x00U});
		writeRegister(dac, 0x05U, uint8_t{0x00U});
		writeRegister(dac, 0x07U, uint8_t{0x00U});
		writeRegister(dac, 0x0fU, uint8_t{0x00U});
		writeRegister(dac, 0x11U, uint8_t{0x00U});
		writeRegister(dac, 0x13U, uint8_t{0xfeU});
		writeRegister(dac, 0x21U, uint8_t{0x83U});
		// Run one clock cycle and make sure the value emitted is a silence sample
		assertTrue(dac.clockCycle());
		assertEqual(dac.sample(*this), 0);
		// Now enable playback
		writeRegister(dac, 0x01U, uint8_t{0x03U});
		// Check that the sample sequence is returned
		for (const auto &sample : substrate::indexSequence_t{1U, 255U})
		{
			assertEqual(dac.sample(*this), static_cast<int8_t>(sample) * 64);
			assertTrue(dac.clockCycle());
		}
		// Then that the second iteration through the loop works and does the same thing
		for (const auto &sample : substrate::indexSequence_t{1U, 255U})
		{
			assertEqual(dac.sample(*this), static_cast<int8_t>(sample) * 64);
			assertTrue(dac.clockCycle());
		}
	}

public:
	CRUNCH_VIS testSTeDAC() noexcept : testsuite{}, m68kMemoryMap_t{}
	{
		// Register some memory for the sample tests, and fill with a couple of patterns that make it easy to see
		// if things are working right within the sampler
		addressMap[{0x000000U, 0x000800U}] = std::make_unique<ram_t<uint32_t, 2_KiB>>();

		// Write a mono stream to the first 254 bytes of RAM
		for (const auto &sample : substrate::indexSequence_t{1U, 255U})
			writeAddress(0x000000U + (sample - 1U), static_cast<uint8_t>(sample));
		// Now a stereo stream to the upper 1KiB
		for (const auto &sample: substrate::indexSequence_t{1U, 255U})
		{
			const size_t offset{(sample - 1U) * 2U};
			writeAddress(0x000400U + offset + 0U, static_cast<uint8_t>(sample));
			writeAddress(0x000400U + offset + 1U, static_cast<uint8_t>(sample - 1U));
		}
	}

	void registerTests() final
	{
		CXX_TEST(testMicrowire)
		CXX_TEST(testDMARegisterIO)
		CXX_TEST(testBadDMARegisterIO)
		CXX_TEST(test50kSampleLoop)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testSTeDAC>();
}
