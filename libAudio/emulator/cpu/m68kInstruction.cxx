// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <string_view>
#include "m68kInstruction.hxx"
#include "console.hxx"

using namespace std::literals::string_view_literals;
using namespace libAudio::console;

static void displayEA(const uint8_t mode, const uint8_t reg) noexcept
{
	switch (mode)
	{
	case 0U:
		console.output('d', reg, nullptr);
		break;
	case 1U:
		if (reg == 7U)
			console.output("sp"sv, nullptr);
		else
			console.output('a', reg, nullptr);
		break;
	case 2U:
		if (reg == 7U)
			console.output("(sp)"sv, nullptr);
		else
			console.output("(a"sv, reg, ')', nullptr);
		break;
	case 3U:
		if (reg == 7U)
			console.output("(sp)+"sv, nullptr);
		else
			console.output("(a"sv, reg, ")+"sv, nullptr);
		break;
	case 4U:
		if (reg == 7U)
			console.output("-(sp)"sv, nullptr);
		else
			console.output("-(a"sv, reg, ')', nullptr);
		break;
	case 5U:
		if (reg == 7U)
			console.output("disp16(sp)"sv, nullptr);
		else
			console.output("disp16(a"sv, reg, ')', nullptr);
		break;
	case 6U:
		if (reg == 7U)
			console.output("disp8(sp, idx)"sv, nullptr);
		else
			console.output("disp8(a"sv, reg, ", idx)"sv, nullptr);
		break;
	case 7U:
		switch (reg)
		{
		case 0U:
			console.output("(addr16).w"sv, nullptr);
			break;
		case 1U:
			console.output("(addr32).l"sv, nullptr);
			break;
		case 2U:
			console.output("disp16(pc)"sv, nullptr);
			break;
		case 3U:
			console.output("disp8(pc, idx)"sv, nullptr);
			break;
		case 4U:
			console.output("#<val>"sv, nullptr);
			break;
		}
	}
}

std::string_view condition(const uint8_t condition) noexcept
{
	switch (condition)
	{
		case 0x0U:
			return "t"sv;
		case 0x1U:
			return "f"sv;
		case 0x2U:
			return "hi"sv;
		case 0x3U:
			return "ls"sv;
		case 0x4U:
			return "cc"sv;
		case 0x5U:
			return "cs"sv;
		case 0x6U:
			return "ne"sv;
		case 0x7U:
			return "eq"sv;
		case 0x8U:
			return "vc"sv;
		case 0x9U:
			return "vs"sv;
		case 0xaU:
			return "pl"sv;
		case 0xbU:
			return "mi"sv;
		case 0xcU:
			return "ge"sv;
		case 0xdU:
			return "lt"sv;
		case 0xeU:
			return "gt"sv;
		case 0xfU:
			return "le"sv;
	}
	return "xx"sv;
}

std::string_view cacheName(const uint8_t cache)
{
	switch (cache)
	{
		case 1U:
			return "dc"sv;
		case 2U:
			return "ic"sv;
		case 3U:
			return "bc"sv;
	}
	return "nc"sv;
}

void displayDisplacement(const uint8_t displacement) noexcept
{
	if (displacement == 0x00U)
		console.output("disp16"sv);
	else if (displacement == 0xffU)
		console.output("disp32"sv);
	else
		console.output('$', static_cast<int8_t>(displacement));
}

uint8_t unpackSize(const uint8_t size) noexcept
	{ return 1U << size; }

static char operandSize(const uint8_t size) noexcept
{
	if (size == 1U)
		return 'b';
	if (size == 2U)
		return 'w';
	return 'l';
}

void displayMOVESpecialCCR(const decodedOperation_t &insn)
{
	if (insn.rx == 8U)
	{
		console.output("move ccr, "sv, nullptr);
		displayEA(insn.mode, insn.ry);
		console.output();
	}
	else
	{
		console.output("move "sv, nullptr);
		displayEA(insn.mode, insn.rx);
		console.output(", ccr"sv);
	}
}

