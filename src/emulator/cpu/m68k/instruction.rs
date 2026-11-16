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
const REG_X_SHIFT: usize = 9;
const REG_MASK: u16 = 0x0007;
const SIZE_MASK: u16 = 0x00c0;
const SIZE_SHIFT: usize = 6;
const EA_MODE_MASK: u16 = 0x0038;
const EA_MODE_SHIFT: usize = 3;

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
						ea_mode
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
						ea_mode
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
						ea_mode
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
						ea_mode
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
						ea_mode
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
						ea_mode
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
				!(ea_mode == 0 || ea_mode == 1 || ea_mode == 3 || ea_mode == 4) &&
				// LEA is not allowed with `#<data>` mode 7
				!(ea_mode == 7 && (ea_reg & 0x04) != 0) => {
					return DecodedOperation::new_unsized(
						Instruction::Lea,
						((insn >> REG_X_SHIFT) & REG_MASK) as u8,
						ea_reg,
						ea_mode
					);
				}
			_ => {},
		};

		DecodedOperation::default()
	}
}
