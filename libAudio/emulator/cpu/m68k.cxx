// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include "m68k.hxx"

constexpr static uint16_t insnMask{0xf1f8U};
constexpr static uint16_t insnMaskEA{0xf1c0U};
constexpr static uint16_t insnMaskEANoReg{0xffc0U};
constexpr static uint16_t insnMaskVector{0xfff8U};
constexpr static uint16_t insnMaskDisplacement{0xff00U};
constexpr static size_t regXShift{9U};
constexpr static uint16_t regMask{0x0007U};
constexpr static uint16_t rmMask{0x0008U};
constexpr static uint16_t irMask{0x0020U};
constexpr static uint16_t sizeMask{0x00c0U};
constexpr static size_t sizeShift{6U};
constexpr static uint16_t conditionMask{0x0f00U};
constexpr static size_t conditionShift{8U};
constexpr static uint16_t eaModeMask{0x0038U};
constexpr static size_t eaModeShift{3U};
constexpr static uint16_t vectorMask{0x0007U};
constexpr static uint16_t displacementMask{0x00ffU};

motorola68000_t::motorola68000_t(memoryMap_t<uint32_t> &peripherals, const uint64_t clockFreq) noexcept :
	_peripherals{peripherals}, clockFrequency{clockFreq}
{
}

decodedOperation_t motorola68000_t::decodeInstruction(const uint16_t insn) const noexcept
{
	// Decode instructions that have exact matches
	switch (insn)
	{
		case 0x023cU:
			return
			{
				instruction_t::andi,
				0U, 8U, // 8 is a special register number (not otherwise valid) indicating CCR.
				{},
				1U, 0U, 0U,
				2U, // 16-bit immediate follows (8 bits used)
			};
		case 0x0a3cU:
			return
			{
				instruction_t::eori,
				0U, 8U, // 8 is a special register number (not otherwise valid) indicating CCR.
				{},
				1U, 0U, 0U,
				2U, // 16-bit immediate follows (8 bits used)
			};
		case 0x49fcU:
			return {instruction_t::illegal};
		case 0x4e71U:
			return {instruction_t::nop};
		case 0x003cU:
			return
			{
				instruction_t::ori,
				0U, 8U, // 8 is a special register number (not otherwise valid) indicating CCR.
				{},
				1U, 0U, 0U,
				2U, // 16-bit immediate follows (8 bits used)
			};
		case 0x4e74U:
			return
			{
				instruction_t::rtd,
				0U, 0U, {}, 0U, 0U, 0U,
				2U, // 16-bit displacement follows
			};
		case 0x4e77U:
			return {instruction_t::rtr};
		case 0x4e75U:
			return {instruction_t::rts};
		case 0x4e76U:
			return {instruction_t::trapv};
	}

	// Decode instructions that use the `insn Ry, Rx` basic form
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

	// Decode instructions that use the effective address form
	switch (insn & insnMaskEA)
	{
		case 0xd000U:
		case 0xd040U:
		case 0xd080U:
		case 0xd100U:
		case 0xd140U:
		case 0xd180U:
			return
			{
				instruction_t::add,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				// Extract the operation direction information
				uint8_t((insn & 0x0100U) >> 8U),
				uint8_t((insn & eaModeMask) >> eaModeShift),
			};
		case 0xd0c0U:
		case 0xd1c0U:
			return
			{
				instruction_t::adda,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{},
				// Decode whether 16- or 32-bit operation
				uint8_t((insn & 0x01c0U) == 0x00c0U ? 2U : 4U),
				0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
			};
		case 0x5000U:
		case 0x5040U:
		case 0x5080U:
			return
			{
				instruction_t::addq,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{operationFlags_t::registerNotImmediate},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
			};
		case 0xc000U:
		case 0xc040U:
		case 0xc080U:
		case 0xc100U:
		case 0xc140U:
		case 0xc180U:
			return
			{
				instruction_t::_and,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				// Extract the operation direction information
				uint8_t((insn & 0x0100U) >> 8U),
				uint8_t((insn & eaModeMask) >> eaModeShift),
			};
		case 0x0140U:
			return
			{
				instruction_t::bchg,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
			};
		case 0x0180U:
			return
			{
				instruction_t::bclr,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
			};
	}

	// Decode instructions that use the effective address form without an Rx register
	switch (insn & insnMaskEANoReg)
	{
		case 0x0600U:
		case 0x0640U:
		case 0x0680U:
			return
			{
				instruction_t::addi,
				0U,
				uint8_t(insn & regMask),
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
			};
		case 0x0200U:
		case 0x0240U:
		case 0x0280U:
			return
			{
				instruction_t::andi,
				0U,
				uint8_t(insn & regMask),
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
			};
		case 0xe0c0U:
			return
			{
				instruction_t::asl,
				0U,
				uint8_t(insn & regMask),
				{},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
			};
		case 0xe1c0U:
			return
			{
				instruction_t::asr,
				0U,
				uint8_t(insn & regMask),
				{},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
			};
		case 0x0840U:
			return
			{
				instruction_t::bchg,
				0U,
				uint8_t(insn & regMask),
				{operationFlags_t::registerNotImmediate},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
				2U, // 16-bit bit number follows (8 bits used)
			};
		case 0x0880U:
			return
			{
				instruction_t::bclr,
				0U,
				uint8_t(insn & regMask),
				{operationFlags_t::registerNotImmediate},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
				2U, // 16-bit bit number follows (8 bits used)
			};
		case 0xeac0U:
			return
			{
				instruction_t::bfchg,
				0U,
				uint8_t(insn & regMask),
				{operationFlags_t::registerNotImmediate},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
				2U, // 16-bit {offset:width} follows
			};
		case 0xecc0U:
			return
			{
				instruction_t::bfclr,
				0U,
				uint8_t(insn & regMask),
				{operationFlags_t::registerNotImmediate},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
				2U, // 16-bit {offset:width} follows
			};
		case 0xebc0U:
			return
			{
				instruction_t::bfexts,
				0U,
				uint8_t(insn & regMask),
				{},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
				2U, // 16-bit {offset:width}, Dn follows
			};
		case 0xe9c0U:
			return
			{
				instruction_t::bfextu,
				0U,
				uint8_t(insn & regMask),
				{},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
				2U, // 16-bit {offset:width}, Dn follows
			};
		case 0xedc0U:
			return
			{
				instruction_t::bfffo,
				0U,
				uint8_t(insn & regMask),
				{},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
				2U, // 16-bit {offset:width}, Dn follows
			};
		case 0xefc0U:
			return
			{
				instruction_t::bfins,
				0U,
				uint8_t(insn & regMask),
				{},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
				2U, // 16-bit Dn, {offset:width} follows
			};
		case 0xeec0U:
			return
			{
				instruction_t::bfset,
				0U,
				uint8_t(insn & regMask),
				{},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
				2U, // 16-bit {offset:width} follows
			};
		case 0xe8c0U:
			return
			{
				instruction_t::bftst,
				0U,
				uint8_t(insn & regMask),
				{},
				0U, 0U,
				uint8_t((insn & eaModeMask) >> eaModeShift),
				2U, // 16-bit {offset:width} follows
			};
	}

	// Decode instructions that specify only a vector field
	switch (insn & insnMaskVector)
	{
		case 0x4848U:
			return
			{
				instruction_t::bkpt,
				uint8_t(insn & vectorMask),
			};
	}

	// Decode instructions that specify an 8-bit displacement
	switch (insn & insnMaskDisplacement)
	{
		case 0x6000U:
			return
			{
				instruction_t::bra,
				uint8_t(insn & displacementMask),
				0U,
				{},
				0U, 0U, 0U,
				[](const uint8_t displacement) -> uint8_t
				{
					// There are two special displacement values. A displacement of 0 means a 16-bit one follows
					if (displacement == 0x00U)
						return 2U;
					// A displacement value of 255 means a 32-bit one follows
					if (displacement == 0xffU)
						return 4U;
					// Otherwise nothing follows.
					return 0U;
				}(insn & displacementMask),
			};
	}
	return {instruction_t::illegal};
}
