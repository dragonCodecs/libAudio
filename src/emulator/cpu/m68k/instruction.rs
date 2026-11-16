// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use bitmask_enum::bitmask;

#[derive(Clone, Copy, PartialEq, Eq)]
struct DecodedOperation
{
	operation: Instruction,
	rx: u8,
	ry: u8,
	flags: OperationFlags,
	operation_size: u8,
	op_mode: u8,
	mode: u8,
	trailing_bytes: u8,
}

#[bitmask(u8)]
enum OperationFlags
{
	/// Operation is on memory, not a register
	MemoryNotRegister,
	/// Operation uses rx as immediate not register
	ImmediateNotRegister,
	/// Operation is post-increment mode
	Postincrement,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum Instruction
{
	// Data movement instructions
	Exg,
	Fmove,
	Fsmove,
	Fdmove,
	Fmovem,
	Lea,
	Link,
	Move,
	Move16,
	Movea,
	Movem,
	Movep,
	Moveq,
	Pea,
	Unlk,

	// Integer arithmetic instructions
	Add,
	Adda,
	Addi,
	Addq,
	Addx,
	Clr,
	Cmp,
	Cmpa,
	Cmpi,
	Cmpm,
	Divs,
	Divu,
	Ext,
	Extb,
	Muls,
	Mulu,
	Neg,
	Negx,
	Sub,
	Suba,
	Subi,
	Subq,
	Subx,

	// Logical operation instructions
	And,
	Andi,
	Eor,
	Eori,
	Not,
	Or,
	Ori,

	// Shifting and rotation instructions
	Asl,
	Asr,
	Lsl,
	Lsr,
	Rol,
	Ror,
	Roxl,
	Roxr,
	Swap,

	// Bit manipulation instructioons
	Bchg,
	Bclr,
	Bset,
	Btst,

	// Bitfield instructions
	Bfchg,
	Bfclr,
	Bfexts,
	Bfextu,
	Bfffo,
	Bfins,
	Bfset,
	Bftst,

	// BCD instructions
	Abcd,
	Nbcd,
	Pack,
	Sbcd,
	Unpk,

	// Flow control instructions
	Callm,
	Bcc,
	Fbcc,
	Dbcc,
	Fdbcc,
	Scc,
	Fscc,
	Bra,
	Bsr,
	Jmp,
	Jsr,
	Nop,
	Fnop,
	Rtd,
	Rtm,
	Rtr,
	Rts,
	Tst,
	Ftst,

	// System control instructioons
	Frestore,
	Fsave,
	Moveusp,
	Movec,
	Moves,
	Reset,
	Rte,
	Stop,
	Bkpt,
	Chk,
	Illegal,
	Trap,
	Trapcc,
	Ftrapcc,
	Trapv,

	// Cache control instructions
	Cinvl,
	Cinvp,
	Cinva,
	Cpushl,
	Cpushp,
	Cpusha,

	// Multiprocessor instructions
	Cas,
	Cas2,
	Tas,
	Cpbcc,
	Cpdbcc,
	Cpgen,
	Cprestore,
	Cpsave,
	Cpscc,
	Cptrapcc,

	// MMU instructions
	Pbcc,
	Pdbcc,
	Pflusha,
	Pflush,
	Pflushn,
	Pflushan,
	Pflushs,
	Pflushr,
	Pload,
	Pmove,
	Prestore,
	Psave,
	Pscc,
	Ptest,
	Ptrapcc,

	// FPU operation instructions
	Fadd,
	Fsadd,
	Fdadd,
	Fcmp,
	Fdiv,
	Fsdiv,
	Fddiv,
	Fmod,
	Fmul,
	Fsmul,
	Fdmul,
	Frem,
	Fscale,
	Fsub,
	Fssub,
	Fdsub,
	Fsgldiv,
	Fsglmul,
	Fabs,
	Facos,
	Fasin,
	Fatan,
	Fcos,
	Fcosh,
	Fetox,
	Fetoxm1,
	Fgetexp,
	Fgetman,
	Fint,
	Fintrz,
	Flogn,
	Flognp1,
	Flog10,
	Flog2,
	Fneg,
	Fsin,
	Fsinh,
	Fsqrt,
	Ftan,
	Ftanh,
	Ftentox,
	Ftwotox,

	// Instructions determined by a bit in their trailing bytes
	DivslDivul,
	Chk2Cmp2,
	MulsMulu,
	Privileged,
	TblsTblu,
	Fpu,
}

const INSN_MASK_EA: u16 = 0xf1c0;
const INSN_MASK_EA_NO_REG: u16 = 0xffc0;
const INSN_MASK_VECTOR_REG: u16 = 0xfff8;
const REG_X_SHIFT: usize = 9;
const REG_MASK: u16 = 0x0007;
const SIZE_MASK: u16 = 0x00c0;
const SIZE_SHIFT: usize = 6;
const CONDITION_MASK: u16 = 0x0f00;
const CONDITION_SHIFT: usize = 8;
const EA_MODE_MASK: u16 = 0x0038;
const EA_MODE_SHIFT: usize = 3;
const VECTOR_MASK: u16 = 0x0007;

impl DecodedOperation
{
	fn new_unsized(operation: Instruction, rx: u8, ry: u8, mode: u8) -> Self
	{
		Self {
			operation,
			rx,
			ry,
			flags: OperationFlags::none(),
			operation_size: 0,
			op_mode: 0,
			mode,
			trailing_bytes: 0,
		}
	}