void displayMOVESpecialSR(const decodedOperation_t &insn)
{
	if (insn.rx == 9U)
	{
		console.output("move sr, "sv, nullptr);
		displayEA(insn.mode, insn.ry);
		console.output();
	}
	else
	{
		console.output("move "sv, nullptr);
		displayEA(insn.mode, insn.rx);
		console.output(", sr"sv);
	}
}

void displayMOVESpecialUSP(const decodedOperation_t &insn)
{
	if (insn.rx == 10U)
	{
		console.output("move usp, "sv, nullptr);
		displayEA(insn.mode, insn.ry);
		console.output();
	}
	else
	{
		console.output("move "sv, nullptr);
		displayEA(insn.mode, insn.rx);
		console.output(", usp"sv);
	}
}

void displayMOVE(const decodedOperation_t &insn)
{
	switch (std::max(insn.rx, insn.ry))
	{
		case 8U:
			displayMOVESpecialCCR(insn);
			return;
		case 9U:
			displayMOVESpecialSR(insn);
			return;
		case 10U:
			displayMOVESpecialUSP(insn);
			return;
	}
	console.output("move."sv, operandSize(insn.operationSize), ' ', nullptr);
	displayEA(insn.mode & 0x07U, insn.ry);
	console.output(", "sv, nullptr);
	displayEA((insn.mode & 0x38U) >> 3U, insn.rx);
	console.output();
}

