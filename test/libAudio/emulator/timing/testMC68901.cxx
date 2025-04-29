// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <crunch++.h>
#include "emulator/memoryMap.hxx"
#include "emulator/cpu/m68k.hxx"
#include "emulator/timing/mc68901.hxx"
#include "emulator/unitsHelpers.hxx"

using m68kMemoryMap_t = memoryMap_t<uint32_t, 0x00ffffffU>;

void writeRegister(peripheral_t<uint32_t> &periph, uint8_t reg, uint8_t value) noexcept
{
	periph.writeAddress(reg, {&value, 1U});
}

void writeRegister(peripheral_t<uint32_t> &periph, uint8_t reg, uint16_t value) noexcept
{
	std::array<uint8_t, 2> data{};
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

class testMC68901 final : public testsuite, m68kMemoryMap_t
{
	motorola68000_t cpu{*this, 8_MHz};
	mc68901_t mfp{2457600U, cpu, 6U};

	void testRegisterIO()
	{
		writeRegister(mfp, 0x00U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x00U), 0x00ffU);
		assertEqual(readRegister<uint8_t>(mfp, 0x01U), 0xffU);
		writeRegister(mfp, 0x01U, uint8_t{0x00U});
		assertEqual(readRegister<uint8_t>(mfp, 0x01U), 0x00U);
		writeRegister(mfp, 0x02U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x02U), 0x00ffU);
		writeRegister(mfp, 0x04U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x04U), 0x00ffU);
		writeRegister(mfp, 0x06U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x06U), 0x00ffU);
		writeRegister(mfp, 0x08U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x08U), 0x00ffU);
		writeRegister(mfp, 0x0aU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x0aU), 0x00ffU);
		writeRegister(mfp, 0x0cU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x0cU), 0x00ffU);
		writeRegister(mfp, 0x0eU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x0eU), 0x00ffU);
		writeRegister(mfp, 0x10U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x10U), 0x00ffU);
		writeRegister(mfp, 0x12U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x12U), 0x00ffU);
		writeRegister(mfp, 0x14U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x14U), 0x00ffU);
		writeRegister(mfp, 0x16U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x16U), 0x00f8U);
		writeRegister(mfp, 0x18U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x18U), 0x001fU);
		writeRegister(mfp, 0x1eU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x1eU), 0x0000U);
		writeRegister(mfp, 0x19U, uint8_t{0x00U});
		writeRegister(mfp, 0x1aU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x1aU), 0x001fU);
		writeRegister(mfp, 0x20U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x20U), 0x0000U);
		writeRegister(mfp, 0x1bU, uint8_t{0x00U});
		writeRegister(mfp, 0x1cU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x1cU), 0x0077U);
		writeRegister(mfp, 0x22U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x22U), 0x0000U);
		writeRegister(mfp, 0x24U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x24U), 0x0000U);
		writeRegister(mfp, 0x1dU, uint8_t{0x00U});
		writeRegister(mfp, 0x1eU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x1eU), 0x00ffU);
		writeRegister(mfp, 0x20U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x20U), 0x00ffU);
		writeRegister(mfp, 0x22U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x22U), 0x00ffU);
		writeRegister(mfp, 0x24U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x24U), 0x00ffU);
		writeRegister(mfp, 0x26U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x26U), 0x00ffU);
		writeRegister(mfp, 0x28U, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x28U), 0x00ffU);
		writeRegister(mfp, 0x2aU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x2aU), 0x00ffU);
		writeRegister(mfp, 0x2cU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x2cU), 0x00ffU);
		writeRegister(mfp, 0x2eU, uint16_t{0xffffU});
		assertEqual(readRegister<uint16_t>(mfp, 0x2eU), 0x00ffU);
	}

public:
	CRUNCH_VIS testMC68901() noexcept : testsuite{}, m68kMemoryMap_t{} { }

	void registerTests() final
	{
		CXX_TEST(testRegisterIO)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMC68901>();
}