	fn new_immediate(operation: Instruction, ry: u8, operation_size: u8, mode: u8) -> Self
	{
		Self {
			operation,
			rx: 0,
			ry,
			flags: OperationFlags::none(),
			operation_size,
			op_mode: 0,
			mode,
			trailing_bytes: 0,
		}
	}

	fn new_immediate_trailing(operation: Instruction, ry: u8, mode: u8, trailing_bytes: u8) -> Self
	{
		Self {
			operation,
			rx: 0,
			ry,
			flags: OperationFlags::ImmediateNotRegister,
			operation_size: 0,
			op_mode: 0,
			mode,
			trailing_bytes,
		}
	}

	fn new_unsized_with_trailing(operation: Instruction, ry: u8, mode: u8, trailing_bytes: u8) -> Self
	{
		Self {
			operation,
			rx: 0,
			ry,
			flags: OperationFlags::none(),
			operation_size: 0,
			op_mode: 0,
			mode,
			trailing_bytes,
		}
	}

	fn new_simple(operation: Instruction, rx: u8, ry: u8) -> Self
	{
		Self {
			operation,
			rx,
			ry,
			flags: OperationFlags::none(),
			operation_size: 0,
			op_mode: 0,
			mode: 0,
			trailing_bytes: 0,
		}
	}

	fn new_simple_sized(operation: Instruction, rx: u8, ry: u8, operation_size: u8) -> Self
	{
		Self {
			operation,
			rx,
			ry,
			flags: OperationFlags::none(),
			operation_size,
			op_mode: 0,
			mode: 0,
			trailing_bytes: 0,
		}
	}

	fn new_simple_with_trailing(operation: Instruction, rx: u8, ry: u8, trailing_bytes: u8) -> Self
	{
		Self {
			operation,
			rx,
			ry,
			flags: OperationFlags::none(),
			operation_size: 0,
			op_mode: 0,
			mode: 0,
			trailing_bytes,
		}
	}

