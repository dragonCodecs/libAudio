// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_CPU_M68K_HXX
#define EMULATOR_CPU_M68K_HXX

#include <cstdint>
#include <array>
#include <substrate/flags>
#include "../memoryMap.hxx"
#include "m68kInstruction.hxx"

enum class m68kStatusBits_t
{
	// User byte bit meanings
	carry = 0U,
	overflow = 1U,
	zero = 2U,
	negative = 3U,
	extend = 4U,

	// System byte bit meanings
	// 8-10 are interrupt bits
	supervisor = 13U,
	trace = 15U,
};

struct fpuReg_t
{
	uint64_t mantisa;
	uint16_t exponent;
};

struct stepResult_t
{
	bool validInsn;
	bool trap;
	size_t cyclesTaken;
};

struct motorola68000_t final
{
private:
	memoryMap_t<uint32_t> &_peripherals;
	uint32_t clockFrequency;

	// There are 8 data registers, 7 address registers, 3 stack pointers,
	// a program counter and a status register in a m68k
	std::array<uint32_t, 8U> d;
	std::array<uint32_t, 7U> a;
	uint32_t systemStackPointer;
	uint32_t interruptStackPointer;
	uint32_t userStackPointer;
	uint32_t programCounter;
	// By default the system starts up in supervisor (system) mode
	substrate::bitFlags_t<uint16_t, m68kStatusBits_t> status{m68kStatusBits_t::supervisor};

	// There are then 8 FPU registers, a control register, a status register, and an instruction address register
	std::array<fpuReg_t, 8U> fp;
	// This needs to be a bitFlags_t. See M68000PRM pg16, §1.2.2.2
	uint16_t fpControl;
	// Bits 24-27 are a bitFlags_t too - see M68000PRM pg17, §1.2.3.1
	// Likewise, 8-15 - see M68000PRM pg17, §1.2.3.3; and 3-7 - see M68000PRM pg18, §1.2.3.4
	uint32_t fpStatus;
	uint32_t fpInstructionAddress;

	// Instruction dispatch/execution functions
	[[nodiscard]] stepResult_t dispatchBRA(const decodedOperation_t &insn) noexcept;

public:
	motorola68000_t(memoryMap_t<uint32_t> &peripherals, uint32_t clockFreq) noexcept;

	void executeFrom(uint32_t entryAddress, uint32_t stackTop, bool asUser = true) noexcept;
	void writeDataRegister(size_t reg, uint32_t value) noexcept;
	void writeAddrRegister(size_t reg, uint32_t value) noexcept;
	uint32_t readProgramCounter() const noexcept;
	[[nodiscard]] stepResult_t step() noexcept;

	// Not actually part of the public interface, just necessary to be exposed for testing
	decodedOperation_t decodeInstruction(uint16_t insn) const noexcept;
};

#endif /*EMULATOR_CPU_M68K_HXX*/
