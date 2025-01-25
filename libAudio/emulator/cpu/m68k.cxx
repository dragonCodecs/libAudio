// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include "m68k.hxx"

constexpr static uint16_t insnMask{0xf1f0U};
constexpr static size_t regXShift{9U};
constexpr static uint16_t regMask{0x0007U};
constexpr static uint16_t rmMask{0x0008U};

motorola68000_t::motorola68000_t(memoryMap_t<uint32_t> &peripherals, const uint64_t clockFreq) noexcept :
	_peripherals{peripherals}, clockFrequency{clockFreq}
{
}

decodedOperation_t motorola68000_t::decodeInstruction(uint16_t insn) const noexcept
{
	switch (insn & insnMask)
	{
		case 0xc100U:
			return
			{
				instruction_t::abcd,
				bool(insn & rmMask),
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
			};
	}
	return {instruction_t::illegal};
}
