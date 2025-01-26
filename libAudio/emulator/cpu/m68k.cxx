// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include "m68k.hxx"

constexpr static uint16_t insnMask{0xf1f8U};
constexpr static uint16_t insnMaskEA{0xf1c0U};
constexpr static size_t regXShift{9U};
constexpr static uint16_t regMask{0x0007U};
constexpr static uint16_t rmMask{0x0008U};
constexpr static uint16_t irMask{0x0020U};
constexpr static uint16_t sizeMask{0x00c0U};
constexpr static size_t sizeShift{6U};
constexpr static uint16_t conditionMask{0x0f00U};
constexpr static size_t conditionShift{8U};

motorola68000_t::motorola68000_t(memoryMap_t<uint32_t> &peripherals, const uint64_t clockFreq) noexcept :
	_peripherals{peripherals}, clockFrequency{clockFreq}
{
}

decodedOperation_t motorola68000_t::decodeInstruction(const uint16_t insn) const noexcept
{
	switch (insn & insnMask)
	{
		case 0xc100U:
		case 0xc108U:
			return
			{
				instruction_t::abcd,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & rmMask)
						return {operationFlags_t::memoryNotRegister};
					return {};
				}(),
			};
		case 0xd100U:
		case 0xd108U:
		case 0xd140U:
		case 0xd148U:
		case 0xd180U:
		case 0xd188U:
			return
			{
				instruction_t::addx,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & rmMask)
						return {operationFlags_t::memoryNotRegister};
					return {};
				}(),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xe000U:
		case 0xe020U:
		case 0xe040U:
		case 0xe060U:
		case 0xe080U:
		case 0xe0a0U:
			return
			{
				instruction_t::asr,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & irMask)
						return {operationFlags_t::registerNotImmediate};
					return {};
				}(),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xe100U:
		case 0xe120U:
		case 0xe140U:
		case 0xe160U:
		case 0xe180U:
		case 0xe1a0U:
			return
			{
				instruction_t::asl,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & irMask)
						return {operationFlags_t::registerNotImmediate};
					return {};
				}(),
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
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0x50c8U:
		case 0x51c8U:
			return
			{
				instruction_t::dbcc,
				uint8_t((insn & conditionMask) >> conditionShift),
				uint8_t(insn & regMask),
				{},
				0U, 0U, 0U,
				2U, // 16-bit displacement follows
			};
		case 0xc140U:
		case 0xc141U:
		case 0xc181U:
			return
			{
				instruction_t::exg,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{},
				0U,
				// Extract out the operating mode
				uint8_t((insn & 0x00f8U) >> 2U),
			};
		case 0xe001U:
		case 0xe021U:
		case 0xe041U:
		case 0xe061U:
		case 0xe081U:
		case 0xe0a1U:
			return
			{
				instruction_t::lsr,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & irMask)
						return {operationFlags_t::registerNotImmediate};
					return {};
				}(),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xe101U:
		case 0xe121U:
		case 0xe141U:
		case 0xe161U:
		case 0xe181U:
		case 0xe1a1U:
			return
			{
				instruction_t::lsl,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & irMask)
						return {operationFlags_t::registerNotImmediate};
					return {};
				}(),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0x0101U:
		case 0x0141U:
		case 0x0181U:
		case 0x01c1U:
			return
			{
				instruction_t::movep,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{operationFlags_t::memoryNotRegister},
				// Extract whether we're working with a u16 or a u32
				uint8_t((insn & 0x0040U) ? 4U : 2U),
				// Extract the operation direction information
				uint8_t((insn & 0x0080U) >> 7U),
				0U,
				2U, // 16-bit displacement follows
			};
		case 0x8140U:
		case 0x8148U:
			return
			{
				instruction_t::pack,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & rmMask)
						return {operationFlags_t::memoryNotRegister};
					return {};
				}(),
				0U, 0U, 0U,
				2U, // 16-bit adjustment extension follows
			};
		case 0xe018U:
		case 0xe038U:
		case 0xe058U:
		case 0xe078U:
		case 0xe098U:
		case 0xe0b8U:
			return
			{
				instruction_t::ror,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & irMask)
						return {operationFlags_t::registerNotImmediate};
					return {};
				}(),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xe118U:
		case 0xe138U:
		case 0xe158U:
		case 0xe178U:
		case 0xe198U:
		case 0xe1b8U:
			return
			{
				instruction_t::rol,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & irMask)
						return {operationFlags_t::registerNotImmediate};
					return {};
				}(),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xe010U:
		case 0xe030U:
		case 0xe050U:
		case 0xe070U:
		case 0xe090U:
		case 0xe0b0U:
			return
			{
				instruction_t::roxr,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & irMask)
						return {operationFlags_t::registerNotImmediate};
					return {};
				}(),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xe110U:
		case 0xe130U:
		case 0xe150U:
		case 0xe170U:
		case 0xe190U:
		case 0xe1b0U:
			return
			{
				instruction_t::roxl,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & irMask)
						return {operationFlags_t::registerNotImmediate};
					return {};
				}(),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0x8100U:
		case 0x8108U:
			return
			{
				instruction_t::sbcd,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & rmMask)
						return {operationFlags_t::memoryNotRegister};
					return {};
				}(),
			};
		case 0x9100U:
		case 0x9108U:
		case 0x9140U:
		case 0x9148U:
		case 0x9180U:
		case 0x9188U:
			return
			{
				instruction_t::subx,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & rmMask)
						return {operationFlags_t::memoryNotRegister};
					return {};
				}(),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0x8180U:
		case 0x8188U:
			return
			{
				instruction_t::unpk,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				[&]() -> decltype(decodedOperation_t::flags)
				{
					if (insn & rmMask)
						return {operationFlags_t::memoryNotRegister};
					return {};
				}(),
				0U, 0U, 0U,
				2U, // 16-bit adjustment extension follows
			};
	}
	return {instruction_t::illegal};
}
