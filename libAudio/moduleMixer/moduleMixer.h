#ifndef __moduleMixer_H__
#define __moduleMixer_H__

#define CHN_LOOP			0x01
#define CHN_TREMOLO			0x02
#define CHN_ARPEGGIO		0x04
#define CHN_VIBRATO			0x08
#define CHN_VOLUMERAMP		0x10
#define CHN_FASTVOLRAMP		0x20
#define CHN_PORTAMENTO		0x40
#define CHN_GLISSANDO		0x80

#define CLIPINT(num, min, max) \
	if (num < min) \
		num = min; \
	else if (num > max) \
		num = max

// Return (a * b) / c [ - no divide error ]
int32_t muldiv(int32_t a, int32_t b, int32_t c)
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
		result = e / c;
	else
		result = 0x7FFFFFFF;
	if (d < 0)
		result = -result;
	return result;
}

/*int muldiv(long a, long b, long c)
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
}*/

#endif /*__moduleMixer_H__*/

