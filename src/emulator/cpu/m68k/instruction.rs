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
