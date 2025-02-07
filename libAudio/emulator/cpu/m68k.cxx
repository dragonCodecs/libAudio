// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include "m68k.hxx"

constexpr static uint16_t insnMaskSimpleRegs{0xf1f8U};
constexpr static uint16_t insnMaskEA{0xf1c0U};
constexpr static uint16_t insnMaskEANoReg{0xffc0U};
constexpr static uint16_t insnMaskVectorReg{0xfff8U};
constexpr static uint16_t insnMaskDisplacement{0xff00U};
constexpr static uint16_t insnMaskSizeOnly{0xf000U};
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

	// Extract out the effecetive addressing mode
	const auto eaMode{uint8_t((insn & eaModeMask) >> eaModeShift)};
	const auto eaReg{uint8_t(insn & regMask)};

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
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				// Extract the operation direction information
				uint8_t((insn & 0x0100U) >> 8U),
				eaMode,
			};
		case 0xd0c0U:
		case 0xd1c0U:
			return
			{
				instruction_t::adda,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				// Decode whether 16- or 32-bit operation
				uint8_t((insn & 0x01c0U) == 0x00c0U ? 2U : 4U),
				0U,
				eaMode,
			};
		case 0x5000U:
		case 0x5040U:
		case 0x5080U:
			// ADDQ is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::addq,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{operationFlags_t::immediateNotRegister},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
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
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				// Extract the operation direction information
				uint8_t((insn & 0x0100U) >> 8U),
				eaMode,
			};
		case 0x0140U:
			// BCHG is not allowed with address registers
			if (eaMode == 1U)
				break;
			// BCHG uses only the u16 and u32 indirect modes for mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::bchg,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x0180U:
			// BCLR is not allowed with address registers
			if (eaMode == 1U)
				break;
			// BCLR uses only the u16 and u32 indirect modes for mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::bclr,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x01c0U:
			// BSET is not allowed with address registers
			if (eaMode == 1U)
				break;
			// BSET uses only the u16 and u32 indirect modes for mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::bset,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x0100U:
			// BTST is not allowed with address registers
			if (eaMode == 1U)
				break;
			// BTST doesn't use a couple of the mode 7 modes
			if (eaMode == 7U && (eaReg == 5U || eaReg == 6U || eaReg == 7U))
				break;
			return
			{
				instruction_t::btst,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x4100U:
		case 0x4180U:
			// CHK is not allowed with address registers
			if (eaMode == 1U)
				break;
			// CHK is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
			return
			{
				instruction_t::chk,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				uint8_t((insn & 0x080U) ? 2U : 4U),
				0U,
				eaMode,
			};
		case 0xb000U:
		case 0xb040U:
		case 0xb080U:
			return
			{
				instruction_t::cmp,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0xb0c0U:
		case 0xb1c0U:
			return
			{
				instruction_t::cmpa,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				// Decode whether 16- or 32-bit operation
				uint8_t((insn & 0x01c0U) == 0x00c0U ? 2U : 4U),
				0U,
				eaMode,
			};
		case 0xf080U:
		case 0xf0c0U:
			return
			{
				instruction_t::cpbcc,
				// Coprocessor ID
				uint8_t((insn >> regXShift) & regMask),
				// Coprocessor Condition
				uint8_t(insn & 0x003fU),
				{},
				0U, 0U, 0U,
				// Decode whether this is a 16- or 32-bit displacement that follows
				uint8_t((insn & 0x0040U) ? 4U : 2U),
			};
		case 0xf000U:
			return
			{
				instruction_t::cpgen,
				// Coprocessor ID
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit coprocessor command follows
			};
		case 0xf040U:
			return
			{
				instruction_t::cpscc,
				// Coprocessor ID
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit coprocessor condition follows
			};
		case 0x81c0U:
			return
			{
				instruction_t::divs,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x80c0U:
			return
			{
				instruction_t::divu,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xb100U:
		case 0xb140U:
		case 0xb180U:
			return
			{
				instruction_t::eor,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x41c0U:
			// LEA is not allowed with direct register usage, or with register modification
			if (eaMode == 0U || eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// LEA is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::lea,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x2040U:
		case 0x3040U:
			// MOVEA is allowed all valid modes
			if (eaMode == 7U && eaReg > 4U)
				break;
			return
			{
				instruction_t::movea,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				// Extract whether this is a u16 or u32 operation
				uint8_t((insn & 0x1000U) ? 2U : 4U),
				0U,
				eaMode,
			};
		case 0xc1c0U:
			return
			{
				instruction_t::muls,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xc0c0U:
			return
			{
				instruction_t::mulu,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x8000U:
		case 0x8040U:
		case 0x8080U:
		case 0x8100U:
		case 0x8140U:
		case 0x8180U:
			return
			{
				instruction_t::_or,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				// Extract the operation direction information
				uint8_t((insn & 0x0100U) >> 8U),
				eaMode,
			};
		case 0x50c0U:
		case 0x51c0U:
			// Scc is not allowed with address registers
			if (eaMode == 1U)
				break;
			// Scc is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::scc,
				0U,
				eaReg,
				{},
				0U,
				uint8_t((insn & conditionMask) >> conditionShift),
				eaMode,
			};
		case 0x9000U:
		case 0x9040U:
		case 0x9080U:
		case 0x9100U:
		case 0x9140U:
		case 0x9180U:
			return
			{
				instruction_t::sub,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				// Extract the operation direction information
				uint8_t((insn & 0x0100U) >> 8U),
				eaMode,
			};
		case 0x90c0U:
		case 0x91c0U:
			return
			{
				instruction_t::suba,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{},
				// Decode whether 16- or 32-bit operation
				uint8_t((insn & 0x01c0U) == 0x00c0U ? 2U : 4U),
				0U,
				eaMode,
			};
		case 0x5100U:
		case 0x5140U:
		case 0x5180U:
			// SUBQ is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::subq,
				uint8_t((insn >> regXShift) & regMask),
				eaReg,
				{operationFlags_t::immediateNotRegister},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
	}

	// Decode instructions that use the effective address form without an Rx register
	switch (insn & insnMaskEANoReg)
	{
		case 0x0600U:
		case 0x0640U:
		case 0x0680U:
			// ADDI is not allowed with address registers
			if (eaMode == 1U)
				break;
			// ADDI is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::addi,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x0200U:
		case 0x0240U:
		case 0x0280U:
			// ANDI is not allowed with address registers
			if (eaMode == 1U)
				break;
			// ANDI is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::andi,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0xe0c0U:
			return
			{
				instruction_t::asl,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe1c0U:
			return
			{
				instruction_t::asr,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x0840U:
			// BCHG is not allowed with address registers
			if (eaMode == 1U)
				break;
			// BCHG is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::bchg,
				0U,
				eaReg,
				{operationFlags_t::immediateNotRegister},
				0U, 0U,
				eaMode,
				2U, // 16-bit bit number follows (8 bits used)
			};
		case 0x0880U:
			// BCLR is not allowed with address registers
			if (eaMode == 1U)
				break;
			// BCLR is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::bclr,
				0U,
				eaReg,
				{operationFlags_t::immediateNotRegister},
				0U, 0U,
				eaMode,
				2U, // 16-bit bit number follows (8 bits used)
			};
		case 0xeac0U:
			return
			{
				instruction_t::bfchg,
				0U,
				eaReg,
				{operationFlags_t::immediateNotRegister},
				0U, 0U,
				eaMode,
				2U, // 16-bit {offset:width} follows
			};
		case 0xecc0U:
			return
			{
				instruction_t::bfclr,
				0U,
				eaReg,
				{operationFlags_t::immediateNotRegister},
				0U, 0U,
				eaMode,
				2U, // 16-bit {offset:width} follows
			};
		case 0xebc0U:
			return
			{
				instruction_t::bfexts,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit {offset:width}, Dn follows
			};
		case 0xe9c0U:
			return
			{
				instruction_t::bfextu,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit {offset:width}, Dn follows
			};
		case 0xedc0U:
			return
			{
				instruction_t::bfffo,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit {offset:width}, Dn follows
			};
		case 0xefc0U:
			return
			{
				instruction_t::bfins,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit Dn, {offset:width} follows
			};
		case 0xeec0U:
			return
			{
				instruction_t::bfset,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit {offset:width} follows
			};
		case 0xe8c0U:
			return
			{
				instruction_t::bftst,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit {offset:width} follows
			};
		case 0x08c0U:
			// BSET is not allowed with address registers
			if (eaMode == 1U)
				break;
			// BSET is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::bset,
				0U,
				eaReg,
				{operationFlags_t::immediateNotRegister},
				0U, 0U,
				eaMode,
				2U, // 16-bit bit number follows (8 bits used)
			};
		case 0x0800U:
			// BTST is not allowed with address registers
			if (eaMode == 1U)
				break;
			// BTST is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::btst,
				0U,
				eaReg,
				{operationFlags_t::immediateNotRegister},
				0U, 0U,
				eaMode,
				2U, // 16-bit bit number follows (8 bits used)
			};
		case 0x06c0:
			// CALLM is not allowed with direct register usage, or with register modification
			if (eaMode == 0U || eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// CALLM is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::callm,
				0U,
				eaReg,
				{operationFlags_t::immediateNotRegister},
				0U, 0U,
				eaMode,
				2U, // 16-bit argument count follows (8 bits used)
			};
		case 0x0ac0U:
		case 0x0cc0U:
		case 0x0ec0U:
			// CAS is not allowed  with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// CAS is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::cas,
				0U,
				eaReg,
				{operationFlags_t::immediateNotRegister},
				uint8_t((insn & 0x0600U) >> 9U),
				0U,
				eaMode,
				2U, // 16-bit Dc, Du follows
			};
		case 0x00c0U:
		case 0x02c0U:
		case 0x04c0U:
			// Only non-altering indirection modes are allowed for CHK2/CMP2
			if (eaMode == 0U || eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// CHK2/CMP2 is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::chk2_cmp2,
				0U,
				eaReg,
				{operationFlags_t::immediateNotRegister},
				uint8_t((insn & 0x0600U) >> 9U),
				0U,
				eaMode,
				2U, // 16-bit Rn follows, bit 11 determines if chk2 (1) or cmp2 (0)
			};
		case 0x4200U:
		case 0x4240U:
		case 0x4280U:
			// CLR is not allowed with address registers
			if (eaMode == 1U)
				break;
			// CLR is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::clr,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x0c00U:
		case 0x0c40U:
		case 0x0c80U:
			// CMPI is not allowed with address registers
			if (eaMode == 1U)
				break;
			// CMPI is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::cmpi,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x4c40U:
			// DIVSL/DIVUL is not allowed with address registers
			if (eaMode == 1U)
				break;
			// DIVSL/DIVUL is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
			return
			{
				instruction_t::divsl_divul,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit Dr:Dq follows, bit 11 determines if DIVSL (1) or DIVUL (0)
			};
		case 0x0a00U:
		case 0x0a40U:
		case 0x0a80U:
			// ORI is not allowed with address registers
			if (eaMode == 1U)
				break;
			// ORI is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::eori,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x4ec0U:
			// Only non-altering indirection modes are allowed for JMP
			if (eaMode == 0U || eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// JMP is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::jmp,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x4e80U:
			// Only non-altering indirection modes are allowed for JSR
			if (eaMode == 0U || eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// JSR is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::jsr,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe2c0U:
			return
			{
				instruction_t::lsr,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe3c0U:
			return
			{
				instruction_t::lsl,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x42c0U:
			// MOVE from CCR is not allowed with address registers
			if (eaMode == 1U)
				break;
			// MOVE from CCR is not allowed with `#<data>` mode or PC-rel data register usage,
			// only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::move,
				8U, // 8 is a special register number (not otherwise valid) indicating CCR.
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x44c0U:
			// MOVE to CCR is not allowed with address registers
			if (eaMode == 1U)
				break;
			// MOVE to CCR is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
			return
			{
				instruction_t::move,
				eaReg,
				8U, // 8 is a special register number (not otherwise valid) indicating CCR.
				{},
				0U, 0U,
				eaMode,
			};
		case 0x40c0U:
			// MOVE from SR is not allowed with address registers
			if (eaMode == 1U)
				break;
			// MOVE from SR is not allowed with `#<data>` mode or PC-rel data register usage,
			// only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::move,
				9U, // 9 is a special register number (not otherwise valid) indicating SR.
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x46c0U:
			// MOVE to SR is not allowed with address registers
			if (eaMode == 1U)
				break;
			// MOVE to SR is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
			return
			{
				instruction_t::move,
				eaReg,
				9U, // 9 is a special register number (not otherwise valid) indicating SR.
				{},
				0U, 0U,
				eaMode,
			};
		case 0x4880U:
		case 0x48c0U:
		case 0x4c80U:
		case 0x4cc0U:
			// MOVEM is not allowed with direct register usage, or postinc mode
			if (eaMode == 0U || eaMode == 1U || eaMode == 3U)
				break;
			// MOVEM is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::movem,
				0U,
				eaReg,
				{},
				// Extract whether the registers should be moved as u16's or u32's
				uint8_t((insn & 0x0040U) ? 4U : 2U),
				// Extract out the transfer direction
				uint8_t((insn & 0x0400U) >> 10U),
				eaMode,
				2U, // 16-bit register list mask follows
			};
		case 0x0e00U:
		case 0x0e40U:
		case 0x0e80U:
			// MOVES is not allowed with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// MOVES is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::moves,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x4c00U:
			// MULS/MULU is not allowed with address registers
			if (eaMode == 1U)
				break;
			// MULS/MULU is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
			return
			{
				instruction_t::muls_mulu,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit Dh - Dl follows, bit 11 determines of MULS (1) or MULU (0)
			};
		case 0x4800U:
			// NBCD is not allowed with address registers
			if (eaMode == 1U)
				break;
			// NBCD is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::nbcd,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x4400U:
		case 0x4440U:
		case 0x4480U:
			// NEG is not allowed with address registers
			if (eaMode == 1U)
				break;
			// NEG is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::neg,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x4000U:
		case 0x4040U:
		case 0x4080U:
			// NEGX is not allowed with address registers
			if (eaMode == 1U)
				break;
			// NEGX is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::negx,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x4600U:
		case 0x4640U:
		case 0x4680U:
			// NOT is not allowed with address registers
			if (eaMode == 1U)
				break;
			// NOT is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::_not,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x0000U:
		case 0x0040U:
		case 0x0080U:
			// ORI is not allowed with address registers
			if (eaMode == 1U)
				break;
			// ORI is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::ori,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x4840U:
			// PEA is not allowed with direct register usage, or with register modification
			if (eaMode == 0U || eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// PEA is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::pea,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe6c0U:
			return
			{
				instruction_t::ror,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe7c0U:
			return
			{
				instruction_t::rol,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe4cU:
			return
			{
				instruction_t::roxr,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe5cU:
			return
			{
				instruction_t::roxl,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};

		case 0x0400U:
		case 0x0440U:
		case 0x0480U:
			// SUBI is not allowed with address registers
			if (eaMode == 1U)
				break;
			// SUBI is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::subi,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
		case 0x4ac0U:
			// TAS is not allowed with address registers
			if (eaMode == 1U)
				break;
			// TAS is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::tas,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0x4a00U:
		case 0x4a40U:
		case 0x4a80U:
			// TST is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
			return
			{
				instruction_t::tst,
				0U,
				eaReg,
				{},
				uint8_t((insn & sizeMask) >> sizeShift),
				0U,
				eaMode,
			};
	}

	// Decode instructions that specify only a vector or register field
	switch (insn & insnMaskVectorReg)
	{
		case 0x4848U:
			return
			{
				instruction_t::bkpt,
				uint8_t(insn & vectorMask),
			};
		case 0x4880U:
		case 0x48c0U:
			return
			{
				instruction_t::ext,
				uint8_t(insn & regMask),
				0U,
				{},
				// Decode whether this is a i8 or i16 we're sign extending (to i16 and i32 respectively)
				uint8_t((insn & 0x0040U) ? 2U : 1U),
			};
		case 0x49c0U:
			return
			{
				instruction_t::extb,
				uint8_t(insn & regMask),
			};
		case 0x4e50U:
			return
			{
				instruction_t::link,
				uint8_t(insn & regMask),
				0U,
				{},
				0U, 0U, 0U,
				2U, // 16-bit displayment follows
			};
		case 0x4808U:
			return
			{
				instruction_t::link,
				uint8_t(insn & regMask),
				0U,
				{},
				0U, 0U, 0U,
				4U, // 32-bit displayment follows
			};
		case 0xf620U:
			return
			{
				instruction_t::move16,
				uint8_t(insn & regMask),
				0U,
				{operationFlags_t::postincrement},
				0U, 0U, 0U,
				2U, // 16-bit instruction continuation follows, containing Ay
			};
		case 0xf600U:
		case 0xf608U:
		case 0xf610U:
		case 0xf618U:
			return
			{
				instruction_t::move16,
				0U,
				uint8_t(insn & regMask),
				{},
				0U,
				// Extract out the operation mode
				uint8_t((insn & 0x0018U) >> 3U),
				0U,
				4U, // 32-bit absolute address follows
			};
		case 0x4e60U:
			// MOVE to USP
			return
			{
				instruction_t::move,
				uint8_t(insn & regMask),
				10U, // 10 is a special register number (not otherwise valid) indicating USP
			};
		case 0x4e68U:
			// MOVE from USP
			return
			{
				instruction_t::move,
				10U, // 10 is a special register number (not otherwise valid) indicating USP
				uint8_t(insn & regMask),
			};
		case 0x06c0U:
		case 0x06c8U:
			return
			{
				instruction_t::rtm,
				0U,
				uint8_t(insn & regMask),
				{},
				0U,
				// Extract the data-or-address bit for what flavour the register in ry is
				uint8_t((insn & 0x0008U) >> 3U),
			};
		case 0x4840U:
			return
			{
				instruction_t::swap,
				uint8_t(insn & regMask),
			};
		case 0x4e40U:
		case 0x4e48U:
			return
			{
				instruction_t::trap,
				uint8_t(insn & 0x000fU),
			};
		case 0x4e58U:
			return
			{
				instruction_t::unlk,
				uint8_t(insn & regMask),
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
		case 0x6100U:
			return
			{
				instruction_t::bsr,
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

	// Decode move - special case
	switch (insn & insnMaskSizeOnly)
	{
		case 0x1000U:
		case 0x3000U:
		case 0x2000U:
		{
			// Extract out the effective addressing mode for the source
			const auto srcEAMode{uint8_t((insn & eaModeMask) >> eaModeShift)};
			const auto srcEAReg{uint8_t(insn & regMask)};
			// MOVE is allowed all valid modes for the source
			if (srcEAMode == 7U && srcEAReg > 4U)
				break;
			// Extract out the effective addressing mode for the destination
			const auto dstEAMode{uint8_t((insn & 0x01c0U) >> 6U)};
			const auto dstEAReg{uint8_t((insn >> regXShift) & regMask)};
			// MOVE is not allowed with a destination address register
			if (dstEAMode == 1U)
				break;
			// MOVE is not allowed with `#<data>` mode or PC-rel destination data register usage,
			// only u16 and u32 indirect mode 7
			if (dstEAMode == 7U && !(dstEAReg == 0U || dstEAReg == 1U))
				break;
			return
			{
				instruction_t::move,
				dstEAReg,
				srcEAReg,
				{},
				// Extract out whether we're to do a u8, u16 or u32 move
				[](const uint8_t size) -> uint8_t
				{
					if (size == 1U)
						return 1U; // u8 move
					if (size == 3U)
						return 2U; // u16 move
					return 4U; // u32 move
				}((insn & 0x3000U) >> 12U),
				0U,
				// Extract out both destination and source mode bits
				uint8_t((insn & 0x01f8U) >> 3U),
			};
		}
	}

	// Decode moveq - special case
	if ((insn & 0xf100U) == 0x7000U)
	{
		return
		{
			instruction_t::moveq,
			uint8_t((insn >> regXShift) & regMask),
			uint8_t(insn & 0x00ffU),
		};
	}

	// Decode instructions that use the `insn Ry, Rx` basic form
	switch (insn & insnMaskSimpleRegs)
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
						return {operationFlags_t::immediateNotRegister};
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
						return {operationFlags_t::immediateNotRegister};
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
		case 0xf048U:
			return
			{
				instruction_t::cpdbcc,
				// Coprocessor ID
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{},
				0U, 0U, 0U,
				4U, // 16-bit coprocessor condition and 16-bit displacement follows
			};
		case 0xf078U:
			return
			{
				instruction_t::cptrapcc,
				// Coprocessor ID
				uint8_t((insn >> regXShift) & regMask),
				0U,
				{},
				0U,
				uint8_t(insn & regMask),
				0U,
				uint8_t
				(
					[](const uint8_t opmode)
					{
						if (opmode == 2U)
							return 2U; // Instruction followed by 1 operand u16
						if (opmode == 3U)
							return 4U; // Instruction followed by 2 operand u16's
						if (opmode == 4U)
							return 0U; // Instruction followed by no operand u16's
						throw std::exception{}; // Illegal instruction - better handling TBD
					}(insn & regMask) + 2U
				), // 16-bit coprocessor condition follows
			};
		case 0x50c8U:
		case 0x51c8U:
			return
			{
				instruction_t::dbcc,
				0U,
				uint8_t(insn & regMask),
				{},
				0U,
				uint8_t((insn & conditionMask) >> conditionShift),
				0U,
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
						return {operationFlags_t::immediateNotRegister};
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
						return {operationFlags_t::immediateNotRegister};
					return {};
				}(),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0x0108U:
		case 0x0148U:
		case 0x0188U:
		case 0x01c8U:
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
						return {operationFlags_t::immediateNotRegister};
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
						return {operationFlags_t::immediateNotRegister};
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
						return {operationFlags_t::immediateNotRegister};
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
						return {operationFlags_t::immediateNotRegister};
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
		case 0x50f8U:
		case 0x51f8U:
			return
			{
				instruction_t::trapcc,
				0U, 0U,
				{},
				0U,
				// Extract the operation condition code
				uint8_t((insn & 0x0f00U) >> 8U),
				0U,
				[](const uint8_t opMode) -> uint8_t
				{
					if (opMode == 2U)
						return 2U; // u16 operand follows
					if (opMode == 3U)
						return 4U; // u32 operand follows
					return 0U; // No operand follows
				}(insn & regMask),
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
		case 0x027cU:
			return
			{
				instruction_t::andi,
				0U, 9U, // 9 is a special register number (not otherwise valid) indicating SR.
				{},
				1U, 0U, 0U,
				2U, // 16-bit immediate follows
			};
		case 0x0cfcU:
		case 0x0efcU:
			return
			{
				instruction_t::cas2,
				0U, 0U, {},
				// Extract whether this is a u16 or u32 operation
				uint8_t((insn & 0x0200U) ? 4U : 2U),
				0U, 0U,
				4U, // 32-bit Dc1:Dc2, Du1:Du2, (Rn1):(Rn2) follows
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
		case 0x4afcU:
			return {instruction_t::illegal};
		case 0x4e7aU:
		case 0x4e7bU:
			return
			{
				instruction_t::movec,
				0U, 0U, {}, 0U,
				// Extract out the transfer direction
				uint8_t(insn & 0x0001U),
			};
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
		case 0x007cU:
			return
			{
				instruction_t::ori,
				0U, 9U, // 8 is a special register number (not otherwise valid) indicating SR.
				{},
				1U, 0U, 0U,
				2U, // 16-bit immediate follows
			};
		case 0x4e70U:
			return {instruction_t::reset};
		case 0x4e74U:
			return
			{
				instruction_t::rtd,
				0U, 0U, {}, 0U, 0U, 0U,
				2U, // 16-bit displacement follows
			};
		case 0x4e73U:
			return {instruction_t::rte};
		case 0x4e77U:
			return {instruction_t::rtr};
		case 0x4e75U:
			return {instruction_t::rts};
		case 0x4e72U:
			return
			{
				instruction_t::stop,
				0U, 0U, {}, 0U, 0U, 0U,
				2U, // 16-bit SR value follows
			};
		case 0x4e76U:
			return {instruction_t::trapv};
	}
	return {instruction_t::illegal};
}