	fn new_simple_with_op_mode(operation: Instruction, rx: u8, ry: u8, op_mode: u8) -> Self
	{
		Self {
			operation,
			rx,
			ry,
			flags: OperationFlags::none(),
			operation_size: 0,
			op_mode,
			mode: 0,
			trailing_bytes: 0,
		}
	}
}

impl Default for DecodedOperation
{
	fn default() -> Self
	{
		Self {
			operation: Instruction::Illegal,
			rx: 0,
			ry: 0,
			flags: OperationFlags::none(),
			operation_size: 0,
			op_mode: 0,
			mode: 0,
			trailing_bytes: 0,
		}
	}
}

impl From<u16> for DecodedOperation
{
	fn from(insn: u16) -> Self
	{
		// Extract out the effective addressing mode
		let ea_mode = ((insn & EA_MODE_MASK) >> EA_MODE_SHIFT) as u8;
		let ea_reg = (insn & REG_MASK) as u8;

		// Decode instructions that use the effective address form
		match insn & INSN_MASK_EA {
			0xd000 | 0xd040 | 0xd080 | 0xd100 | 0xd140 | 0xd180 if
				// If the effective address target is used only as a source operand
				if (insn & 0x0100) == 0 {
					// ADD is allowed all valid modes
					!(ea_mode == 7 && ea_reg > 4)
				}
				// Otherwise if it's a target, the requirements are much stricter
				else {
					// ADD is not allowed with direct register usage
					ea_mode != 0 && ea_mode != 1 &&
					// ADD is not allowed with `#<data>` mode or PC-rel data register
					// usage, only u16 and u32 indirect mode 7
					!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1))
				} => {
					return DecodedOperation {
						operation: Instruction::Add,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						operation_size: ((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						// Extract the operation direction information
						op_mode: ((insn & 0x0100) >> 8) as u8,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0xd0c0 | 0xd1c0 if
				// ADDA is allowed all valid modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation {
						operation: Instruction::Adda,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						// Decode whether 16- or 32-bit operation
						operation_size: if (insn & 0x01c0) == 0x00c0 {
							2
						} else {
							4
						},
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0x5000 | 0x5040 | 0x5080 if
				// ADDQ is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation {
						operation: Instruction::Addq,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::ImmediateNotRegister,
						operation_size: ((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0xc000 | 0xc040 | 0xc080 | 0xc100 | 0xc140 | 0xc180 if
				// If the effective address target is used only as a source operand
				if (insn & 0x0100) == 0 {
					// AND is not allowed with address registers
					ea_mode != 1 &&
					// AND is allowed all valid mode 7 modes
					!(ea_mode == 7 && ea_reg > 4)
				}
				// Otherwise if it's a target, the requirements are much stricter
				else {
					// AND is not allowed with direct register usage
					ea_mode != 0 && ea_mode != 1 &&
					// AND is not allowed with `#<data>` mode or PC-rel data register
					// usage, only u16 and u32 indirect mode 7
					!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1))
				} => {
					return DecodedOperation {
						operation: Instruction::And,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						operation_size: ((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						// Extract the operation direction information
						op_mode: ((insn & 0x0100) >> 8) as u8,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0x0140 if
				// BCHG is not allowed with address registers
				ea_mode != 1 &&
				// BCHG uses only the u16 and u32 indirect modes for mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized(
						Instruction::Bchg,
						((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ea_reg,
						ea_mode,
					);
				},
			0x0180 if
				// BCLR is not allowed with address registers
				ea_mode != 1 &&
				// BCLR uses only the u16 and u32 indirect modes for mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized(
						Instruction::Bclr,
						((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ea_reg,
						ea_mode,
					);
				},
			0x01c0 if
				// BSET is not allowed with address registers
				ea_mode != 1 &&
				// BSET uses only the u16 and u32 indirect modes for mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized(
						Instruction::Bset,
						((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ea_reg,
						ea_mode,
					);
				},
			0x0100 if
				// BTST is not allowed with address registers
				ea_mode != 1 &&
				// BTST doesn't use a couple of the mode 7 modes
				!(ea_mode == 7 && (ea_reg == 5 || ea_reg == 6 || ea_reg == 7)) => {
					return DecodedOperation::new_unsized(
						Instruction::Btst,
						((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ea_reg,
						ea_mode,
					);
				},
			0x4100 | 0x4180 if
				// CHK is not allowed with address registers
				ea_mode != 1 &&
				// CHK is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation {
						operation: Instruction::Chk,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						// Decode whether 16- or 32-bit operation
						operation_size: if (insn & 0x0080) != 0 {
							2
						} else {
							4
						},
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0xb000 | 0xb040 | 0xb080 if
				// CMP is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation {
						operation: Instruction::Cmp,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						operation_size: ((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				}
			0xb0c0 | 0xb1c0  if
				// CMPA is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation {
						operation: Instruction::Cmpa,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						// Decode whether 16- or 32-bit operation
						operation_size: if (insn & 0x01c0) == 0x00c0 {
							2
						} else {
							4
						},
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0x81c0 if
				// DIVS is not allowed with address registers
				ea_mode != 1 &&
				// DIVS is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation::new_unsized(
						Instruction::Divs,
						((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ea_reg,
						ea_mode,
					);
				},
			0x80c0 if
				// DIVU is not allowed with address registers
				ea_mode != 1 &&
				// DIVU is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation::new_unsized(
						Instruction::Divu,
						((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ea_reg,
						ea_mode,
					);
				},
			0xb100 | 0xb140 | 0xb180 if
				// EOR is not allowed with address registers
				ea_mode != 1 &&
				// EOR is not allowed with `#<data>` mode or PC-rel data register usage,
				// only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1))
				=> {
					return DecodedOperation {
						operation: Instruction::Eor,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						operation_size: ((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0x41c0 if
				// LEA is not allowed with direct register usage, or with register modification
				ea_mode != 0 && ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// LEA is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized(
						Instruction::Lea,
						((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ea_reg,
						ea_mode,
					);
				}
			0x2040 | 0x3040 if
				// MOVEA is allowed all valid modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation {
						operation: Instruction::Movea,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						// Extract whether this is a u16 or u32 operation
						operation_size: if (insn & 0x1000) != 0 {
							2
						} else {
							4
						},
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0xc1c0 if
				// MULS is not allowed with address registers
				ea_mode != 1 &&
				// MULS is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation::new_unsized(
						Instruction::Muls,
						((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ea_reg,
						ea_mode,
					);
				},
			0xc0c0 if
				// MULU is not allowed with address registers
				ea_mode != 1 &&
				// MULU is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation::new_unsized(
						Instruction::Mulu,
						((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ea_reg,
						ea_mode,
					);
				},
			0x8000 | 0x8040 | 0x8080 | 0x8100 | 0x8140 | 0x8180 if
				// If the effective address target is used only as a source operand
				if (insn & 0x0100) == 0 {
					// OR is not allowed with address registers
					ea_mode != 1 &&
					// OR is allowed all valid modes
					!(ea_mode == 7 && ea_reg > 4)
				}
				// Otherwise if it's a target, the requirements are much stricter
				else {
					// OR is not allowed with direct register usage
					ea_mode != 0 && ea_mode != 1 &&
					// OR is not allowed with `#<data>` mode or PC-rel data register
					// usage, only u16 and u32 indirect mode 7
					!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1))
				} => {
					return DecodedOperation {
						operation: Instruction::Or,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						operation_size: ((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						// Extract the operation direction information
						op_mode: ((insn & 0x0100) >> 8) as u8,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0x50c0 | 0x51c0 if
				// Scc is not allowed with address registers
				ea_mode != 1 &&
				// Scc is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation {
						operation: Instruction::Scc,
						rx: 0,
						ry: ea_reg,
						flags: OperationFlags::none(),
						operation_size: 0,
						op_mode: ((insn & CONDITION_MASK) >> CONDITION_SHIFT) as u8,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0x9000 | 0x9040 | 0x9080 | 0x9100 | 0x9140 | 0x9180 if
				// If the effective address target is used only as a source operand
				if (insn & 0x0100) == 0 {
					// SUB is allowed all valid modes
					!(ea_mode == 7 && ea_reg > 4)
				}
				// Otherwise if it's a target, the requirements are much stricter
				else {
					// SUB is not allowed with direct register usage
					ea_mode != 0 && ea_mode != 1 &&
					// SUB is not allowed with `#<data>` mode or PC-rel data register
					// usage, only u16 and u32 indirect mode 7
					!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1))
				} => {
					return DecodedOperation {
						operation: Instruction::Sub,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						operation_size: ((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						// Extract the operation direction information
						op_mode: ((insn & 0x0100) >> 8) as u8,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0x90c0 | 0x91c0 if
				// SUBA is allowed all valid modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation {
						operation: Instruction::Suba,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::none(),
						// Decode whether 16- or 32-bit operation
						operation_size: if (insn & 0x01c0) == 0x00c0 {
							2
						} else {
							4
						},
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			0x5100 | 0x5140 | 0x5180 if
				// SUBQ is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation {
						operation: Instruction::Subq,
						rx: ((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ry: ea_reg,
						flags: OperationFlags::ImmediateNotRegister,
						operation_size: ((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0,
					};
				},
			_ => {},
		};

		// Decode instructions that use the effective address form without an Rx register
		match insn & INSN_MASK_EA_NO_REG {
			0x0600 | 0x0640 | 0x0680 if
				// ADDI is not allowed with address registers
				ea_mode != 1 &&
				// ADDI is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Addi,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0x0200 | 0x0240 | 0x0280 if
				// ANDI is not allowed with address registers
				ea_mode != 1 &&
				// ANDI is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Andi,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0xe0c0 if
				// ASR is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// ASR is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation {
						operation: Instruction::Asr,
						rx: 0,
						ry: ea_reg,
						flags: OperationFlags::MemoryNotRegister,
						operation_size: 1,
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0
					};
				},
			0xe1c0 if
				// ASL is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// ASL is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation {
						operation: Instruction::Asl,
						rx: 0,
						ry: ea_reg,
						flags: OperationFlags::MemoryNotRegister,
						operation_size: 1,
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0
					};
				},
			0x0840 if
				// BCHG is not allowed with address registers
				ea_mode != 1 &&
				// BCHG is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate_trailing(
						Instruction::Bchg,
						ea_reg,
						ea_mode,
						2, // 16-bit bit number follows (8 bits used)
					);
				},
			0x0880 if
				// BCLR is not allowed with address registers
				ea_mode != 1 &&
				// BCLR is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate_trailing(
						Instruction::Bclr,
						ea_reg,
						ea_mode,
						2, // 16-bit bit number follows (8 bits used)
					);
				},
			0xeac0 if
				// BFCHG is not allowed with address registers, or with register modification
				ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// BFCHG is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate_trailing(
						Instruction::Bfchg,
						ea_reg,
						ea_mode,
						2, // 16-bit {offset:width} follows
					);
				},
			0xecc0 if
				// BFCLR is not allowed with address registers, or with register modification
				ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// BFCLR is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate_trailing(
						Instruction::Bfclr,
						ea_reg,
						ea_mode,
						2, // 16-bit {offset:width} follows
					);
				},
			0xebc0 if
				// BFEXTS is not allowed with address registers, or with register modification
				ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// BFEXTS is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Bfexts,
						ea_reg,
						ea_mode,
						2, // 16-bit {offset:width}, Dn follows
					);
				},
			0xe9c0 if
				// BFEXTU is not allowed with address registers, or with register modification
				ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// BFEXTU is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Bfextu,
						ea_reg,
						ea_mode,
						2, // 16-bit {offset:width}, Dn follows
					);
				},
			0xedc0 if
				// BFFFO is not allowed with address registers, or with register modification
				ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// BFFFO is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Bfffo,
						ea_reg,
						ea_mode,
						2, // 16-bit {offset:width}, Dn follows
					);
				},
			0xefc0 if
				// BFINS is not allowed with address registers, or with register modification
				ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// BFINS is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Bfins,
						ea_reg,
						ea_mode,
						2, // 16-bit {offset:width}, Dn follows
					);
				},
			0xeec0 if
				// BFSET is not allowed with address registers, or with register modification
				ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// BFSET is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Bfset,
						ea_reg,
						ea_mode,
						2, // 16-bit {offset:width}, Dn follows
					);
				},
			0xe8c0 if
				// BFTST is not allowed with address registers, or with register modification
				ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// BFTST is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Bftst,
						ea_reg,
						ea_mode,
						2, // 16-bit {offset:width}, Dn follows
					);
				},
			0x08c0 if
				// BSET is not allowed with address registers
				ea_mode != 1 &&
				// BSET is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate_trailing(
						Instruction::Bset,
						ea_reg,
						ea_mode,
						2, // 16-bit number follows (8 bits used)
					);
				},
			0x0800 if
				// BTST is not allowed with address registers
				ea_mode != 1 &&
				// BTST is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_immediate_trailing(
						Instruction::Btst,
						ea_reg,
						ea_mode,
						2, // 16-bit number follows (8 bits used)
					);
				},
			0x06c0 if
				// CALLM is not allowed with direct register usage, or with register modification
				ea_mode != 0 && ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// CALLM is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_immediate_trailing(
						Instruction::Callm,
						ea_reg,
						ea_mode,
						2, // 16-bit argument count follows (8 bits used)
					);
				},
			0x0ac0 | 0x0cc0 | 0x0ec0 if
				// CAS is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// CAS is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation {
						operation: Instruction::Cas,
						rx: 0,
						ry: ea_reg,
						flags: OperationFlags::ImmediateNotRegister,
						operation_size: ((insn & 0x0600) >> 9) as u8,
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 2, // 16-bit Dc, Du follows
					};
				},
			0x00c0 | 0x02c0 | 0x04c0 if
				// Only non-altering indirection modes are allowed for CHK2/CMP2
				ea_mode != 0 && ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// CHK2/CMP2 is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation {
						operation: Instruction::Chk2Cmp2,
						rx: 0,
						ry: ea_reg,
						flags: OperationFlags::ImmediateNotRegister,
						operation_size: ((insn & 0x0600) >> 9) as u8,
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 2, // 16-bit Rn follows, bit 11 determines if chk2 (1) or cmp2 (0)
					};
				},
			0x4200 | 0x4240 | 0x4280 if
				// CLR is not allowed with address registers
				ea_mode != 1 &&
				// CLR is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Clr,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0x0c00 | 0x0c40 | 0x0c80 if
				// CMPI is not allowed with address registers
				ea_mode != 1 &&
				// CMPI is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Cmpi,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0x4c40 if
				// DIVSL/DIVUL is not allowed with address registers
				ea_mode != 1 &&
				// DIVSL/DIVUL is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::DivslDivul,
						ea_reg,
						ea_mode,
						2, // 16-bit Dr:Dq follows, bit 11 determines if DIVSL (1) or DIVUL (0)
					);
				}
			0x0a00 | 0x0a40 | 0x0a80 if
				// EORI is not allowed with address registers
				ea_mode != 1 &&
				// EORI is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Eori,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0xf280 | 0xf2c0 if
				// Only condition codes up to (and including) 0x1f are valid
				(ea_mode & 0x04) != 0 => {
					return DecodedOperation {
						operation: Instruction::Fbcc,
						rx: 0,
						ry: 0,
						flags: OperationFlags::none(),
						operation_size: 0,
						op_mode: (ea_mode << 3) | ea_reg, // Rebuild the condition code
						mode: 0,
						trailing_bytes: if (insn & 0x0040) == 0 {
							2
						} else {
							4
						}, // 16- or 32-bit of displacement follows
					};
				},
			0xf200 if
				// F* instructions are not allowed with address registers
				ea_mode != 1 &&
				// F* instructions are allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Fpu,
						ea_reg,
						ea_mode,
						2, // 16-bit instruction continuation follows
					);
				},
			0xf340 if
				// FRESTORE is not allowed with direct register usage, or with pre-dec register modification
				ea_mode != 0 && ea_mode != 1 && ea_mode != 4 &&
				// FRESTORE is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Frestore,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0xf300 if
				// FSAVE is not allowed with direct register usage, or with pre-dec register modification
				ea_mode != 0 && ea_mode != 1 && ea_mode != 4 &&
				// FSAVE is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Fsave,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0xf240 if
				// FScc instruction is not allowed with address registers
				ea_mode != 1 &&
				// FScc is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Fscc,
						ea_reg,
						ea_mode,
						2, // 16-bits follow (5 bits used) for conditional predicate
					);
				},
			0x4ec0 if
				// Only non-altering indirection modes are allowed for JMP
				ea_mode != 0 && ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// JMP is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Jmp,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0x4e80 if
				// Only non-altering indirection modes are allowed for JSR
				ea_mode != 0 && ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// JSR is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Jsr,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0xe2c0 if
				// LSR is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// LSR is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation {
						operation: Instruction::Lsr,
						rx: 0,
						ry: ea_reg,
						flags: OperationFlags::MemoryNotRegister,
						operation_size: 1,
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0
					};
				},
			0xe3c0 if
				// LSL is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// LSL is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation {
						operation: Instruction::Lsl,
						rx: 0,
						ry: ea_reg,
						flags: OperationFlags::MemoryNotRegister,
						operation_size: 1,
						op_mode: 0,
						mode: ea_mode,
						trailing_bytes: 0
					};
				},
			0x42c0 if
				// MOVE from CCR is not allowed with address registers
				ea_mode != 1 &&
				// MOVE from CCR is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized(
						Instruction::Move,
						8, // 8 is a special register number (not otherwise valid) indicating CCR.
						ea_reg,
						ea_mode,
					);
				},
			0x44c0 if
				// MOVE to CCR is not allowed with address registers
				ea_mode != 1 &&
				// MOVE to CCR is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation::new_unsized(
						Instruction::Move,
						ea_reg,
						8, // 8 is a special register number (not otherwise valid) indicating CCR.
						ea_mode,
					);
				},
			0x40c0 if
				// MOVE from SR is not allowed with address registers
				ea_mode != 1 &&
				// MOVE from SR is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized(
						Instruction::Move,
						9, // 9 is a special register number (not otherwise valid) indicating SR.
						ea_reg,
						ea_mode,
					);
				},
			0x46c0 if
				// MOVE to SR is not allowed with address registers
				ea_mode != 1 &&
				// MOVE to SR is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation::new_unsized(
						Instruction::Move,
						ea_reg,
						9, // 9 is a special register number (not otherwise valid) indicating SR.
						ea_mode,
					);
				},
			0x4880 | 0x48c0 | 0x4c80 | 0x4cc0 if
				// MOVEM is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// If this is a reg -> mem MOVEM
				if (insn & 0x0400) == 0 {
					// MOVEM is not allowed with postincrement mode
					ea_mode != 3 &&
					// MOVEM is not allowed with `#<data>` mode or PC-rel data register
					// usage, only u16 and u32 indirect mode 7
					!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1))
				}
				// Otherwise if this is a mem -> reg MOVEM
				else {
					// MOVEM is not allowed with predecrement mode
					ea_mode != 4 &&
					// MOVEM is not allowed with `#<data>` mode 7
					!(ea_mode == 7 && (ea_reg & 0x04) != 0)
				} => {
					return DecodedOperation {
						operation: Instruction::Movem,
						rx: 0,
						ry: ea_reg,
						flags: OperationFlags::none(),
						// Extract whether the registers should be moved as u16's or u32's
						operation_size: if (insn & 0x0040) == 0 {
							2
						} else {
							4
						},
						// Extract out the transfer direction
						op_mode: ((insn & 0x0400) >> 10) as u8,
						mode: ea_mode,
						trailing_bytes: 2, // 16-bit register list mask follows
					};
				},
			0x0e00 | 0x0e40 | 0x0e80 if
				// MOVES is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// MOVES is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Moves,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0x4c00 if
				// MULS/MULU is not allowed with address registers
				ea_mode != 1 &&
				// MULS/MULU is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::MulsMulu,
						ea_reg,
						ea_mode,
						2, // 16-bit Dh - Dl follows, bit 11 determines of MULS (1) or MULU (0)
					);
				},
			0x4800 if
				// NBCD is not allowed with address registers
				ea_mode != 1 &&
				// NBCD is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Nbcd,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0x4400 | 0x4440 | 0x4480 if
				// NEG is not allowed with address registers
				ea_mode != 1 &&
				// NEG is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Neg,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0x4000 | 0x4040 | 0x4080 if
				// NEGX is not allowed with address registers
				ea_mode != 1 &&
				// NEGX is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Negx,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0x4600 | 0x4640 | 0x4680 if
				// NOT is not allowed with address registers
				ea_mode != 1 &&
				// NOT is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Not,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0x0000 | 0x0040 | 0x0080 if
				// ORI is not allowed with address registers
				ea_mode != 1 &&
				// ORI is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Not,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0x4840 if
				// PEA is not allowed with direct register usage, or with register modification
				ea_mode != 0 && ea_mode != 1 && ea_mode != 3 && ea_mode != 4 &&
				// PEA is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Pea,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0xf080 if
				// Only condition codes up to (and including) 0x0f are valid
				ea_mode == 0 || ea_mode == 1 => {
					return DecodedOperation {
						operation: Instruction::Pbcc,
						rx: 0,
						ry: 0,
						flags: OperationFlags::none(),
						operation_size: 0,
						op_mode: (ea_mode << 3) | ea_reg, // Rebuild the condition code
						mode: 0,
						trailing_bytes: if (insn & 0x0040) == 0 {
							2
						} else {
							4
						}, // 16- or 32-bit of displacement follows
					};
				},
			0xf140 if
				// PRESTORE is not allowed with direct register usage, or with pre-dec register modification
				ea_mode != 0 && ea_mode != 1 && ea_mode != 4 &&
				// PRESTORE is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Prestore,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0xf000 | 0xf040 if
				// P* instruction is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) &&
				// Filter out PDBcc
				!((insn & 0x00c0) == 0x0040 && ea_mode == 1) &&
				// filter out PTRAPcc
				!((insn & 0x00c0) == 0x0040 && ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation {
						operation: Instruction::Privileged,
						rx: 0,
						ry: ea_reg,
						flags: OperationFlags::none(),
						operation_size: 0,
						// Extract which block of privileged instructions this one belongs to
						op_mode: ((insn & 0x00c0) >> 6) as u8,
						mode: ea_mode,
						trailing_bytes: 2, // 16-bit instruction continuation follows
					};
				},
			0xf100 if
				// PSAVE is not allowed with direct register usage, or with pre-dec register modification
				ea_mode != 0 && ea_mode != 1 && ea_mode != 4 &&
				// PSAVE is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Psave,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0xe6c0 if
				// ROR is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// ROR is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Ror,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0xe7c0 if
				// ROL is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// ROL is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Rol,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0xe4c0 if
				// ROXR is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// ROXR is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Roxr,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0xe5c0 if
				// ROXL is not allowed with direct register usage
				ea_mode != 0 && ea_mode != 1 &&
				// ROXL is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Roxl,
						ea_reg,
						ea_mode,
						0,
					);
				},

			0x0400 | 0x0440 | 0x0480 if
				// SUBI is not allowed with address registers
				ea_mode != 1 &&
				// SUBI is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_immediate(
						Instruction::Subi,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			0x4ac0 if
				// TAS is not allowed with address registers
				ea_mode != 1 &&
				// TAS is not allowed with `#<data>` mode or PC-rel data register
				// usage, only u16 and u32 indirect mode 7
				!(ea_mode == 7 && !(ea_reg == 0 || ea_reg == 1)) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::Tas,
						ea_reg,
						ea_mode,
						0,
					);
				},
			0xf800 if
				// TBLU/TBLS is not allowed with address registers, or with post-inc register modification
				ea_mode != 1 && ea_mode != 3 &&
				// TBLU/TBLS is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized_with_trailing(
						Instruction::TblsTblu,
						ea_reg,
						ea_mode,
						2, // 16-bit instruction continuation follows, containing Dx, Dyn, size.
						// Bit 8 determines DReg (0) vs TLB (1), bit 11 determines TLBS (1) vs TBLU (0)
					);
				},
			0x4a00 | 0x4a40 | 0x4a80 if
				// TST is allowed all valid mode 7 modes
				!(ea_mode == 7 && ea_reg > 4) => {
					return DecodedOperation::new_immediate(
						Instruction::Tst,
						ea_reg,
						((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
						ea_mode,
					);
				},
			_ => {},
		};

		// Decode instructions that specify only a vector or register field
		match insn & INSN_MASK_VECTOR_REG {
			0x4848 => return DecodedOperation::new_simple(Instruction::Bkpt, (insn & VECTOR_MASK) as u8, 0),
			0xf408 | 0xf448 | 0xf488 | 0xf4c8 => {
				return DecodedOperation::new_simple(
					Instruction::Cinvl,
					(insn & REG_MASK) as u8,
					((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
				);
			},
			0xf410 | 0xf450 | 0xf490 | 0xf4d0 => {
				return DecodedOperation::new_simple(
					Instruction::Cinvp,
					(insn & REG_MASK) as u8,
					((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
				);
			},
			0xf418 | 0xf458 | 0xf498 | 0xf4d8 => {
				return DecodedOperation::new_simple(
					Instruction::Cinva,
					(insn & REG_MASK) as u8,
					((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
				);
			},
			0xf428 | 0xf468 | 0xf4a8 | 0xf4e8 => {
				return DecodedOperation::new_simple(
					Instruction::Cpushl,
					(insn & REG_MASK) as u8,
					((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
				);
			},
			0xf430 | 0xf470 | 0xf4b0 | 0xf4f0 => {
				return DecodedOperation::new_simple(
					Instruction::Cpushp,
					(insn & REG_MASK) as u8,
					((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
				);
			},
			0xf438 | 0xf478 | 0xf4b8 | 0xf4f8 => {
				return DecodedOperation::new_simple(
					Instruction::Cpusha,
					(insn & REG_MASK) as u8,
					((insn & SIZE_MASK) >> SIZE_SHIFT) as u8,
				);
			},
			0x4880 | 0x48c0 => {
				return DecodedOperation::new_simple_sized(
					Instruction::Ext,
					(insn & REG_MASK) as u8,
					0,
					// Decode whether this is a i8 or i16 we're sign extending (to i16 and i32 respectively)
					if (insn & 0x0040) == 0 {
						1
					} else {
						2
					},
				);
			},
			0x49c0 => return DecodedOperation::new_simple(Instruction::Extb, (insn & REG_MASK) as u8, 0),
			0xf248 => {
				return DecodedOperation::new_simple_with_trailing(
					Instruction::Fdbcc,
					0,
					(insn & REG_MASK) as u8,
					4, // 16-bit instruction continuation + 16-bit displacement follows
				);
			},
			0xf278 => {
				let op_mode = (insn & REG_MASK) as u8;
				if op_mode == 2 || op_mode == 3 || op_mode == 4 {
					return DecodedOperation {
						operation: Instruction::Ftrapcc,
						rx: 0,
						ry: 0,
						flags: OperationFlags::none(),
						operation_size: 0,
						op_mode,
						mode: 0,
						trailing_bytes: if op_mode == 2 {
							2 // Instruction followed by 1 operand u16
						} else if op_mode == 3 {
							4 // Instruction followed by 2 operand u16's
						} else {
							0 // Instruction followed by no operand u16's
						} + 2, // 16-bit conditionial predicate (5 bits used) follows
					};
				}
			},
			0x4e50 => {
				return DecodedOperation::new_simple_with_trailing(
					Instruction::Link,
					(insn & REG_MASK) as u8,
					0,
					2, // 16-bit displayment follows
				);
			},
			0x4808 => {
				return DecodedOperation::new_simple_with_trailing(
					Instruction::Link,
					(insn & REG_MASK) as u8,
					0,
					4, // 32-bit displayment follows
				);
			},
			0xf620 => {
				return DecodedOperation {
					operation: Instruction::Move16,
					rx: (insn & REG_MASK) as u8,
					ry: 0,
					flags: OperationFlags::Postincrement,
					operation_size: 0,
					op_mode: 0,
					mode: 0,
					trailing_bytes: 2, // 16-bit instruction continuation follows, containing Ay
				};
			},
			0xf600 | 0xf608 | 0xf610 | 0xf618 => {
				return DecodedOperation {
					operation: Instruction::Move16,
					rx: 0,
					ry: (insn & REG_MASK) as u8,
					flags: OperationFlags::none(),
					operation_size: 0,
					// Extract out the operation mode
					op_mode: ((insn & 0x0018) >> 3) as u8,
					mode: 0,
					trailing_bytes: 4, // 32-bit absolute address follows
				};
			},
			0x4e60 => {
				// MOVE to USP
				return DecodedOperation::new_simple(
					Instruction::Move,
					(insn & REG_MASK) as u8,
					10, // 10 is a special register number (not otherwise valid) indicating USP
				);
			},
			0x4e68 => {
				// MOVE from USP
				return DecodedOperation::new_simple(
					Instruction::Move,
					10, // 10 is a special register number (not otherwise valid) indicating USP
					(insn & REG_MASK) as u8,
				);
			},
			0xf048 => {
				return DecodedOperation::new_simple_with_trailing(
					Instruction::Pdbcc,
					0,
					(insn & REG_MASK) as u8,
					4, // 16-bit instruction continuation + 16-bit displacement follows
				);
			},
			0xf500 => return DecodedOperation::new_simple(Instruction::Pflushn, (insn & REG_MASK) as u8, 0),
			0xf508 => return DecodedOperation::new_simple(Instruction::Pflush, (insn & REG_MASK) as u8, 0),
			0xf510 => return DecodedOperation::new_simple(Instruction::Pflushan, (insn & REG_MASK) as u8, 0),
			0xf518 => return DecodedOperation::new_simple(Instruction::Pflusha, (insn & REG_MASK) as u8, 0),
			0xf548 | 0xf568 => {
				return DecodedOperation::new_simple_with_op_mode(
					Instruction::Ptest,
					(insn & REG_MASK) as u8,
					0,
					((insn & 0x0020) >> 5) as u8, // Extract read vs write
				);
			},
			0xf078 => {
				let op_mode = (insn & REG_MASK) as u8;
				if op_mode == 2 || op_mode == 3 || op_mode == 4 {
					return DecodedOperation {
						operation: Instruction::Ptrapcc,
						rx: 0,
						ry: 0,
						flags: OperationFlags::none(),
						operation_size: 0,
						op_mode,
						mode: 0,
						trailing_bytes: if op_mode == 2 {
							2 // Instruction followed by 1 operand u16
						} else if op_mode == 3 {
							4 // Instruction followed by 2 operand u16's
						} else {
							0 // Instruction followed by no operand u16's
						} + 2, // 16-bit coprocessor condition follows
					};
				}
			},
			0x06c0 | 0x06c8 => {
				return DecodedOperation::new_simple_with_op_mode(
					Instruction::Rtm,
					0,
					(insn & REG_MASK) as u8,
					// Extract the data-or-address bit for what flavour the register in ry is
					((insn & 0x0008) >> 3) as u8,
				);
			},
			0x4840 => return DecodedOperation::new_simple(Instruction::Swap, (insn & REG_MASK) as u8, 0),
			_ => {},
		};

		DecodedOperation::default()
	}
}
