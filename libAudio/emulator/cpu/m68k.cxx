// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <string_view>
#include <substrate/index_sequence>
#include "m68k.hxx"
#include "console.hxx"

using namespace std::literals::string_view_literals;
using namespace libAudio::console;

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

motorola68000_t::motorola68000_t(memoryMap_t<uint32_t> &peripherals, const uint32_t clockFreq) noexcept :
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
			// If the effective address target is used only as a source operand
			if ((insn & 0x0100U) == 0U)
			{
				// ADD is allowed all valid modes
				if (eaMode == 7U && eaReg > 4U)
					break;
			}
			// Otherwise if it's a target, the requirements are much stricter
			else
			{
				// ADD is not allowed with direct register usage
				if (eaMode == 0U || eaMode == 1U)
					break;
				// ADD is not allowed with `#<data>` mode or PC-rel data register usage,
				// only u16 and u32 indirect mode 7
				if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
					break;
			}
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
			// ADDA is allowed all valid modes
			if (eaMode == 7U && eaReg > 4U)
				break;
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
			// If the effective address target is used only as a source operand
			if ((insn & 0x0100U) == 0U)
			{
				// AND is not allowed with address registers
				if (eaMode == 1U)
					break;
				// AND is allowed all valid mode 7 modes
				if (eaMode == 7U && eaReg > 4U)
					break;
			}
			// Otherwise if it's a target, the requirements are much stricter
			else
			{
				// AND is not allowed with direct register usage
				if (eaMode == 0U || eaMode == 1U)
					break;
				// SUB is not allowed with `#<data>` mode or PC-rel data register usage,
				// only u16 and u32 indirect mode 7
				if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
					break;
			}
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
			// CMP is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
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
			// CMPA is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
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
		case 0x81c0U:
			// DIVS is not allowed with address registers
			if (eaMode == 1U)
				break;
			// DIVS is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
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
			// DIVU is not allowed with address registers
			if (eaMode == 1U)
				break;
			// DIVU is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
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
			// EOR is not allowed with address registers
			if (eaMode == 1U)
				break;
			// EOR is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
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
			// MULS is not allowed with address registers
			if (eaMode == 1U)
				break;
			// MULS is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
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
			// MULU is not allowed with address registers
			if (eaMode == 1U)
				break;
			// MULU is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
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
			// If the effective address target is used only as a source operand
			if ((insn & 0x0100U) == 0U)
			{
				// OR is not allowed with address registers
				if (eaMode == 1U)
					break;
				// OR is allowed all valid mode 7 modes
				if (eaMode == 7U && eaReg > 4U)
					break;
			}
			// Otherwise if it's a target, the requirements are much stricter
			else
			{
				// OR is not allowed with direct register usage
				if (eaMode == 0U || eaMode == 1U)
					break;
				// OR is not allowed with `#<data>` mode or PC-rel data register usage,
				// only u16 and u32 indirect mode 7
				if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
					break;
			}
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
			// If the effective address target is used only as a source operand
			if ((insn & 0x0100U) == 0U)
			{
				// SUB is allowed all valid mode 7 modes
				if (eaMode == 7U && eaReg > 4U)
					break;
			}
			// Otherwise if it's a target, the requirements are much stricter
			else
			{
				// SUB is not allowed with direct register usage
				if (eaMode == 0U || eaMode == 1U)
					break;
				// SUB is not allowed with `#<data>` mode or PC-rel data register usage,
				// only u16 and u32 indirect mode 7
				if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
					break;
			}
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
			// SUBA is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
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
			// ASR is not allowed with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// ASR is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::asr,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe1c0U:
			// ASR is not allowed with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// ASR is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::asl,
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
			// BFCHG is not allowed with address registers, or with register modification
			if (eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// BFCHG is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
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
			// BFCLR is not allowed with address registers, or with register modification
			if (eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// BFCLR is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
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
			// BFEXTS is not allowed with address registers, or with register modification
			if (eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// BFEXTS is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
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
			// BFEXTU is not allowed with address registers, or with register modification
			if (eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// BFEXTU is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
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
			// BFFFO is not allowed with address registers, or with register modification
			if (eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// BFFFO is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
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
			// BFINS is not allowed with address registers, or with register modification
			if (eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// BFINS is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
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
			// BFSET is not allowed with address registers, or with register modification
			if (eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// BFSET is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
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
			// BFTST is not allowed with address registers, or with register modification
			if (eaMode == 1U || eaMode == 3U || eaMode == 4U)
				break;
			// BFTST is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
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
		case 0xf280U:
		case 0xf2c0U:
			// Only condition codes up to (and including) 0x1f are valid
			if ((eaMode & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::fbcc,
				0U, 0U,
				{},
				0U,
				uint8_t((eaMode << 3U) | eaReg), // Rebuild the condition code
				0U,
				uint8_t((insn & 0x0040U) == 0U ? 2U : 4U), // 16- or 32-bit of displacement follows
			};
		case 0xf200U:
			// F* instruction is not allowed with address registers
			if (eaMode == 1U)
				break;
			// F* instruction is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
			return
			{
				instruction_t::fpu,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit instruction continuation follows
			};
		case 0xf340U:
			// FRESTORE is not allowed with direct register usage, or with pre-dec register modification
			if (eaMode == 0U || eaMode == 1U || eaMode == 4U)
				break;
			// FRESTORE is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::frestore,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xf300U:
			// FSAVE is not allowed with direct register usage, or with post-inc register modification
			if (eaMode == 0U || eaMode == 1U || eaMode == 3U)
				break;
			// FSAVE is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::fsave,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xf240U:
			// FScc instruction is not allowed with address registers
			if (eaMode == 1U)
				break;
			// FScc is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::fscc,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bits follow (5 bits used) for conditional predicate
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
			// LSR is not allowed with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// LSR is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
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
			// LSL is not allowed with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// LSL is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
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
			// MOVEM is not allowed with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// If this is a reg -> mem MOVEM
			if ((insn & 0x0400U) == 0U)
			{
				// MOVEM is not allowed with postincrement mode
				if (eaMode == 3U)
					break;
				// MOVEM is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
				if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
					break;
			}
			// Otherwise if this is a mem -> reg MOVEM
			else
			{
				// MOVEM is not allowed with predecrement mode
				if (eaMode == 4U)
					break;
				// MOVEM is not allowed with `#<data>` mode 7
				if (eaMode == 7U && (eaReg & 0x4U) != 0U)
					break;
			}
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
		case 0xf080U:
		case 0xf0c0U:
			// Only condition codes up to (and including) 0x0f are valid
			if (eaMode != 0U && eaMode != 1U)
				break;
			return
			{
				instruction_t::pbcc,
				0U, 0U,
				{},
				0U,
				uint8_t((eaMode << 3U) | eaReg), // Rebuild the condition code
				0U,
				uint8_t((insn & 0x0040U) == 0U ? 2U : 4U), // 16- or 32-bit of displacement follows
			};
		case 0xf140U:
			// PRESTORE is not allowed with direct register usage, or with pre-dec register modification
			if (eaMode == 0U || eaMode == 1U || eaMode == 4U)
				break;
			// PRESTORE is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::prestore,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xf000U:
		case 0xf040U:
			// P* instruction is allowed all valid mode 7 modes
			if (eaMode == 7U && eaReg > 4U)
				break;
			// Filter out PDBcc
			if ((insn & 0x00c0U) == 0x0040U && eaMode == 1U)
				break;
			// filter out PTRAPcc
			if ((insn & 0x00c0U) == 0x0040U && eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::privileged,
				0U,
				eaReg,
				{},
				0U,
				// Extract which block of privileged instructions this one belongs to
				uint8_t((insn & 0x00c0U) >> 6U),
				eaMode,
				2U, // 16-bit instruction continuation follows
			};
		case 0xf100U:
			// PSAVE is not allowed with direct register usage, or with post-inc register modification
			if (eaMode == 0U || eaMode == 1U || eaMode == 3U)
				break;
			// PSAVE is not allowed with `#<data>` mode or PC-rel data register usage,
			// only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::psave,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe6c0U:
			// ROR is not allowed with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// ROR is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
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
			// ROL is not allowed with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// ROL is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::rol,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe4c0U:
			// ROXR is not allowed with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// ROXR is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
			return
			{
				instruction_t::roxr,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
			};
		case 0xe5c0U:
			// ROXL is not allowed with direct register usage
			if (eaMode == 0U || eaMode == 1U)
				break;
			// ROXL is not allowed with `#<data>` mode or PC-rel data register usage, only u16 and u32 indirect mode 7
			if (eaMode == 7U && !(eaReg == 0U || eaReg == 1U))
				break;
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
		case 0xf800U:
			// TBLU/TBLS is not allowed with address registers, or with post-inc register modification
			if (eaMode == 1U || eaMode == 3U)
				break;
			// TBLU/TBLS is not allowed with `#<data>` mode 7
			if (eaMode == 7U && (eaReg & 0x4U) != 0U)
				break;
			return
			{
				instruction_t::tbls_tblu,
				0U,
				eaReg,
				{},
				0U, 0U,
				eaMode,
				2U, // 16-bit instruction continuation follows, containing Dx, Dyn, size.
				// Bit 8 determines DReg (0) vs TLB (1), bit 11 determines TLBS (1) vs TBLU (0)
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
		case 0xf408U:
		case 0xf448U:
		case 0xf488U:
		case 0xf4c8U:
			return
			{
				instruction_t::cinvl,
				uint8_t(insn & regMask),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xf410U:
		case 0xf450U:
		case 0xf490U:
		case 0xf4d0U:
			return
			{
				instruction_t::cinvp,
				uint8_t(insn & regMask),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xf418U:
		case 0xf458U:
		case 0xf498U:
		case 0xf4d8U:
			return
			{
				instruction_t::cinva,
				uint8_t(insn & regMask),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xf428U:
		case 0xf468U:
		case 0xf4a8U:
		case 0xf4e8U:
			return
			{
				instruction_t::cpushl,
				uint8_t(insn & regMask),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xf430U:
		case 0xf470U:
		case 0xf4b0U:
		case 0xf4f0U:
			return
			{
				instruction_t::cpushp,
				uint8_t(insn & regMask),
				uint8_t((insn & sizeMask) >> sizeShift),
			};
		case 0xf438U:
		case 0xf478U:
		case 0xf4b8U:
		case 0xf4f8U:
			return
			{
				instruction_t::cpusha,
				uint8_t(insn & regMask),
				uint8_t((insn & sizeMask) >> sizeShift),
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
		case 0xf248U:
			return
			{
				instruction_t::fdbcc,
				0U,
				uint8_t(insn & regMask),
				{},
				0U, 0U, 0U,
				4U, // 16-bit instruction continuation + 16-bit displacement follows
			};
		case 0xf278U:
		{
			const auto opmode{uint8_t(insn & regMask)};
			if (opmode != 2U && opmode != 3U && opmode != 4U)
				break;
			return
			{
				instruction_t::ftrapcc,
				0U, 0U,
				{},
				0U,
				opmode,
				0U,
				uint8_t
				(
					[&]()
					{
						if (opmode == 2U)
							return 2U; // Instruction followed by 1 operand u16
						if (opmode == 3U)
							return 4U; // Instruction followed by 2 operand u16's
						return 0U; // Instruction followed by no operand u16's
					}() + 2U // 16-bit conditionial predicate (5 bits used) follows
				),
			};
		}
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
		case 0xf048U:
			return
			{
				instruction_t::pdbcc,
				0U,
				uint8_t(insn & regMask),
				{},
				0U, 0U, 0U,
				4U, // 16-bit instruction continuation + 16-bit displacement follows
			};
		case 0xf500U:
			return
			{
				instruction_t::pflushn,
				uint8_t(insn & regMask),
			};
		case 0xf508U:
			return
			{
				instruction_t::pflush,
				uint8_t(insn & regMask),
			};
		case 0xf510U:
			return
			{
				instruction_t::pflushan,
				uint8_t(insn & regMask),
			};
		case 0xf518U:
			return
			{
				instruction_t::pflusha,
				uint8_t(insn & regMask),
			};
		case 0xf548U:
		case 0xf568U:
			return
			{
				instruction_t::ptest,
				uint8_t(insn & regMask),
				0U,
				{},
				0U,
				uint8_t((insn & 0x0020U) >> 5U), // Extract read vs write
			};
		case 0xf078U:
		{
			const auto opmode{uint8_t(insn & regMask)};
			if (opmode != 2U && opmode != 3U && opmode != 4U)
				break;
			return
			{
				instruction_t::ptrapcc,
				0U, 0U,
				{},
				0U,
				opmode,
				0U,
				uint8_t
				(
					[&]()
					{
						if (opmode == 2U)
							return 2U; // Instruction followed by 1 operand u16
						if (opmode == 3U)
							return 4U; // Instruction followed by 2 operand u16's
						return 0U; // Instruction followed by no operand u16's
					}() + 2U
				), // 16-bit coprocessor condition follows
			};
		}
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
		case 0x6200U:
		case 0x6300U:
		case 0x6400U:
		case 0x6500U:
		case 0x6600U:
		case 0x6700U:
		case 0x6800U:
		case 0x6900U:
		case 0x6a00U:
		case 0x6b00U:
		case 0x6c00U:
		case 0x6d00U:
		case 0x6e00U:
		case 0x6f00U:
			return
			{
				instruction_t::bcc,
				uint8_t(insn & displacementMask),
				0U,
				{},
				0U,
				uint8_t((insn & conditionMask) >> conditionShift),
				0U,
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
		case 0xc148U:
		case 0xc188U:
			return
			{
				instruction_t::exg,
				uint8_t((insn >> regXShift) & regMask),
				uint8_t(insn & regMask),
				{},
				0U,
				// Extract out the operating mode
				uint8_t((insn & 0x00f8U) >> 3U),
			};
		case 0xe008U:
		case 0xe028U:
		case 0xe048U:
		case 0xe068U:
		case 0xe088U:
		case 0xe0a8U:
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
		case 0xe108U:
		case 0xe128U:
		case 0xe148U:
		case 0xe168U:
		case 0xe188U:
		case 0xe1a8U:
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
		{
			const auto opMode{uint8_t(insn & regMask)};
			// Only 3 of the opmode combinations are valid for this instruction
			if (opMode != 2U && opMode != 3U && opMode != 4U)
				break;
			return
			{
				instruction_t::trapcc,
				0U, 0U,
				{},
				0U,
				// Extract the operation condition code
				uint8_t((insn & 0x0f00U) >> 8U),
				0U,
				[&]() -> uint8_t
				{
					if (opMode == 2U)
						return 2U; // u16 operand follows
					if (opMode == 3U)
						return 4U; // u32 operand follows
					return 0U; // No operand follows
				}(),
			};
		}
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

void motorola68000_t::executeFrom(const uint32_t entryAddress, const uint32_t stackTop, const bool asUser) noexcept
{
	// Copy in the new program counter value, and set up the stack pointer and CPU state
	programCounter = entryAddress;
	if (asUser)
	{
		userStackPointer = stackTop;
		status.clear(m68kStatusBits_t::supervisor);
	}
	else
	{
		systemStackPointer = stackTop;
		status.set(m68kStatusBits_t::supervisor);
	}
}

uint32_t &motorola68000_t::dataRegister(const size_t reg) noexcept
	{ return d.at(reg); }

uint32_t &motorola68000_t::addrRegister(const size_t reg) noexcept
{
	if (reg == 7U)
		return activeStackPointer();
	return a.at(reg);
}

void motorola68000_t::writeDataRegister(const size_t reg, const uint32_t value) noexcept
	{ dataRegister(reg) = value; }
void motorola68000_t::writeAddrRegister(const size_t reg, const uint32_t value) noexcept
	{ addrRegister(reg) = value; }
uint32_t motorola68000_t::readProgramCounter() const noexcept { return programCounter; }

stepResult_t motorola68000_t::step() noexcept
{
	// Start by fetching a uint16_t for the instruction and trying to decode it
	auto instruction{decodeInstruction(_peripherals.readAddress<uint16_t>(programCounter))};
	programCounter += 2U;
	// Now we have an instruction to run, try to dispatch it
	switch (instruction.operation)
	{
		case instruction_t::add:
			return dispatchADD(instruction);
		case instruction_t::adda:
			return dispatchADDA(instruction);
		case instruction_t::addq:
			return dispatchADDQ(instruction);
		case instruction_t::andi:
			return dispatchANDI(instruction);
		case instruction_t::bcc:
			return dispatchBcc(instruction);
		case instruction_t::bclr:
			return dispatchBCLR(instruction);
		case instruction_t::bra:
			return dispatchBRA(instruction);
		case instruction_t::bsr:
			return dispatchBSR(instruction);
		case instruction_t::clr:
			return dispatchCLR(instruction);
		case instruction_t::cmp:
			return dispatchCMP(instruction);
		case instruction_t::cmpi:
			return dispatchCMPI(instruction);
		case instruction_t::dbcc:
			return dispatchDBcc(instruction);
		case instruction_t::lea:
			return dispatchLEA(instruction);
		case instruction_t::move:
			return dispatchMOVE(instruction);
		case instruction_t::movea:
			return dispatchMOVEA(instruction);
		case instruction_t::movem:
			return dispatchMOVEM(instruction);
		case instruction_t::moveq:
			return dispatchMOVEQ(instruction);
		case instruction_t::rts:
			return dispatchRTS();
		case instruction_t::scc:
			return dispatchScc(instruction);
		case instruction_t::sub:
			return dispatchSUB(instruction);
		case instruction_t::tst:
			return dispatchTST(instruction);
	}

	return {false, false, 34U};
}

void motorola68000_t::displayRegs() const noexcept
{
	// Start by displaying the d-regs
	for (const auto idx : substrate::indexSequence_t{8U})
	{
		if ((idx & 3U) == 0U)
			console.debug(nullptr);
		console.output("  d"sv, idx, ": "sv, asHex_t<8U, '0'>{d[idx]}, nullptr);
		if ((idx & 3U) == 3U)
			console.output();
		else
			console.output(' ', nullptr);
	}

	// Now the a-regs, including the active the stack pointer
	for (const auto idx : substrate::indexSequence_t{8U})
	{
		if ((idx & 3U) == 0U)
			console.debug(nullptr);
		if (idx != 7U)
			console.output("  a"sv, idx, ": "sv, asHex_t<8U, '0'>{a[idx]}, nullptr);
		else
			console.output
			(
				"  a7: "sv,
				asHex_t<8U, '0'>{status.includes(m68kStatusBits_t::supervisor) ? systemStackPointer : userStackPointer},
				nullptr
			);

		if ((idx & 3U) == 3U)
			console.output();
		else
			console.output(' ', nullptr);
	}

	// Now display the stack pointers, program counter, and finally the status register
	console.debug
	(
		" ssp: "sv, asHex_t<8U, '0'>{systemStackPointer}, "  usp: "sv, asHex_t<8U, '0'>{userStackPointer},
		"   pc: "sv, asHex_t<8U, '0'>{programCounter}, "   sr: "sv, asHex_t<4U, '0'>{status.toRaw()}
	);
}

int16_t motorola68000_t::readIndex(const uint16_t extension) const noexcept
{
	// Get the index value that will be used
	const auto index
	{
		[&](const bool needsExtending) -> int32_t
		{
			// Extract out the register value that will be used to construct this index
			const auto value
			{
				[&]()
				{
					// Determine data or address, then extract that register's value
					if (extension & 0x8000U)
						return a[(extension & 0x7000U) >> 12U];
					return d[(extension & 0x7000U) >> 12U];
				}()
			};
			// Fix up anything that we need to fix up
			if (needsExtending)
				return static_cast<int16_t>(value);
			return static_cast<int32_t>(value);
		}((extension & 0x0800U) == 0U)
	};
	// Extract what scaling it needs, and dispatch
	switch (extension & 0x0600U)
	{
		case 0x0000U: // * 1
			return index;
		case 0x0200U: // * 2
			return index * 2U;
		case 0x0400U: // * 4
			return index * 4U;
		case 0x0600U: // * 8
			return index * 8;
	}
	// We can't actually get here.. but just incase
	return INT16_MAX;
}

int32_t motorola68000_t::readExtraDisplacement(const uint8_t displacementSize) noexcept
{
	switch (displacementSize)
	{
		case 2U: // s16 displacement follows
		{
			const auto result{_peripherals.readAddress<int16_t>(programCounter)};
			programCounter += 2U;
			return result;
		}
		case 3U: // s32 displacement follows
		{
			const auto result{_peripherals.readAddress<int32_t>(programCounter)};
			programCounter += 4U;
			return result;
		}
		default:
			// No displacement follows (or we got the reserved size)
			return 0U;
	}
}

uint32_t motorola68000_t::readImmediateUnsigned(const size_t immediateSize) noexcept
{
	// Extract the immediate from just after the instruction
	const auto immediate
	{
		[&]() -> uint32_t
		{
			if (immediateSize == 1U)
				return static_cast<uint8_t>(_peripherals.readAddress<uint16_t>(programCounter));
			if (immediateSize == 2U)
				return _peripherals.readAddress<uint16_t>(programCounter);
			return _peripherals.readAddress<uint32_t>(programCounter);
		}()
	};
	// Adjust the program counter by the number of u16's just consumed
	programCounter += immediateSize == 4U ? 4U : 2U;
	// Return the zero-extended immediate
	return immediate;
}

int32_t motorola68000_t::readImmediateSigned(const size_t immediateSize) noexcept
{
	// Extract the immediate from just after the instruction
	const auto immediate
	{
		[&]() -> int32_t
		{
			if (immediateSize == 1U)
				return static_cast<int8_t>(_peripherals.readAddress<int16_t>(programCounter));
			if (immediateSize == 2U)
				return _peripherals.readAddress<int16_t>(programCounter);
			return _peripherals.readAddress<int32_t>(programCounter);
		}()
	};
	// Adjust the program counter by the number of u16's just consumed
	programCounter += immediateSize == 4U ? 4U : 2U;
	// Return the sign-extended immediate
	return immediate;
}

uint32_t motorola68000_t::computeIndirect(const uint32_t baseAddress) noexcept
{
	const auto extra{_peripherals.readAddress<uint16_t>(programCounter)};
	programCounter += 2U;
	// Determine which extension mode we're in
	if (extra & 0x0100U)
	{
		// Figure out what do for the base displacement
		const auto baseDisplacement
		{
			[&]() -> int32_t
			{
				// If it's suppressed, make it 0
				if (extra & 0x0080U)
					return 0;
				// Otherwise, determine how many bytes follow and read them
				return readExtraDisplacement((extra & 0x0030U) >> 4U);
			}()
		};
		// Now the index, which can also be suppressed
		const auto index{(extra & 0x0040U) ? 0 : readIndex(extra)};
		// Extract the indirection action selection
		const auto action{uint8_t(extra & 0x0007U)};
		const auto outerDisplacement
		{
			[&]() -> int32_t
			{
				// If it is 4, for any reason, that is reserved and should be ignored
				if (action == 4U)
					return 0;

				// If the index is being suppressed, it changes what happens next
				if (extra & 0x0040U)
				{
					// If the action is above 4, it is reserved in this mode
					if (action > 4U)
						return 0;
					return readExtraDisplacement(action);
				}
				// All combinations are allowed, get the displacement back
				return readExtraDisplacement(action & 0x03U);
			}()
		};

		// If indexing is being suppressed
		if (extra & 0x0040U)
		{
			// Determine what outer displacement needs applying (treat reserved modes as no memory indirection)
			switch (action)
			{
				// Memory Indirect with Outer Displacement
				case 1U:
				case 2U:
				case 3U:
				{
					// Start by pulling back the indirect memory address to use
					const auto indirectAddress{_peripherals.readAddress<uint32_t>(baseAddress + baseDisplacement)};
					// Now compute the final address
					return indirectAddress + outerDisplacement;
				}
			}
		}
		else
		{
			// Determine what outer displacement needs applying (treat reserved modes as no memory indirection)
			switch (action)
			{
				// Indirect Pre-Indexed with Outer Displacement
				case 1U:
				case 2U:
				case 3U:
				{
					// Start by pulling back the indexed indirect memory address to use
					const auto indirectAddress{_peripherals.readAddress<uint32_t>(baseAddress + baseDisplacement + index)};
					// Now build the final address
					return indirectAddress + outerDisplacement;
				}
				// Indirect Post-Indexed with Outer Displacement
				case 5U:
				case 6U:
				case 7U:
				{
					// Start by pulling back the indirect memory address to use
					const auto indirectAddress{_peripherals.readAddress<uint32_t>(baseAddress + baseDisplacement)};
					// Now build the indexed final address
					return indirectAddress + index + outerDisplacement;
				}
			}
		}

		return baseAddress + baseDisplacement + index;
	}
	else
	{
		const auto index{readIndex(extra)};
		const auto displacement{int8_t(extra)};
		return baseAddress + displacement + index;
	}
}

uint32_t motorola68000_t::computeEffectiveAddress(const uint8_t mode, const uint8_t reg, const size_t operandSize) noexcept
{
	switch (mode)
	{
		case 2U: // (An)
			return a[reg];
		case 3U: // (An)+
		{
			const auto ptr{addrRegister(reg)};
			addrRegister(reg) = ptr + operandSize;
			return ptr;
		}
		case 4U: // -(An)
		{
			const auto ptr{addrRegister(reg) - operandSize};
			addrRegister(reg) = ptr;
			return ptr;
		}
		case 5U: // (d16,An)
		{
			const auto displacement{_peripherals.readAddress<int16_t>(programCounter)};
			programCounter += 2U;
			return addrRegister(reg) + displacement;
		}
		case 6U:
			// (d8,An,Xn.SIZE*SCALE), (bd,An,Xn.SIZE*SCALE),
			// ([bd,An],Xn.SIZE*SCALE,od), and ([bd,An,Xn.SIZE*SCALE].od)
			return computeIndirect(addrRegister(reg));
		case 7U: // PC-rel and absolute modes
			switch (reg)
			{
				case 0U: // (xxx).W
				{
					const auto ptr{_peripherals.readAddress<int16_t>(programCounter)};
					programCounter += 2U;
					return static_cast<uint32_t>(int32_t{ptr});
				}
				case 1U: // (xxx).L
				{
					const auto ptr{_peripherals.readAddress<int32_t>(programCounter)};
					programCounter += 4U;
					return static_cast<uint32_t>(ptr);
				}
				case 2U: // (d16,PC)
				{
					const auto ptr{_peripherals.readAddress<int16_t>(programCounter) + programCounter};
					programCounter += 2U;
					return ptr;
				}
				case 3U:
					// (d8,PC,Xn.SIZE*SCALE), (bd,PC,Xn.SIZE*SCALE),
					// ([bd,PC],Xn.SIZE*SCALE,od), and ([bd,PC,Xn.SIZE*SCALE],od)
					return computeIndirect(programCounter);
				case 4U: // #<xxx>
					// u8 and u16 operation is trivial enough
					if (operandSize == 1U || operandSize == 2U)
					{
						// Read the u16 that follows the instructioon
						const auto value{_peripherals.readAddress<uint16_t>(programCounter)};
						programCounter += 2U;
						// Truncate if only a byte is needed
						if (operandSize == 1U)
							return uint8_t(value);
						return value;
					}
					// u32 is even easier, just read the u32 and return it
					if (operandSize == 4U)
					{
						const auto value{_peripherals.readAddress<uint32_t>(programCounter)};
						programCounter += 4U;
						return value;
					}
					// TODO: Deal with float modes
					break;
			}
	}
	// Shouldn't be able to get here, but just in case..
	return UINT32_MAX;
}

// Template helper that allows us to pull out the different underlying storage types for a given result type
template<typename T> struct underlyingStorageFor_t;

// u32 result type underlying storage
template<> struct underlyingStorageFor_t<uint32_t>
{
	using i8 = uint8_t;
	using i16 = uint16_t;
	using i32 = uint32_t;
};

// s32 result type underlying storage
template<> struct underlyingStorageFor_t<int32_t>
{
	using i8 = int8_t;
	using i16 = int16_t;
	using i32 = int32_t;
};

template<typename T>
	T motorola68000_t::readValue(const uint8_t mode, const uint8_t reg, const uint32_t address) noexcept
{
	// Dispatch the effective addressing mode used
	switch (mode)
	{
		case 0U: // Dn
			return static_cast<T>(dataRegister(reg));
		case 1U: // An
			return static_cast<T>(addrRegister(reg));
		default:
		{
			if (mode == 7U && reg == 4U) // #<xxx> form so just return the value
				return static_cast<T>(address);
			// Everything else needs us to actually access memory
			return _peripherals.readAddress<T>(address);
		}
	}
}

template int8_t motorola68000_t::readValue<int8_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
template uint8_t motorola68000_t::readValue<uint8_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
template int16_t motorola68000_t::readValue<int16_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
template uint16_t motorola68000_t::readValue<uint16_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
template int32_t motorola68000_t::readValue<int32_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
template uint32_t motorola68000_t::readValue<uint32_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;

template<typename T> T motorola68000_t::readValue(
	const uint8_t mode, const uint8_t reg, const uint32_t address, const size_t width) noexcept
{
	// Get the underlying storage types for our result type
	using underlyingStorage_t = underlyingStorageFor_t<T>;
	// Dispatch the access width requested
	switch (width)
	{
		case 1U:
			return readValue<typename underlyingStorage_t::i8>(mode, reg, address);
		case 2U:
			return readValue<typename underlyingStorage_t::i16>(mode, reg, address);
		case 4U:
			return readValue<typename underlyingStorage_t::i32>(mode, reg, address);
	}
	// Should be impossible to get here.. but just in case
	return UINT32_MAX;
}

template uint32_t motorola68000_t::readValue<uint32_t>(uint8_t mode, uint8_t reg, uint32_t address, size_t width) noexcept;
template int32_t motorola68000_t::readValue<int32_t>(uint8_t mode, uint8_t reg, uint32_t address, size_t width) noexcept;

template<typename T>
	T motorola68000_t::readEffectiveAddress(const uint8_t mode, const uint8_t reg, const size_t width) noexcept
{
	// Compute the effective address to use and read
	return readValue<T>(mode, reg, computeEffectiveAddress(mode, reg, width), width);
}

template uint32_t motorola68000_t::readEffectiveAddress<uint32_t>(uint8_t mode, uint8_t reg, size_t width) noexcept;
template int32_t motorola68000_t::readEffectiveAddress<int32_t>(uint8_t mode, uint8_t reg, size_t width) noexcept;

template<typename T>
	void motorola68000_t::writeValue(const uint8_t mode, const uint8_t reg, const uint32_t address, const T value) noexcept
{
	switch (mode)
	{
		case 0U: // Dn
			dataRegister(reg) = value;
			break;
		case 1U: // An
			addrRegister(reg) = value;
			break;
		default:
			_peripherals.writeAddress<T>(address, value);
			break;
	}
}

template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, int8_t value) noexcept;
template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, uint8_t value) noexcept;
template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, int16_t value) noexcept;
template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, uint16_t value) noexcept;
template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, int32_t value) noexcept;
template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, uint32_t value) noexcept;

template<typename T> void motorola68000_t::writeValue(
	const uint8_t mode, const uint8_t reg, const uint32_t address, const size_t width, T value) noexcept
{
	// Get the underlying storage types for our result type
	using underlyingStorage_t = underlyingStorageFor_t<T>;
	// Dispatch the access width requested
	switch (width)
	{
		case 1U:
			writeValue(mode, reg, address, static_cast<typename underlyingStorage_t::i8>(value));
			break;
		case 2U:
			writeValue(mode, reg, address, static_cast<typename underlyingStorage_t::i16>(value));
			break;
		case 4U:
			writeValue(mode, reg, address, static_cast<typename underlyingStorage_t::i32>(value));
			break;
	}
}

template void
	motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, size_t width, uint32_t value) noexcept;
template void
	motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, size_t width, int32_t value) noexcept;

template<typename T> void motorola68000_t::writeEffectiveAddress(
	const uint8_t mode, const uint8_t reg, const size_t width, const T value) noexcept
{
	// Compute the effective address to use and write
	writeValue(mode, reg, computeEffectiveAddress(mode, reg, width), width, value);
}

template void motorola68000_t::writeEffectiveAddress(uint8_t mode, uint8_t reg, size_t width, uint32_t value) noexcept;
template void motorola68000_t::writeEffectiveAddress(uint8_t mode, uint8_t reg, size_t width, int32_t value) noexcept;

uint32_t &motorola68000_t::activeStackPointer() noexcept
{
	// XXX: Need to deal with interrupt contexts and the interrupt stack pointer..
	if (status.includes(m68kStatusBits_t::supervisor))
		return systemStackPointer;
	return userStackPointer;
}

int32_t motorola68000_t::readImmediateDisplacement(const decodedOperation_t &insn) noexcept
{
	// If the branch includes the displacement in the instruction itself, extract and sign extend it
	if (insn.trailingBytes == 0U)
		return int8_t(insn.rx);
	// If the branch is for a 16-bit displacement, extract, and sign-extend it
	if (insn.trailingBytes == 2U)
		return _peripherals.readAddress<int16_t>(programCounter);
	// Otherwise, extract a 32-bit displacement
	return _peripherals.readAddress<int32_t>(programCounter);
}

bool motorola68000_t::checkCondition(const uint8_t condition) const noexcept
{
	switch (condition)
	{
		case 0x0U: // T
			return true;
		case 0x1U: // F
			return false;
		case 0x2U: // HI
			return status.excludes(m68kStatusBits_t::carry, m68kStatusBits_t::zero);
		case 0x3U: // LS
			return status.includes(m68kStatusBits_t::carry) || status.includes(m68kStatusBits_t::zero);
		case 0x4U: // CC
			return status.excludes(m68kStatusBits_t::carry);
		case 0x5U: // CS
			return status.includes(m68kStatusBits_t::carry);
		case 0x6U: // NE
			return status.excludes(m68kStatusBits_t::zero);
		case 0x7U: // EQ
			return status.includes(m68kStatusBits_t::zero);
		case 0x8U: // VC
			return status.excludes(m68kStatusBits_t::overflow);
		case 0x9U: // VS
			return status.includes(m68kStatusBits_t::overflow);
		case 0xaU: // PL
			return status.excludes(m68kStatusBits_t::negative);
		case 0xbU: // MI
			return status.includes(m68kStatusBits_t::negative);
		case 0xcU: // GE
			return
				status.includes(m68kStatusBits_t::negative, m68kStatusBits_t::overflow) ||
				status.excludes(m68kStatusBits_t::negative, m68kStatusBits_t::overflow);
		case 0xdU: // LT
			return
				(status.includes(m68kStatusBits_t::negative) && status.excludes(m68kStatusBits_t::overflow)) ||
				(status.excludes(m68kStatusBits_t::negative) && status.includes(m68kStatusBits_t::overflow));
		case 0xeU: // GT
			return
				(status.includes(m68kStatusBits_t::negative, m68kStatusBits_t::overflow) ||
				status.excludes(m68kStatusBits_t::negative, m68kStatusBits_t::overflow)) &&
				status.excludes(m68kStatusBits_t::zero);
		case 0xfU: // LE
			return
				status.includes(m68kStatusBits_t::zero) ||
				(status.includes(m68kStatusBits_t::negative) && status.excludes(m68kStatusBits_t::overflow)) ||
				(status.excludes(m68kStatusBits_t::negative) && status.includes(m68kStatusBits_t::overflow));
	}
	// Impossible, but just in case
	return false;
}

size_t motorola68000_t::unpackSize(const uint8_t sizeField) const noexcept
{
	// u8 operation size
	if (sizeField == 0U)
		return 1U;
	// u16 operation size
	if (sizeField == 1U)
		return 2U;
	// u32 operation size
	return 4U;
}

void motorola68000_t::recomputeStatusFlags(uint32_t lhs, uint32_t rhs, uint64_t result, size_t operationSize) noexcept
{
	// Compute which bit is the sign bit of the result
	const auto signBit{1U << ((8U * operationSize) - 1U)};

	// Recompute all the flags, starting with the negative bit
	if (result & signBit)
		status.set(m68kStatusBits_t::negative);
	else
		status.clear(m68kStatusBits_t::negative);
	// Then the zero bit
	if (result == 0U)
		status.set(m68kStatusBits_t::zero);
	else
		status.clear(m68kStatusBits_t::zero);
	// Check if overflow occurred during the calculation
	const auto overflow
	{
		[&]() -> bool
		{
			// If the sign bits of the inputs disagree, this can't overflow
			if ((lhs & signBit) != (rhs & signBit))
				return false;
			// However, if they do agree and the sign of the output is different, we did
			return (lhs & signBit) != (result & signBit);
		}()
	};
	if (overflow)
		status.set(m68kStatusBits_t::overflow);
	else
		status.clear(m68kStatusBits_t::overflow);
	// And finally check and see if the calculation would generate a carry
	if (result & (UINT64_C(1) << 32U))
		status.set(m68kStatusBits_t::extend, m68kStatusBits_t::carry);
	else
		status.clear(m68kStatusBits_t::extend, m68kStatusBits_t::carry);
}

int32_t motorola68000_t::readDataRegisterSigned(const size_t reg, const size_t size) const noexcept
{
	// Extract the register's value
	const auto value{d[reg]};
	// Properly sign extend it to int32_t based on the operation size to be performed
	if (size == 1U)
		return static_cast<int8_t>(value);
	if (size == 2U)
		return static_cast<int16_t>(value);
	return static_cast<int32_t>(value);
}

void motorola68000_t::writeDataRegisterSized(const size_t reg, const size_t size, const uint32_t value) noexcept
{
	// Write the result back to the register, preserving bits not touched by the width of the operation
	dataRegister(reg) = static_cast<uint32_t>([&]() -> int32_t
	{
		// Extract the current register value
		const auto data{dataRegister(reg)};
		// Narrow the result and overwrite just those bits in the register value
		if (size == 1U)
			return (data & 0xffffff00U) | static_cast<uint8_t>(value);
		if (size == 2U)
			return (data & 0xffff0000U) | static_cast<uint16_t>(value);
		return static_cast<uint32_t>(value);
	}());
}

stepResult_t motorola68000_t::dispatchADD(const decodedOperation_t &insn) noexcept
{
	// Unpack the operation size to a value in bytes
	const auto operationSize{unpackSize(insn.operationSize)};
	// Figure out the effective address operand as much as possible so we know where to go poking
	const auto effectiveAddress{computeEffectiveAddress(insn.mode, insn.ry, operationSize)};
	// Grab the data register value used as one of the operands (add is commutative, so don't care
	// which order we grab them here.. unlike for subtract which matters)
	const auto lhs{static_cast<uint32_t>(readDataRegisterSigned(insn.rx, operationSize))};
	// Grab the other operand using the computed address
	const auto rhs{static_cast<uint32_t>(readValue<int32_t>(insn.mode, insn.ry, effectiveAddress))};
	// With the two values retreived, do the addition
	const auto result{uint64_t{lhs} + uint64_t{rhs}};

	// Recompute all the flags
	recomputeStatusFlags(lhs, rhs, result, operationSize);

	// Store the result back
	if (insn.opMode == 0U)
		writeDataRegisterSized(insn.rx, operationSize, static_cast<uint32_t>(result));
	else
		writeValue(insn.mode, insn.ry, effectiveAddress, operationSize, static_cast<uint32_t>(result));

	// Get done and figure out how many cycles that took
	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchADDA(const decodedOperation_t &insn) noexcept
{
	// Extract the value to be added to the target address register
	const auto value{readEffectiveAddress<uint32_t>(insn.mode, insn.ry, insn.operationSize)};
	// Now update the target address register
	addrRegister(insn.rx) += value;
	// Figure out how long this operation took and finish up
	return {true, false, insn.operationSize == 2U ? 8U : 6U};
}

stepResult_t motorola68000_t::dispatchADDQ(const decodedOperation_t &insn) noexcept
{
	// Unpack the operation size to a value in bytes
	const auto operationSize{unpackSize(insn.operationSize)};
	// Figure out the effective address operand as much as possible so we know where to go poking
	const auto effectiveAddress{computeEffectiveAddress(insn.mode, insn.ry, operationSize)};
	// Grab the LHS using the computed address
	const auto lhs{static_cast<uint32_t>(readValue<int32_t>(insn.mode, insn.ry, effectiveAddress, operationSize))};
	// Unpack the value to add as the RHS (remapping 0 as 8)
	const uint32_t rhs{insn.rx == 0U ? 8U : insn.rx};
	// With the two values retreived, do the addition
	const auto result{uint64_t{lhs} + uint64_t{rhs}};

	// Recompute all the flags if not poking an address register
	if (insn.mode != 1U)
		recomputeStatusFlags(lhs, rhs, result, operationSize);

	// Store the result back
	writeValue(insn.mode, insn.ry, effectiveAddress, operationSize, static_cast<uint32_t>(result));

	// Get done and figure out how many cycles that took
	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchANDI(const decodedOperation_t &insn) noexcept
{
	// Deal with the special register forms first
	switch (insn.ry)
	{
		case 8U:
		case 9U:
			return dispatchANDISpecialCCR(insn);
	}
	// Unpack the operation size to a value in bytes
	const auto operationSize{unpackSize(insn.operationSize)};
	// Extract the immediate that follows this instruction
	const auto rhs{readImmediateUnsigned(operationSize)};
	// Figure out the effective address operand as much as possible so we know where to go poking
	const auto effectiveAddress{computeEffectiveAddress(insn.mode, insn.ry, operationSize)};
	// Grab the LHS using the computed address
	const auto lhs{readValue<uint32_t>(insn.mode, insn.ry, effectiveAddress, operationSize)};
	// Apply the masking operation
	const auto result{lhs & rhs};

	// Compute which bit is the sign bit of the result
	const auto signBit{1U << ((8U * operationSize) - 1U)};
	// Recompute all the flags, starting with the negative bit
	if (result & signBit)
		status.set(m68kStatusBits_t::negative);
	else
		status.clear(m68kStatusBits_t::negative);
	// Then the zero bit
	if (result == 0U)
		status.set(m68kStatusBits_t::zero);
	else
		status.clear(m68kStatusBits_t::zero);
	// The overflow and carry bits are always cleared by this instruction
	status.clear(m68kStatusBits_t::carry, m68kStatusBits_t::overflow);

	// Store the result back
	writeValue(insn.mode, insn.ry, effectiveAddress, result);

	// Return how many cycles that took
	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchANDISpecialCCR(const decodedOperation_t &insn) noexcept
{
	// If this instruction is for the SR specifically, it is privileged, so check that
	if (insn.ry == 9U && status.excludes(m68kStatusBits_t::supervisor))
		return {true, true, 0U};
	// Extract the immediate that follows this instruction and widen it to 16-bit
	const auto rhs{static_cast<uint16_t>(readImmediateUnsigned(1U) | 0xff00U)};
	// Grab the SR
	const auto lhs{status.toRaw()};
	// Apply the mask and write it back
	status.fromRaw(lhs & rhs);

	// Return how many cycles that took
	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchBcc(const decodedOperation_t &insn) noexcept
{
	// Extract the displacement for this instruction
	const auto displacement{readImmediateDisplacement(insn)};
	// Determine if the branch condition is met
	const auto branch{checkCondition(insn.opMode)};
	// If we should branch, update the program counter with the displacement
	if (branch)
		programCounter += displacement;
	// Otherwise, update it with the size of the displacement to get past the trailing bytes
	else
		programCounter += insn.trailingBytes;
	// Get done and figure out how long this took
	return
	{
		true, false,
		[&]() -> size_t
		{
			if (branch) // If the branch is taken, it's a constant amount
				return 10U;
			if (insn.trailingBytes == 0U) // If it is not taken, and there are no trailing bytes
				return 8U;
			return 12U; // If it is not taken and there are trailing bytes
		}()
	};
}

stepResult_t motorola68000_t::dispatchBCLR(const decodedOperation_t &insn) noexcept
{
	// Determine if the target is a data register or not
	const auto isDataReg{insn.mode == 0U};
	const size_t operationSize{isDataReg ? 4U : 1U};
	// Grab the bit number to poke at
	const auto bitIndex
	{
		[&]() -> uint8_t
		{
			// If the bit number is coming from a trailing immediate
			if (insn.flags.includes(operationFlags_t::immediateNotRegister))
			{
				// Grab back the u16 for that, adjusting the program counter
				const auto result{_peripherals.readAddress<uint16_t>(programCounter)};
				programCounter += 2U;
				// We're actually only interested in the bottom 8 bits, so discard the rest
				return static_cast<uint8_t>(result);
			}
			// Otherwise, if the bit number is coming from a register, extract that
			else
				return static_cast<uint8_t>(dataRegister(insn.rx));
		}() & ((operationSize * 8U) - 1U) // Make sure the bit number is in range for the destination width
	};
	// Now extract the address of the manipulation target
	const auto effectiveAddress{computeEffectiveAddress(insn.mode, insn.ry, operationSize)};
	// Read back the value at the destination
	const auto value{readValue<uint32_t>(insn.mode, insn.ry, effectiveAddress, operationSize)};

	// Test to see if the bit at the requested position is zero or not
	if (value & (1U << bitIndex))
		status.clear(m68kStatusBits_t::zero);
	else
		status.set(m68kStatusBits_t::zero);
	// Now zero that bit position and write it back, then return
	writeValue(insn.mode, insn.ry, effectiveAddress, operationSize, uint32_t{value & ~(1U << bitIndex)});
	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchBRA(const decodedOperation_t &insn) noexcept
{
	// Extract the displacement for this instruction
	const auto displacement{readImmediateDisplacement(insn)};
	// Now we have a displacement, update the program counter and get done
	programCounter += displacement;
	return {true, false, 10U};
}

stepResult_t motorola68000_t::dispatchBSR(const decodedOperation_t &insn) noexcept
{
	// Extract the displacement for this instruction
	const auto displacement{readImmediateDisplacement(insn)};
	// Now we have the displacement, calculate the post-instruction program counter, and push it to stack
	auto &stackPointer{activeStackPointer()};
	stackPointer -= 4U;
	_peripherals.writeAddress(stackPointer, programCounter + insn.trailingBytes);
	// Now update the program counter to the new execution address and get done
	programCounter += displacement;
	return {true, false, 18U};
}

stepResult_t motorola68000_t::dispatchCLR(const decodedOperation_t &insn) noexcept
{
	// Figure out the operation width
	const auto operationSize{unpackSize(insn.operationSize)};
	// Clear (0) the target of the effective address
	writeEffectiveAddress(insn.mode, insn.ry, operationSize, uint32_t{0U});
	// Set the condition flags for zero
	status.clear(m68kStatusBits_t::carry, m68kStatusBits_t::overflow, m68kStatusBits_t::negative);
	status.set(m68kStatusBits_t::zero);

	// Figure out how long that all took and return
	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchCMP(const decodedOperation_t &insn) noexcept
{
	// Figure out the operation width
	const auto operationSize{unpackSize(insn.operationSize)};
	// Grab the data register to be used on the LHS of the subtraction
	const auto lhs{static_cast<uint32_t>(readDataRegisterSigned(insn.rx, operationSize))};
	// Grab the data to be used on the RHS of the subtraction from the EA
	const auto rhs{static_cast<uint32_t>(readEffectiveAddress<int32_t>(insn.mode, insn.ry, operationSize))};
	// With the two values retreived, do the subtraction
	// We do the subtraction unsigned so we don't UB on overflow
	const auto result{uint64_t{lhs} - uint64_t{rhs}};

	// Recompute all the flags
	recomputeStatusFlags(lhs, ~rhs + 1U, result, operationSize);

	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchCMPI(const decodedOperation_t &insn) noexcept
{
	// Figure out the operation width
	const auto operationSize{unpackSize(insn.operationSize)};
	// Grab the data to be used on the RHS of the subtraction from just after the instruction
	const auto lhs{static_cast<uint32_t>(readImmediateSigned(operationSize))};
	// Grab the data to be used on the LHS of the subtraction from the EA
	const auto rhs{static_cast<uint32_t>(readEffectiveAddress<int32_t>(insn.mode, insn.ry, operationSize))};
	// With the two values retreived, do the subtraction
	// We do the subtraction unsigned so we don't UB on overflow
	const auto result{uint64_t{lhs} - uint64_t{rhs}};

	// Recompute all the flags
	recomputeStatusFlags(lhs, ~rhs + 1U, result, operationSize);

	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchDBcc(const decodedOperation_t &insn) noexcept
{
	// Grab the current program counter value as this is used as the base value for the displacement calculation
	const auto branchBase{programCounter};
	// Extract the displacement specified by the instruction and
	// set the program counter for just after the instruction
	const auto displacement{readImmediateDisplacement(insn)};
	programCounter += insn.trailingBytes;
	// Now test the condition for the instruction
	const auto condition{checkCondition(insn.opMode)};
	// If it is false, then we decrement the data register for the instruction
	if (!condition)
	{
		// Grab the result of this as we have a further test to do
		const auto value{static_cast<int32_t>(--dataRegister(insn.ry))};
		// If the result of that calculation is not -1, adjust the program counter by the displacement
		if (value != -1)
			programCounter = branchBase + displacement;
	}
	// Get done and compute how long that took
	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchLEA(const decodedOperation_t &insn) noexcept
{
	// Determine the effective address that would be accessed by this instruction
	// and then write it to the indicated address register from the instruction
	writeAddrRegister(insn.rx, computeEffectiveAddress(insn.mode, insn.ry, 0U));
	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchMOVE(const decodedOperation_t &insn) noexcept
{
	// Detect and handle the special MOVE instructions (USP, CCR, and SR)
	switch (std::max(insn.rx, insn.ry))
	{
		case 8U:
			return dispatchMOVESpecialCCR(insn);
		case 9U:
			return dispatchMOVESpecialSR(insn);
		case 10U:
			return dispatchMOVESpecialUSP(insn);
	}

	// Extract out the mode parts of the effective addresses
	const auto srcEAMode{insn.mode & 0x07U};
	const auto dstEAMode{(insn.mode & 0x38U) >> 3U};
	// Read the data to be moved, allowing it to sign extend to make resetting the flags easier
	const auto value{readEffectiveAddress<int32_t>(srcEAMode, insn.ry, insn.operationSize)};
	// Clear flags that are always cleared
	status.clear(m68kStatusBits_t::carry, m68kStatusBits_t::overflow);
	// Check if the value is zero
	if (value == 0)
		status.set(m68kStatusBits_t::zero);
	else
		status.clear(m68kStatusBits_t::zero);
	// Check if the value is negative
	if (value < 0)
		status.set(m68kStatusBits_t::negative);
	else
		status.clear(m68kStatusBits_t::negative);

	// Now put the value into the destination location
	writeEffectiveAddress(dstEAMode, insn.rx, insn.operationSize, value);

	// Get done and figure out how long the execution took - NB, this is massively dependant
	// on the operating modes, forming a large matrix
	return {true, false, 4U};
}

stepResult_t motorola68000_t::dispatchMOVESpecialCCR(const decodedOperation_t &insn) noexcept
{
	// Determine if this is a from (true) or to (false) move
	const auto direction{insn.rx == 8U};
	// Using that, dispatch the move
	if (direction)
	{
		// Grab the CCR portion of the SR
		const auto ccr{uint8_t(status.toRaw())};
		// Write it to the target location, 0-extended
		writeEffectiveAddress(insn.mode, insn.ry, uint16_t{ccr});
		// Execution takes an unknown amount of time.. *sighs*
		return {true, false, 0U};
	}
	else
	{
		// Grab the value from the target location and truncate to just the CCR byte
		const auto ccr{uint8_t(readEffectiveAddress<uint16_t>(insn.mode, insn.rx))};
		// Get back the status register value and patch
		status.fromRaw((status.toRaw() & 0xff00U) | ccr);
		// Execution takes a fixed amount of time thankfully
		return {true, false, 12U};
	}
}

stepResult_t motorola68000_t::dispatchMOVESpecialSR(const decodedOperation_t &insn) noexcept
{
	// Determine if this is a from (true) or to (false) move
	const auto direction{insn.rx == 9U};
	// Using that, dispatch the move
	if (direction)
	{
		// Copy the SR to the target location
		writeEffectiveAddress(insn.mode, insn.ry, status.toRaw());
		// Execution time depends on where the register got moved to
		return
		{
			true, false,
			insn.ry == 0U ? 6U : 8U
		};
	}
	else
	{
		// Check that we're in supervisor state, and if not, abort
		if (status.excludes(m68kStatusBits_t::supervisor))
			return {true, true, 0U};
		// Grab the new SR value and stuff it into the register
		status.fromRaw(readEffectiveAddress<uint16_t>(insn.mode, insn.rx));
		// Execution takes a fixed amount of time thankfully
		return {true, false, 12U};
	}
}

stepResult_t motorola68000_t::dispatchMOVESpecialUSP(const decodedOperation_t &insn) noexcept
{
	// Check that we're in supervisor state, and if not, abort
	if (status.excludes(m68kStatusBits_t::supervisor))
		return {true, true, 0U};

	// Determine if this is a from (true) or to (false) move
	const auto direction{insn.rx == 10U};
	// Using that, dispatch the move
	if (direction)
		// Copy the USP to the target location
		writeEffectiveAddress(insn.mode, insn.ry, userStackPointer);
	else
		// Grab the new stack pointer value and stuff it into the register
		userStackPointer = readEffectiveAddress<uint32_t>(insn.mode, insn.rx);

	// Get done, execution takes a fixed amount of time thankfuly
	return {true, false, 4U};
}

stepResult_t motorola68000_t::dispatchMOVEA(const decodedOperation_t &insn) noexcept
{
	// Read the data to be moved, allowing it to sign extend to fill the whole target address register
	const auto value{readEffectiveAddress<int32_t>(insn.mode, insn.ry, insn.operationSize)};
	// Store the value into the target address register and report the execution time
	addrRegister(insn.rx) = static_cast<uint32_t>(value);
	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchMOVEM(const decodedOperation_t &insn) noexcept
{
	// Extract the list of registers to move
	const auto registerList{_peripherals.readAddress<uint16_t>(programCounter)};
	programCounter += 2U;
	// Determine whether we're in postincrement mode or not
	const auto isPostincrement{insn.mode == 3U};
	// Determine whether we're in predecrement mode or not
	const auto isPredecrement{insn.mode == 4U};
	// Determine whether we're in a complex EA mode (something that has instruction-trailing addressing bytes)
	const auto complexMode{!(isPostincrement || isPredecrement)};
	// Figure out the base address for where to move the registers to if in a complex EA mode
	auto address
	{
		[&]() -> uint32_t
		{
			if (complexMode)
				return computeEffectiveAddress(insn.mode, insn.ry, insn.operationSize);
			return 0U;
		}()
	};
	// Keep a tally of the number of registers transferred
	size_t copied{0U};
	// Loop through all 16 possible register slots
	for (const auto idx : substrate::indexSequence_t{16U})
	{
		// Check if this register should be moved or not
		if ((registerList & (1U << idx)) == 0U)
			continue; // Skip this one then

		// Determine which register to move
		auto &reg
		{
			[&]() -> uint32_t &
			{
				// If it's predecrement, the whole lot is backwards - a7-a0, then d7-d0
				if (isPredecrement)
				{
					if (idx < 8U)
						return addrRegister(7U - idx);
					else
						return dataRegister(7U - (idx - 8U));
				}
				// Otherwise, if it's any other mode, it's d0-d7, then a0-a7
				else
				{
					if (idx < 8U)
						return dataRegister(idx);
					else
						return addrRegister(idx - 8U);
				}
			}()
		};

		// For the simple EA modes, figure out to what address we should move this register
		if (!complexMode)
			address = computeEffectiveAddress(insn.mode, insn.ry, insn.operationSize);

		// Determine transfer direction (1 = mem -> reg, 0 = reg -> mem)
		if (insn.opMode)
		{
			// Grab the value to update the register to from memory
			const auto value
			{
				[&]() -> int32_t
				{
					if (insn.operationSize == 2U)
						return _peripherals.readAddress<int16_t>(address);
					return _peripherals.readAddress<int32_t>(address);
				}()
			};
			// And update the selected register with that value
			reg = static_cast<uint32_t>(value);
		}
		else
		{
			// Put the register's value to memory
			if (insn.operationSize == 2U)
				_peripherals.writeAddress(address, uint16_t(reg));
			else
				_peripherals.writeAddress(address, reg);
		}

		// If we're in a simple mode, update the address appropriately
		if (isPostincrement)
			address += insn.operationSize;
		else if (isPredecrement)
			address -= insn.operationSize;

		// Mark another register copied for the number of cycles taken
		++copied;
	}

	// Get done and mark out how many cycles this took
	// XXX: Need to take into account the EA mode fully.. between 12 and 20 cycles extra depending on mode.
	return {true, false, 4U * copied};
}

stepResult_t motorola68000_t::dispatchMOVEQ(const decodedOperation_t &insn) noexcept
{
	// Make the value signed so when we copy it to the target register, it sign-extends
	// and so that flags generation works propeprly
	const int32_t value{static_cast<int8_t>(insn.ry)};
	// Clear flags that are always cleared
	status.clear(m68kStatusBits_t::carry, m68kStatusBits_t::overflow);
	// Check if the value is zero
	if (value == 0)
		status.set(m68kStatusBits_t::zero);
	else
		status.clear(m68kStatusBits_t::zero);
	// Check if the value is negative
	if (value < 0)
		status.set(m68kStatusBits_t::negative);
	else
		status.clear(m68kStatusBits_t::negative);

	// Move the value to the target data register and return
	dataRegister(insn.rx) = static_cast<uint32_t>(value);
	return {true, false, 4U};
}

stepResult_t motorola68000_t::dispatchRTS() noexcept
{
	// Extract the new program counter value from the current stack pointer
	auto &stackPointer{activeStackPointer()};
	programCounter = _peripherals.readAddress<uint32_t>(stackPointer);
	// Now adjust the stack pointer by the size of an address
	stackPointer += 4U;
	// RTS thankfully takes a fixed number of cycles, so return that
	return {true, false, 16U};
}

stepResult_t motorola68000_t::dispatchScc(const decodedOperation_t &insn) noexcept
{
	// Figure out what state the condition for this instruction is in
	const bool state{checkCondition(insn.opMode)};
	// Turn that into either all-highs (true) or lows (false) and write it to the EA specified
	writeEffectiveAddress(insn.mode, insn.ry, uint8_t(state ? UINT8_MAX : 0U));

	// Get done and mark how many cycles this took
	return
	{
		true, false,
		[&]() -> size_t
		{
			// If this was a write to a register
			if (insn.mode == 0U)
			{
				// And we wrote a true
				if (state)
					return 6U;
				// Othewise we wrote a false
				return 4U;
			}
			// Everything else is constant access time
			return 8U;
		}()
	};
}

stepResult_t motorola68000_t::dispatchSUB(const decodedOperation_t &insn) noexcept
{
	// Unpack the operation size to a value in bytes
	const auto operationSize{unpackSize(insn.operationSize)};
	// Figure out the effective address operand as much as possible so we know where to go poking
	const auto effectiveAddress{computeEffectiveAddress(insn.mode, insn.ry, operationSize)};
	// Figure out which direction the operation is going in
	const auto eaAsSource{insn.opMode == 0U};
	// Grab the LHS operand
	const auto lhs
	{
		static_cast<uint32_t>([&]() -> int32_t
		{
			// If the EA operand is the destination, it is also the LHS operand
			if (!eaAsSource)
				return readValue<int32_t>(insn.mode, insn.ry, effectiveAddress, operationSize);
			// Otherwise the LHS is the data register
			return readDataRegisterSigned(insn.rx, operationSize);
		}())
	};
	// Grab the RHS operand
	const auto rhs
	{
		static_cast<uint32_t>([&]() -> int32_t
		{
			// If the EA operand is the source, it is then the RHS operand
			if (eaAsSource)
				return readValue<int32_t>(insn.mode, insn.ry, effectiveAddress, operationSize);
			// Otherwise the RHS is the data register
			return readDataRegisterSigned(insn.rx, operationSize);
		}())
	};
	// With the two values retreived, do the subtraction
	const auto result{uint64_t{lhs} - uint64_t{rhs}};

	// Recompute all the flags
	recomputeStatusFlags(lhs, ~rhs + 1U, result, operationSize);

	// Store the result back
	if (eaAsSource)
		writeDataRegisterSized(insn.rx, operationSize, static_cast<uint32_t>(result));
	else
		writeValue(insn.mode, insn.ry, effectiveAddress, operationSize, static_cast<uint32_t>(result));

	// Get done and figure out how many cycles that took
	return {true, false, 0U};
}

stepResult_t motorola68000_t::dispatchTST(const decodedOperation_t &insn) noexcept
{
	// Unpack the operation size to a value in bytes
	const auto operationSize{unpackSize(insn.operationSize)};
	// Get the value to test (sign-extended to make negative testing easier)
	const auto value{readEffectiveAddress<int32_t>(insn.mode, insn.ry, operationSize)};
	// Clear flags that are always cleared
	status.clear(m68kStatusBits_t::carry, m68kStatusBits_t::overflow);
	// Check if the value is zero
	if (value == 0)
		status.set(m68kStatusBits_t::zero);
	else
		status.clear(m68kStatusBits_t::zero);
	// Check if the value is negative
	if (value < 0)
		status.set(m68kStatusBits_t::negative);
	else
		status.clear(m68kStatusBits_t::negative);

	// Get done and mark how many cycles this took
	return {true, false, 4U};
}
