// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2013-2023 Rachel Mant <git@dragonmux.network>
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <inttypes.h>

// Return (a * b) / c [ - no divide error ]
int32_t muldiv_c(int32_t a, int32_t b, int32_t c)
{
	int32_t result;
	uint64_t e;
	int32_t d = a;
	if (a < 0)
		a = -a;
	d ^= b;
	if (b < 0)
		b = -b;
	d ^= c;
	if (c < 0)
		c = -c;
	e = (uint32_t)a * (uint32_t)b;
	if ((uint64_t)c < e)
		result = (int32_t)(e / (uint64_t)c);
	else
		result = 0x7FFFFFFF;
	if (d < 0)
		result = -result;
	return result;
}

int muldiv_asm(long a, long b, long c)
{
	int result;
#ifdef _WINDOWS
	__asm
	{
		mov eax, a
		mov ebx, b
		or eax, eax
		mov edx, eax
		jge a_neg
		neg eax
a_neg:
		xor edx, ebx
		or ebx, ebx
		mov ecx, c
		jge b_neg
		neg ebx
b_neg:
		xor edx, ecx
		or ecx, ecx
		mov esi, edx
		jge c_neg
		neg ecx
c_neg:
		mul ebx
		cmp edx, ecx
		jae diverr // unsigned jge
		div ecx
		jmp ok
diverr:
		mov eax, 0x7FFFFFFF
ok:
		mov edx, esi
		or edx, edx
		jge r_neg
		neg eax
r_neg:
		mov result, eax
	}
#else
	asm(".intel_syntax noprefix\n"
		"\tor eax, eax\n"
		"\tmov edx, eax\n"
		"\tjge a_neg\n"
		"\tneg eax\n"
		"a_neg:\n"
		"\txor edx, ebx\n"
		"\tor ebx, ebx\n"
		"\tjge b_neg\n"
		"\tneg ebx\n"
		"b_neg:\n"
		"\txor edx, ecx\n"
		"\tor ecx, ecx\n"
		"\tmov esi, edx\n"
		"\tjge c_neg\n"
		"\tneg ecx\n"
		"c_neg:\n"
		"\tmul ebx\n"
		"\tcmp edx, ecx\n"
		"\tjae diverr\n" // unsigned jge
		"\tdiv ecx\n"
		"\tjmp ok\n"
		"diverr:\n"
		"\tmov eax, 0x7FFFFFFF\n"
		"ok:\n"
		"\tmov edx, esi\n"
		"\tor edx, edx\n"
		"\tjge r_neg\n"
		"\tneg eax\n"
		"r_neg:\n"
		".att_syntax\n" : [result] "=a" (result) : [a] "a" (a),
		[b] "b" (b), [c] "c" (c) : "edx", "esi");
#endif
	return result;
}

int main(int argc, char **argv)
{
	printf("ASM: (10 * 10) / 2 = %d\n", muldiv_asm(10, 10, 2));
	printf("  C: (10 * 10) / 2 = %d\n", muldiv_c(10, 10, 2));

	return 0;
}