void decodedOperation_t::display(const uint32_t programCounter) const noexcept
{
	console.debug(asHex_t<6, '0'>{programCounter}, ": ", nullptr);
	switch (operation)
	{
		case instruction_t::add:
			console.output("add."sv, operandSize(unpackSize(operationSize)), ' ', nullptr);
			if (opMode)
			{
				console.output('d', rx, ", "sv, nullptr);
				displayEA(mode, ry);
				console.output();
			}
			else
			{
				displayEA(mode, ry);
				console.output(", d"sv, rx);
			}
			break;
		case instruction_t::adda:
			console.output("adda."sv, operandSize(operationSize), ' ', nullptr);
			displayEA(mode, ry);
			console.output(", a"sv, rx);
			break;
		case instruction_t::addi:
			console.output("addi."sv, operandSize(unpackSize(operationSize)), " #<val>, "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::addq:
			console.output("addq."sv, operandSize(unpackSize(operationSize)), " #"sv,
				rx == 0U ? 8U : rx, ", "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::andi:
			switch (ry)
			{
			case 8U:
				console.output("andi #<val>, ccr"sv);
				break;
			case 9U:
				console.output("andi #<val>, sr"sv);
				break;
			default:
				console.output("andi."sv, operandSize(unpackSize(operationSize)), " #<val>, "sv, nullptr);
				displayEA(mode, ry);
				console.output();
				break;
			}
			break;
		case instruction_t::asl:
		case instruction_t::asr:
			console.output("as"sv, operation == instruction_t::asl ? 'l' : 'r', '.',
				operandSize(unpackSize(operationSize)), ' ', nullptr);
			if (flags.includes(operationFlags_t::immediateNotRegister))
				console.output('#', rx == 0U ? 8U : rx, ", d"sv, ry);
			else if (flags.excludes(operationFlags_t::memoryNotRegister))
				console.output('d', rx, ", d"sv, ry);
			else
			{
				displayEA(mode, ry);
				console.output();
			}
			break;
		case instruction_t::clr:
			console.output("clr."sv, operandSize(unpackSize(operationSize)), ' ', nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::bcc:
			console.output('b', condition(opMode), ' ', nullptr);
			displayDisplacement(rx);
			break;
		case instruction_t::bclr:
			console.output("bclr "sv, nullptr);
			if (flags.includes(operationFlags_t::immediateNotRegister))
				console.output("#<val>, "sv, nullptr);
			else
				console.output('d', rx, ", "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::bra:
			console.output("bra "sv, nullptr);
			displayDisplacement(rx);
			break;
		case instruction_t::bset:
			console.output("bset "sv, nullptr);
			if (flags.includes(operationFlags_t::immediateNotRegister))
				console.output("#<val>, "sv, nullptr);
			else
				console.output('d', rx, ", "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::bsr:
			console.output("bsr "sv, nullptr);
			displayDisplacement(rx);
			break;
		case instruction_t::btst:
			console.output("btst "sv, nullptr);
			if (flags.includes(operationFlags_t::immediateNotRegister))
				console.output("#<val>, "sv, nullptr);
			else
				console.output('d', rx, ", "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::cinva:
			console.output("cinva "sv, cacheName(ry));
			break;
		case instruction_t::cinvl:
			console.output("cinvl "sv, cacheName(ry), ", "sv, nullptr);
			if (rx == 7U)
				console.output("(sp)"sv);
			else
				console.output("(a"sv, rx, ')');
			break;
		case instruction_t::cinvp:
			console.output("cinvp "sv, cacheName(ry), ", "sv, nullptr);
			if (rx == 7U)
				console.output("(sp)"sv);
			else
				console.output("(a"sv, rx, ')');
			break;
		case instruction_t::cmp:
			console.output("cmp."sv, operandSize(unpackSize(operationSize)), ' ', nullptr);
			displayEA(mode, ry);
			console.output(", d"sv, rx);
			break;
		case instruction_t::cmpa:
			console.output("cmpa."sv, operandSize(operationSize), ' ', nullptr);
			displayEA(mode, ry);
			if (rx == 7U)
				console.output(", sp"sv);
			else
				console.output(", a"sv, rx);
			break;
		case instruction_t::cmpi:
			console.output("cmpi."sv, operandSize(unpackSize(operationSize)), " #<val>, "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::cpusha:
			console.output("cpusha "sv, cacheName(ry));
			break;
		case instruction_t::cpushl:
			console.output("cpushl "sv, cacheName(ry), ", "sv, nullptr);
			if (rx == 7U)
				console.output("(sp)"sv);
			else
				console.output("(a"sv, rx, ')');
			break;
		case instruction_t::cpushp:
			console.output("cpushp "sv, cacheName(ry), ", "sv, nullptr);
			if (rx == 7U)
				console.output("(sp)"sv);
			else
				console.output("(a"sv, rx, ')');
			break;
		case instruction_t::dbcc:
			console.output("db"sv, condition(opMode), " d"sv, ry, ", disp16"sv);
			break;
		case instruction_t::divs:
			console.output("divs.w "sv, nullptr);
			displayEA(mode, ry);
			console.output(", d"sv, rx);
			break;
		case instruction_t::divu:
			console.output("divu.w "sv, nullptr);
			displayEA(mode, ry);
			console.output(", d"sv, rx);
			break;
		case instruction_t::eori:
			console.output("eori."sv, operandSize(unpackSize(operationSize)), " #<val>, "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::ext:
			console.output("ext."sv, operationSize == 1U ? 'w' : 'l', " d"sv, rx);
			break;
		case instruction_t::extb:
			console.output("extb.l d"sv, rx);
			break;
		case instruction_t::jmp:
			console.output("jmp "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::jsr:
			console.output("jsr "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::lea:
			console.output("lea.l "sv, nullptr);
			displayEA(mode, ry);
			console.output(", a"sv, rx);
			break;
		case instruction_t::lsl:
		case instruction_t::lsr:
			console.output("ls"sv, operation == instruction_t::lsl ? 'l' : 'r', '.',
				operandSize(unpackSize(operationSize)), ' ', nullptr);
			if (flags.includes(operationFlags_t::immediateNotRegister))
				console.output('#', rx == 0U ? 8U : rx, ", d"sv, ry);
			else if (flags.excludes(operationFlags_t::memoryNotRegister))
				console.output('d', rx, ", d"sv, ry);
			else
			{
				displayEA(mode, ry);
				console.output();
			}
			break;
		case instruction_t::move:
			displayMOVE(*this);
			break;
		case instruction_t::movea:
			console.output("movea."sv, operandSize(operationSize), ' ', nullptr);
			displayEA(mode, ry);
			if (rx == 7U)
				console.output(", sp"sv);
			else
				console.output(", a"sv, rx);
			break;
		case instruction_t::movem:
			console.output("movem."sv, operandSize(operationSize), ' ', nullptr);
			if (opMode)
			{
				displayEA(mode, ry);
				console.output(", regs"sv);
			}
			else
			{
				console.output("regs, "sv, nullptr);
				displayEA(mode, ry);
				console.output();
			}
			break;
		case instruction_t::movep:
			console.output("movep."sv, operandSize(operationSize), ' ', nullptr);
			if (opMode)
			{
				console.output('d', rx, ", disp16"sv, nullptr);
				if (rx == 7U)
					console.output("(sp)"sv);
				else
					console.output("(a"sv, rx, ')');
			}
			else
			{
				console.output("disp16"sv, nullptr);
				if (rx == 7U)
					console.output("(sp)"sv, nullptr);
				else
					console.output("(a"sv, rx, ')', nullptr);
				console.output(", d"sv, rx);
			}
			break;
		case instruction_t::moveq:
			console.output("moveq #"sv, static_cast<int8_t>(ry), ", d"sv, rx);
			break;
		case instruction_t::muls:
			console.output("muls.w "sv, nullptr);
			displayEA(mode, ry);
			console.output(", d"sv, rx);
			break;
		case instruction_t::mulu:
			console.output("mulu.w "sv, nullptr);
			displayEA(mode, ry);
			console.output(", d"sv, rx);
			break;
		case instruction_t::neg:
			console.output("neg."sv, operandSize(unpackSize(operationSize)), ' ', nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::nop:
			console.output("nop"sv);
			break;
		case instruction_t::_or:
			console.output("or."sv, operandSize(unpackSize(operationSize)), ' ', nullptr);
			if (opMode)
			{
				console.output('d', rx, ", "sv, nullptr);
				displayEA(mode, ry);
				console.output();
			}
			else
			{
				displayEA(mode, ry);
				console.output(", d"sv, rx);
			}
			break;
		case instruction_t::ori:
			console.output("ori."sv, operandSize(unpackSize(operationSize)), " #<val>, "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::rte:
			console.output("rte"sv);
			break;
		case instruction_t::rts:
			console.output("rts"sv);
			break;
		case instruction_t::scc:
			console.output('s', condition(opMode), ' ', nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::sub:
			console.output("sub."sv, operandSize(unpackSize(operationSize)), ' ', nullptr);
			if (opMode)
			{
				console.output('d', rx, ", "sv, nullptr);
				displayEA(mode, ry);
				console.output();
			}
			else
			{
				displayEA(mode, ry);
				console.output(", d"sv, rx);
			}
			break;
		case instruction_t::suba:
			console.output("suba."sv, operandSize(operationSize), ' ', nullptr);
			displayEA(mode, ry);
			console.output(", a"sv, rx);
			break;
		case instruction_t::subi:
			console.output("subi."sv, operandSize(unpackSize(operationSize)), " #<val>, "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::subq:
			console.output("subq."sv, operandSize(unpackSize(operationSize)), " #"sv,
				rx == 0U ? 8U : rx, ", "sv, nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		case instruction_t::swap:
			console.output("swap d"sv, rx);
			break;
		case instruction_t::trap:
			console.output("trap #"sv, rx);
			break;
		case instruction_t::tst:
			console.output("tst."sv, operandSize(unpackSize(operationSize)), ' ', nullptr);
			displayEA(mode, ry);
			console.output();
			break;
		default:
			console.output("XXX: fixme "sv, static_cast<uint32_t>(operation));
	}
}
