// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_CPU_M68K_INSTRUCTION_HXX
#define EMULATOR_CPU_M68K_INSTRUCTION_HXX

#include <cstdint>
#include <substrate/flags>

enum class instruction_t
{
	// Data movement instructions
	exg,
	fmove,
	fsmove,
	fdmove,
	fmovem,
	lea,
	link,
	move,
	move16,
	movea,
	movem,
	movep,
	moveq,
	pea,
	unlk,

	// Integer arithmetic instructions
	add,
	adda,
	addi,
	addq,
	addx,
	clr,
	cmp,
	cmpa,
	cmpi,
	cmpm,
	divs,
	divu,
	ext,
	extb,
	muls,
	mulu,
	neg,
	negx,
	sub,
	suba,
	subi,
	subq,
	subx,

	// Logical operation instructions
	_and,
	andi,
	eor,
	eori,
	_not,
	_or,
	ori,

	// Shifting and rotation instructions
	asl,
	asr,
	lsl,
	lsr,
	rol,
	ror,
	roxl,
	roxr,
	swap,

	// Bit manipulation instructioons
	bchg,
	bclr,
	bset,
	btst,

	// Bitfield instructions
	bfchg,
	bfclr,
	bfexts,
	bfextu,
	bfffo,
	bfins,
	bfset,
	bftst,

	// BCD instructions
	abcd,
	nbcd,
	pack,
	sbcd,
	unpk,

	// Flow control instructions
	callm,
	bcc,
	fbcc,
	dbcc,
	fdbcc,
	scc,
	fscc,
	bra,
	bsr,
	jmp,
	jsr,
	nop,
	fnop,
	rtd,
	rtm,
	rtr,
	rts,
	tst,
	ftst,

	// System control instructioons
	frestore,
	fsave,
	moveusp,
	movec,
	moves,
	reset,
	rte,
	stop,
	bkpt,
	chk,
	illegal,
	trap,
	trapcc,
	ftrapcc,
	trapv,

	// Cache control instructions
	cinvl,
	cinvp,
	cinva,
	cpushl,
	cpushp,
	cpusha,

	// Multiprocessor instructions
	cas,
	cas2,
	tas,
	cpbcc,
	cpdbcc,
	cpgen,
	cprestore,
	cpsave,
	cpscc,
	cptrapcc,

	// MMU instructions
	pbcc,
	pdbcc,
	pflusha,
	pflush,
	pflushn,
	pflushan,
	pflushs,
	pflushr,
	pload,
	pmove,
	prestore,
	psave,
	pscc,
	ptest,
	ptrapcc,

	// FPU operation instructions
	fadd,
	fsadd,
	fdadd,
	fcmp,
	fdiv,
	fsdiv,
	fddiv,
	fmod,
	fmul,
	fsmul,
	fdmul,
	frem,
	fscale,
	fsub,
	fssub,
	fdsub,
	fsgldiv,
	fsglmul,
	fabs,
	facos,
	fasin,
	fatan,
	fcos,
	fcosh,
	fetox,
	fetoxm1,
	fgetexp,
	fgetman,
	fint,
	fintrz,
	flogn,
	flognp1,
	flog10,
	flog2,
	fneg,
	fsin,
	fsinh,
	fsqrt,
	ftan,
	ftanh,
	ftentox,
	ftwotox,

	// Instructions determined by a bit in their trailing bytes
	divsl_divul,
	chk2_cmp2,
	muls_mulu,
	privileged,
	tbls_tblu,
};

enum class operationFlags_t
{
	memoryNotRegister, // Operation is on memory, not a register
	immediateNotRegister, // Operation uses rx as immediate not register
	postincrement, // Operation is post-increment mode
};

struct decodedOperation_t
{
	instruction_t operation;
	uint8_t rx{0U};
	uint8_t ry{0U};
	substrate::bitFlags_t<uint8_t, operationFlags_t> flags{};
	uint8_t operationSize{0U};
	uint8_t opMode{0U};
	uint8_t mode{0U};
	uint8_t trailingBytes{0U};
};

constexpr inline bool operator ==(const decodedOperation_t a, const decodedOperation_t b) noexcept
{
	return
		a.operation == b.operation &&
		a.rx == b.rx &&
		a.ry == b.ry &&
		a.flags == b.flags &&
		a.operationSize == b.operationSize &&
		a.opMode == b.opMode &&
		a.mode == b.mode &&
		a.trailingBytes == b.trailingBytes;
}

#endif /*EMULATOR_CPU_M68K_INSTRUCTION_HXX*/
