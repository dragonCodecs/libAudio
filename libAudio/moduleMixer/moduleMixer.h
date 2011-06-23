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
int muldiv(long a, long b, long c)
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

uint32_t __CDECL__ Convert32to16(void *_out, int *_in, uint32_t SampleCount)
{
	uint32_t result;
#ifdef _WINDOWS
	__asm
	{
		mov ebx, _out
		mov edx, _in
		mov edi, SampleCount
cliploop:
		mov eax, dword ptr [edx]
		add ebx, 2
		add eax, (1 << 11)
		add edx, 4
		cmp eax, (-0x07FFFFFF)
		jl cliplow
		cmp eax, (0x07FFFFFF)
		jg cliphigh
cliprecover:
		sar eax, 12
		dec edi
		mov word ptr [ebx - 2], ax
		jnz cliploop
		jmp done
cliplow:
		mov eax, (-0x07FFFFFF)
		jmp cliprecover
cliphigh:
		mov eax, (0x07FFFFFF)
		jmp cliprecover
done:
		mov eax, SampleCount
		add eax, eax
		mov result, eax
	}
#else
	asm(".intel_syntax noprefix\n"
#ifdef __x86_64__
		"\tpush rdi\n"
#else
		"\tpush edi\n"
#endif
		"cliploop:\n"
		"\tmov eax, dword ptr [edx]\n"
		"\tadd ebx, 2\n"
		"\tadd eax, 2048\n" // 1 << 11
		"\tadd edx, 4\n"
		"\tcmp eax, 0xF8000001\n" // This should be the negative of the next constant
		"\tjl cliplow\n"
		"\tcmp eax, 0x07FFFFFF\n"
		"\tjg cliphigh\n"
		"cliprecover:\n"
		"\tsar eax, 12\n"
		"\tdec edi\n"
		"\tmov word ptr [ebx - 2], ax\n"
		"\tjnz cliploop\n"
		"\tjmp done\n"
		"cliplow:\n"
		"\tmov eax, 0xF8000001\n"
		"\tjmp cliprecover\n"
		"cliphigh:\n"
		"\tmov eax, 0x07FFFFFF\n"
		"\tjmp cliprecover\n"
		"done:\n"
#ifdef __x86_64__
		"\tpop rax\n"
#else
		"\tpop eax\n"
#endif
		"\tadd eax, eax\n"
		".att_syntax\n" : [result] "=a" (result) : [_out] "b" (_out), 
		[_in] "d" (_in), [SampleCount] "D" (SampleCount) : );
#endif
	return result;
}

#endif /*__moduleMixer_H__*/

