// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_CPU_M68K_HXX
#define EMULATOR_CPU_M68K_HXX

#include <cstdint>
#include <array>
#include <type_traits>
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
	memoryMap_t<uint32_t, 0x00ffffffU> &_peripherals;
	uint32_t clockFrequency;
	uint32_t waitCycles{0U};

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

	bool trapState{false};

	// Instruction dispatch/execution functions
	[[nodiscard]] int16_t readIndex(uint16_t extension) const noexcept;
	[[nodiscard]] int32_t readExtraDisplacement(uint8_t displacementSize) noexcept;
	[[nodiscard]] uint32_t readImmediateUnsigned(size_t immediateSize) noexcept;
	[[nodiscard]] int32_t readImmediateSigned(size_t immediateSize) noexcept;
	[[nodiscard]] uint32_t computeIndirect(uint32_t baseAddress) noexcept;
	[[nodiscard]] uint32_t computeEffectiveAddress(uint8_t mode, uint8_t reg, size_t operandSize) noexcept;
	template<typename T> [[nodiscard]] T readValue(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
	template<typename T> [[nodiscard]] T readValue(uint8_t mode, uint8_t reg, uint32_t address, size_t width) noexcept;
	template<typename T> void writeValue(uint8_t mode, uint8_t reg, uint32_t address, T value) noexcept;
	template<typename T> void writeValue(uint8_t mode, uint8_t reg, uint32_t address, size_t width, T value) noexcept;
	template<typename T> [[nodiscard]] T readEffectiveAddress(uint8_t mode, uint8_t reg, size_t width) noexcept;
	template<typename T> void writeEffectiveAddress(uint8_t mode, uint8_t reg, size_t width, T value) noexcept;

	template<typename T> [[nodiscard]] T readEffectiveAddress(uint8_t mode, uint8_t reg) noexcept
	{
		using effective_t = std::conditional_t<std::is_unsigned_v<T>, uint32_t, int32_t>;
		return readEffectiveAddress<effective_t>(mode, reg, sizeof(T));
	}

	template<typename T> void writeEffectiveAddress(uint8_t mode, uint8_t reg, T value) noexcept
	{
		using effective_t = std::conditional_t<std::is_unsigned_v<T>, uint32_t, int32_t>;
		writeEffectiveAddress(mode, reg, sizeof(T), effective_t{value});
	}

	[[nodiscard]] uint32_t &activeStackPointer() noexcept;
	[[nodiscard]] const uint32_t &activeStackPointer() const noexcept;
	[[nodiscard]] int32_t readImmediateDisplacement(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] bool checkCondition(uint8_t condition) const noexcept;
	[[nodiscard]] size_t unpackSize(uint8_t sizeField) const noexcept;
	void recomputeStatusFlags(uint32_t lhs, uint32_t rhs, uint64_t result, size_t operationSize) noexcept;
	void recomputeStatusFlags(uint32_t result, bool carry, uint32_t signBit) noexcept;
	void recomputeStatusFlagsArithmetic(uint32_t result, bool carry, bool oldSign, uint32_t signBit) noexcept;

	[[nodiscard]] uint32_t &dataRegister(size_t reg) noexcept;
	[[nodiscard]] const uint32_t &dataRegister(size_t reg) const noexcept;
	[[nodiscard]] uint32_t &addrRegister(size_t reg) noexcept;
	[[nodiscard]] const uint32_t &addrRegister(size_t reg) const noexcept;

	[[nodiscard]] int32_t readDataRegisterSigned(size_t reg, size_t size) const noexcept;
	void writeDataRegisterSized(size_t reg, size_t size, uint32_t value) noexcept;

	[[nodiscard]] stepResult_t dispatchADD(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchADDA(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchADDI(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchADDQ(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchANDI(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchANDISpecialCCR(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchASL(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchASR(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchBcc(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchBCLR(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchBRA(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchBSET(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchBSR(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchBTST(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchCINV(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchCLR(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchCMP(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchCMPA(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchCMPI(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchCPUSH(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchDBcc(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchDIVS(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchDIVU(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchEXT(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchEXTB(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchJMP(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchLEA(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchLSL(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchLSR(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchMOVE(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchMOVESpecialCCR(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchMOVESpecialSR(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchMOVESpecialUSP(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchMOVEA(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchMOVEM(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchMOVEP(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchMOVEQ(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchMULS(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchMULU(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchNEG(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchOR(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchORI(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchORISpecialCCR(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchRTE() noexcept;
	[[nodiscard]] stepResult_t dispatchRTS() noexcept;
	[[nodiscard]] stepResult_t dispatchScc(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchSUB(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchSUBQ(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchSWAP(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchTRAP(const decodedOperation_t &insn) noexcept;
	[[nodiscard]] stepResult_t dispatchTST(const decodedOperation_t &insn) noexcept;

public:
	motorola68000_t(memoryMap_t<uint32_t, 0x00ffffffU> &peripherals, uint32_t clockFreq) noexcept;

	void executeFrom(uint32_t entryAddress, uint32_t stackTop, bool asUser = true) noexcept;
	void executeFromException(uint32_t entryAddress, uint32_t stackTop, uint8_t vectorNumber) noexcept;
	[[nodiscard]] bool executeToReturn(uint32_t entryAddress, uint32_t stackTop, bool asUser = true) noexcept;
	void stageIRQCall(uint32_t vectorAddress) noexcept;
	[[nodiscard]] uint32_t readDataRegister(size_t reg) const noexcept;
	void writeDataRegister(size_t reg, uint32_t value) noexcept;
	[[nodiscard]] uint32_t readAddrRegister(size_t reg) const noexcept;
	void writeAddrRegister(size_t reg, uint32_t value) noexcept;
	[[nodiscard]] uint32_t readProgramCounter() const noexcept;
	[[nodiscard]] uint16_t readStatus() const noexcept;
	[[nodiscard]] stepResult_t step() noexcept;
	[[nodiscard]] bool advanceClock() noexcept;
	[[nodiscard]] bool trapped() const noexcept { return trapState; }

	void displayRegs() const noexcept;

	// Not actually part of the public interface, just necessary to be exposed for testing
	decodedOperation_t decodeInstruction(uint16_t insn) const noexcept;
};

extern template uint8_t motorola68000_t::readValue<uint8_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
extern template int8_t motorola68000_t::readValue<int8_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
extern template uint16_t motorola68000_t::readValue<uint16_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
extern template int16_t motorola68000_t::readValue<int16_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
extern template uint32_t motorola68000_t::readValue<uint32_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;
extern template int32_t motorola68000_t::readValue<int32_t>(uint8_t mode, uint8_t reg, uint32_t address) noexcept;

extern template uint32_t
	motorola68000_t::readValue<uint32_t>(uint8_t mode, uint8_t reg, uint32_t address, size_t width) noexcept;
extern template int32_t
	motorola68000_t::readValue<int32_t>(uint8_t mode, uint8_t reg, uint32_t address, size_t width) noexcept;

extern template uint32_t
	motorola68000_t::readEffectiveAddress<uint32_t>(uint8_t mode, uint8_t reg, size_t width) noexcept;
extern template int32_t
	motorola68000_t::readEffectiveAddress<int32_t>(uint8_t mode, uint8_t reg, size_t width) noexcept;

extern template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, int8_t value) noexcept;
extern template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, uint8_t value) noexcept;
extern template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, int16_t value) noexcept;
extern template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, uint16_t value) noexcept;
extern template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, int32_t value) noexcept;
extern template void motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, uint32_t value) noexcept;

extern template void
	motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, size_t width, uint32_t value) noexcept;
extern template void
	motorola68000_t::writeValue(uint8_t mode, uint8_t reg, uint32_t address, size_t width, int32_t value) noexcept;

extern template void
	motorola68000_t::writeEffectiveAddress(uint8_t mode, uint8_t reg, size_t width, uint32_t value) noexcept;
extern template void
	motorola68000_t::writeEffectiveAddress(uint8_t mode, uint8_t reg, size_t width, int32_t value) noexcept;

#endif /*EMULATOR_CPU_M68K_HXX*/
