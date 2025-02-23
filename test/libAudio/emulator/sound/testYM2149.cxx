// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <crunch++.h>
#include <substrate/index_sequence>
#include "emulator/sound/ym2149.hxx"
#include "emulator/unitsHelpers.hxx"

void writeRegister(peripheral_t<uint32_t> &psg, uint8_t reg, uint8_t value) noexcept
{
	psg.writeAddress(0U, {&reg, 1U});
	psg.writeAddress(2U, {&value, 1U});
}

class testYM2149 final : public testsuite
{
	void testToneWithEnvelope()
	{
		// Set up a PSG to generate 48kHz audio, and configure the channels and mixer settings
		ym2149_t psg{2_MHz, 48_kHz};
		// Write channel configs, A = 638, B = 0, C = 0
		writeRegister(psg, 0U, 0x7eU); // ChA fine
		writeRegister(psg, 1U, 0x02U); // ChA rough
		writeRegister(psg, 2U, 0U);
		writeRegister(psg, 3U, 0U);
		writeRegister(psg, 4U, 0U);
		writeRegister(psg, 5U, 0U);
		// Noise freq = 0
		writeRegister(psg, 6U, 0U);
		// Mixer config - enable only tone channel A
		writeRegister(psg, 7U, 0x3eU);
		// Write channel level configs
		writeRegister(psg, 8U, 0x10U);
		writeRegister(psg, 9U, 0U);
		writeRegister(psg, 10U, 0U);
		// And the envelope config, freq = 28, shape = 0xa
		writeRegister(psg, 11U, 0x1cU); // fine adjust
		writeRegister(psg, 12U, 0x00U); // rough adjust
		writeRegister(psg, 13U, 0x0aU); // shape

		assertFalse(psg.sampleReady());
		// Having set everything up, run for enough cycles to generate a sample
		for (const auto cycle : substrate::indexSequence_t{41U})
		{
			assertTrue(psg.clockCycle());
			if (cycle == 40U)
				assertTrue(psg.sampleReady());
			else
				assertFalse(psg.sampleReady());
		}
		// Now check the value of the sample
		assertEqual(psg.sample(), 0x2ad7);
	}

public:
	void registerTests() final
	{
		CXX_TEST(testToneWithEnvelope)
	}
};

// A level = 0x10, B level = 0, C level = 0, env freq = 28, shape = 0xa

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testYM2149>();
}
