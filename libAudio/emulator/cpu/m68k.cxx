// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include "m68k.hxx"

constexpr static uint16_t insnMask{0xf1f8U};
constexpr static size_t regXShift{9U};
constexpr static uint16_t regMask{0x0007U};
constexpr static uint16_t rmMask{0x0008U};
constexpr static uint16_t irMask{0x0020U};
constexpr static uint16_t sizeMask{0x00c0U};
constexpr static size_t sizeShift{6U};

motorola68000_t::motorola68000_t(memoryMap_t<uint32_t> &peripherals, const uint64_t clockFreq) noexcept :
	_peripherals{peripherals}, clockFrequency{clockFreq}
{
}

decodedOperation_t motorola68000_t::decodeInstruction(uint16_t insn) const noexcept
{
	switch (insn & insnMask)
	{
		case 0xc100U:
		case 0xc108U:
			if (insn & rmMask)
				return
				{
					instruction_t::abcd,
					uint8_t((insn >> regXShift) & regMask),
					uint8_t(insn & regMask),
					{operationFlags_t::memoryNotRegister},
				};
			else
				return
				{
					instruction_t::abcd,
					uint8_t((insn >> regXShift) & regMask),
					uint8_t(insn & regMask),
				};
		case 0xd100U:
		case 0xd108U:
		case 0xd140U:
		case 0xd148U:
		case 0xd180U:
		case 0xd188U:
			if (insn & rmMask)
				return
				{
					instruction_t::addx,
					uint8_t((insn >> regXShift) & regMask),
					uint8_t(insn & regMask),
					{operationFlags_t::memoryNotRegister},
					uint8_t((insn & sizeMask) >> sizeShift),
				};
			else
				return
				{
					instruction_t::addx,
					uint8_t((insn >> regXShift) & regMask),
					uint8_t(insn & regMask),
					{},
					uint8_t((insn & sizeMask) >> sizeShift),
				};
		case 0xe000U:
		case 0xe020U:
		case 0xe040U:
		case 0xe060U:
		case 0xe080U:
		case 0xe0a0U:
			if (insn & irMask)
				return
				{
					instruction_t::asr,
					uint8_t((insn >> regXShift) & regMask),
					uint8_t(insn & regMask),
					{operationFlags_t::memoryNotRegister, operationFlags_t::registerNotImmediate},
					uint8_t((insn & sizeMask) >> sizeShift),
				};
			else
				return
				{
					instruction_t::asr,
					uint8_t((insn >> regXShift) & regMask),
					uint8_t(insn & regMask),
					{operationFlags_t::memoryNotRegister},
					uint8_t((insn & sizeMask) >> sizeShift),
				};
		case 0xb108U:
		case 0xb148U:
		case 0xb188U:
			return
			{
				instruction_t::cmpm,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{operationFlags_t::memoryNotRegister},
				uint8_t((insn & sizeMask) >> sizeShift),
			};
	}
	return {instruction_t::illegal};
}
