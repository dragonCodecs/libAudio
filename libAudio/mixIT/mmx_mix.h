#define SURROUNDBUFFERSIZE		2048

static long XBassFlt_Y1 = 0;
static long XBassFlt_X1 = 0;
static long XBassFlt_B0 = 0;
static long XBassFlt_B1 = 0;
static long XBassFlt_A1 = 0;

static long DCRFlt_Y1l = 0;
static long DCRFlt_X1l = 0;
static long DCRFlt_Y1r = 0;
static long DCRFlt_X1r = 0;

static long LeftNR = 0;
static long RightNR = 0;

static long DolbyHP_B0 = 0;
static long DolbyHP_B1 = 0;
static long DolbyHP_X1 = 0;
static long DolbyHP_A1 = 0;
static long DolbyHP_Y1 = 0;
static long DolbyLP_B0 = 0;
static long DolbyLP_B1 = 0;
static long DolbyLP_A1 = 0;
static long DolbyLP_Y1 = 0;

static long SurroundBuffer[SURROUNDBUFFERSIZE];
static long SurroundPos = 0;
static long SurroundSize = 0;

#define MMX_ENTER \
	__asm push ebx \
	__asm push esi \
	__asm push edi \
	__asm push ebp

#define MMX_LEAVE \
	__asm pop ebp \
	__asm pop edi \
	__asm pop esi \
	__asm pop ebx \
	__asm ret

#define MMX_PARAM1 20[esp]
#define MMX_PARAM2 24[esp]
#define MMX_PARAM3 28[esp]
#define MMX_PARAM4 32[esp]
#define MMX_PARAM5 36[esp]

#define MMX_CHANNEL MMX_PARAM1
#define MMX_BUFFER MMX_PARAM2
#define MMX_BUFMAX MMX_PARAM3

#define CHNOFFS_CURRENTSAMPLE	0
#define CHNOFFS_POS				4
#define CHNOFFS_POSLO			8
#define CHNOFFS_INC				12
#define CHNOFFS_RIGHTVOL		16
#define CHNOFFS_LEFTVOL			20
#define CHNOFFS_RIGHTRAMP		24
#define CHNOFFS_LEFTRAMP		28

void MMX_EndMix()
{
	__asm
	{
		pxor mm0, mm0
		emms
	}
}

__declspec(naked) void __cdecl MMX_Mono8BitMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov eax, 0xFFFF
		movd mm6, eax
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov edx, MMX_BUFMAX
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		add esi, [ecx + CHNOFFS_POS]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		mov ebp, [ecx + CHNOFFS_INC]
		punpckldq mm6, mm6
		movq mm4, qword ptr [ecx + CHNOFFS_RIGHTVOL]
		pand mm4, mm6
		mov eax, edx
		sub eax, edi
		push ecx
		test eax, 8
		jz mixloop
		mov eax, ebx
		sar eax, 16
		movsx eax, byte ptr [esi + eax]
		add edi, 8
		shl eax, 8
		movd mm0, eax
		punpckldq mm0, mm0
		paddd mm0, qword ptr [edi - 8]
		add ebx, ebp
		cmp edi, edx
		movq qword ptr [esi - 8], mm0
		jb mixloop
		jmp done
		align 16
mixloop:
		mov eax, ebx
		add ebx, ebp
		sar eax, 16
		mov ecx, ebx
		movsx eax, byte ptr [esi + eax]
		sar ecx, 16
		add edi, 16
		movsx ecx, byte ptr [esi + ecx]
		shl eax, 8
		add ebx, ebp
		movd mm1, eax
		shl ecx, 8
		punpckldq mm1, mm1
		movd mm0, ecx
		pmaddwd mm1, mm4
		punpckldq mm0, mm0
		paddd mm1, qword ptr [edi - 16]
		pmaddwd mm0, mm4
		paddd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 16], mm1
		movq qword ptr [edi - 8], mm0
		jb mixloop
done:
		pop ecx
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_Mono16BitMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov eax, 0xFFFF
		movd mm6, eax
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov edx, MMX_BUFMAX
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		mov eax, [ecx + CHNOFFS_POS]
		lea esi, [esi + eax * 2]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		mov ebp, [ecx + CHNOFFS_INC]
		punpckldq mm6, mm6
		movq mm4, dword ptr [ecx + CHNOFFS_RIGHTVOL]
		pand mm4, mm6
		mov eax, edx
		sub eax, edi
		push ecx
		test eax, 8
		jz mixloop
		mov eax, ebx
		sar eax, 16
		movsx eax, word ptr [esi + eax * 2]
		add edi, 8
		movd mm0, eax
		punpckldq mm0, mm0
		pmaddwd mm0, mm4
		paddd mm0, qword ptr [edi - 8]
		add ebx, ebp
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		jmp done
		align 16
mixloop:
		mov eax, ebx
		add ebx, ebp
		sar eax, 16
		mov ecx, edx
		movsx eax, word ptr [esi + eax * 2]
		sar ecx, 16
		add edi, 16
		movsx ecx, word ptr [esi + ecx * 2]
		movd mm1, eax
		add ebx, ebp
		punpckldq mm1, mm1
		movd mm0, ecx
		pmaddwd mm0, mm4
		paddd mm1, qword ptr [edi - 16]
		paddd mm0, qword ptr [edi - 8]
		cmp edi, ebx
		movq qword ptr [edi - 16], mm1
		movq qword ptr [edi - 8], mm0
		jb mixloop
done:
		pop ecx
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_Mono8BitLinearMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		add esi, [ecx + CHNOFFS_POS]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		mov ebp, [ecx + CHNOFFS_INC]
		mov eax, 0x0000FFFF
		movq mm4, qword ptr [ecx + CHNOFFS_RIGHTVOL]
		movd mm6, eax
		punpckldq mm6, mm6
		pand mm6, eax
		mov ecx, MMX_BUFMAX
mixloop:
		mov eax, ebx
		sar eax, 16
		movsx edx, byte ptr [esi + eax]
		movsx eax, byte ptr [esi + eax + 1]
		add edi, 8
		movd mm1, eax
		movd mm0, edx
		movzx edx, bh
		psubd mm1, mm0
		movd mm2, edx
		pslld mm0, 8
		pmaddwd mm1, mm2
		paddd mm0, mm1
		punpckldq mm0, mm0
		pmaddwd mm0, mm4
		paddd mm0, qword ptr [edi - 8]
		add ebx, ebp
		cmp edi, ecx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		mov ecx, MMX_CHANNEL
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_Mono16BitLinearMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		mov eax, [ecx + CHNOFFS_POS]
		lea esi, [esi + eax * 2]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		mov ebp, [ecx + CHNOFFS_INC]
		mov eax, 0x0000FFFF
		movq mm4, qword ptr [ecx + CHNOFFS_RIGHTVOL]
		movd mm6, eax
		punpckldq mm6, mm6
		pand mm4, mm6
		mov ecx, MMX_BUFMAX
mixloop:
		mov eax, ebx
		sar eax, 16
		movsx edx, word ptr [esi + eax * 2]
		movsx eax, word ptr [esi + eax * 2 + 2]
		add edi, 8
		movd mm1, eax
		movzx eax, bh
		movd mm0, edx
		movd mm2, eax
		psubsw mm1, mm0
		pmaddwd mm1, mm2
		psrad mm1, 8
		paddd mm0, mm1
		punpckldq mm0, mm0
		pmaddwd mm0, mm4
		paddd mm0, qword ptr [edi - 8]
		add ebx, ebp
		cmp edi, ecx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		mov ecx, MMX_CHANNEL
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_Mono8BitHQMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov eax, 0xFFFF
		movd mm6, eax
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov edx, MMX_BUFMAX
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		add esi, [ecx + CHNOFFS_POS]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		dec esi
		mov ebp, [ecx + CHNOFFS_INC]
		punpckldq mm6, mm6
		movq mm4, qword ptr [ecx + CHNOFFS_RIGHTVOL]
		pand mm4, mm6
		paddsw mm4, mm4
mixloop:
		mov eax, ebx
		sar eax, 16
		movzx ecx, bh
		movd mm0, dword ptr [esi + eax]
		pxor mm7, mm7
		movq mm1, qword ptr [FastSinc + ecx * 8]
		punpcklbw mm7, mm0
		pmaddwd mm7, mm1
		add edi, 8
		movq mm0, mm7
		psrlq mm7, 32
		paddd mm0, mm7
		punpckldq mm0, mm0
		psrad mm0, 15
		packssdw mm0, mm0
		pmaddwd mm0, mm4
		paddd mm0, qword ptr [edi - 8]
		add ebx, ebp
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		mov ecx, MMX_CHANNEL
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_Mono16BitHQMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov eax, 0xFFFF
		movd mm6, eax
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov edx, MMX_BUFMAX
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		mov eax, [ecx + CHNOFFS_POS]
		lea esi, [esi + eax * 2 - 2]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		mov ebp, [ecx + CHNOFFS_INC]
		punpckldq mm6, mm6
		movq mm4, dword ptr [ecx + CHNOFFS_RIGHTVOL]
		pand mm4, mm6
mixloop:
		mov eax, ebx
		sar eax, 16
		movzx ecx, bh
		movq mm7, qword ptr [esi + eax * 2]
		movq mm1, qword ptr [FastSinc + ecx * 8]
		add edi, 8
		pmaddwd mm7, mm1
		movq mm0, mm7
		psrlq mm7, 32
		paddd mm0, mm7
		punpckldq mm0, mm0
		psrad mm0, 14
		packssdw mm0, mm0
		pmaddwd mm0, mm4
		paddd mm0, qword ptr [edi - 8]
		add ebx, ebp
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		mov ecx, MMX_CHANNEL
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_Mono8BitKaiserMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov eax, 0xFFFF
		movd mm6, eax
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov edx, MMX_BUFMAX
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		add esi, [ecx + CHNOFFS_POS]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		sub esi, 3
		mov ebp, [ecx + CHNOFFS_INC]
		punpckldq mm6, mm6
		movq mm4, qword ptr [ecx + CHNOFFS_RIGHTVOL]
		pand mm4, mm6
		paddsw mm4, mm4
		cmp ebp, 0x18000
		jg mixloop_2x
		cmp ebp, -0x18000
		jl mixloop_2x
		cmp ebp, 0x13000
		jg mixloop_3x
		cmp ebp, -0x13000
		jl mixloop_3x
mixloop:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm0, qword ptr [esi + eax]
		pxor mm6, mm6
		movq mm1, qword ptr [KaiserSinc + ecx]
		movq mm2, qword ptr [KaiserSinc + ecx + 8]
		punpcklbw mm6, mm0
		pxor mm7, mm7
		pmaddwd mm6, mm1
		punpckhbw mm7, mm0
		pmaddwd mm7, mm2
		add edi, 8
		add ebx, ebp
		paddd mm6, mm7
		movq mm0, mm6
		psrlq mm6, 32
		paddd mm0, mm6
		psrad mm0, 15
		punpckldq mm0, mm0
		packssdw mm0, mm0
		pmaddwd mm0, mm4
		paddd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		jmp done
mixloop_2x:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm0, qword ptr [esi + eax]
		pxor mm6, mm6
		movq mm1, qword ptr [DownSample2x + ecx]
		movq mm2, qword ptr [DownSample2x + ecx + 8]
		punpcklbw mm6, mm0
		pxor mm7, mm7
		pmaddwd mm6, mm1
		punpckhbw mm7, mm0
		pmaddwd mm7, mm2
		add edi, 8
		add ebx, ebp
		paddd mm6, mm7
		movq mm0, mm6
		psrlq mm6, 32
		paddd mm0, mm6
		psrad mm0, 15
		punpckldq mm0, mm0
		packssdw mm0, mm0
		pmaddwd mm0, mm4
		paddd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop_2x
		jmp done
mixloop_3x:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm0, qword ptr [esi + eax]
		pxor mm6, mm6
		movq mm1, qword ptr [DownSample13x + ecx]
		movq mm2, qword ptr [DownSample13x + ecx + 8]
		punpcklbw mm6, mm0
		pxor mm7, mm7
		pmaddwd mm6, mm1
		punpckhbw mm7, mm0
		pmaddwd mm7, mm2
		add edi, 8
		add ebx, ebp
		paddd mm6, mm7
		movq mm0, mm6
		psrlq mm6, 32
		paddd mm0, mm6
		psrad mm0, 15
		punpckldq mm0, mm0
		packssdw mm0, mm0
		pmaddwd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop_3x
done:
		mov ecx, MMX_CHANNEL
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_Mono8BitKaiserRampMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov edx, MMX_BUFMAX
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		add esi, [ecx + CHNOFFS_POS]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		sub esi, 3
		mov ebp, [ecx + CHNOFFS_INC]
		movq mm4, qword ptr [ecx + Channel.RampRightVol]
		movq mm3, qword ptr [ecx + Channel.RightRamp]
		cmp ebp, 0x18000
		jg mixloop_2x
		cmp ebp, -0x18000
		jl mixloop_2x
		cmp ebp, 0x13000
		jg mixloop_3x
		cmp ebp, -0x13000
		jl mixloop_3x
mixloop:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm0, qword ptr [esi + eax]
		pxor mm6, mm6
		movq mm1, qword ptr [KaiserSinc + ecx]
		movq mm2, qword ptr [KaiserSinc + ecx + 8]
		punpcklbw mm6, mm0
		pxor mm7, mm7
		pmaddwd mm6, mm1
		punpckhbw mm7, mm0
		pmaddwd mm7, mm2
		paddd mm4, mm3
		add edi, 8
		add ebx, ebp
		paddd mm6, mm7
		movq mm7, mm4
		movq mm0, mm6
		psrlq mm6, 32
		psrad mm7, VOLUMERAMPPRECISION - 1
		paddd mm0, mm6
		pxor mm6, mm6
		psrad mm0, 15
		packssdw mm7, mm7
		punpckldq mm0, mm0
		punpcklwd mm7, mm6
		packssdw mm0, mm0
		pmaddwd mm0, mm7
		paddd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		jmp done
mixloop_2x:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm0, qword ptr [esi + eax]
		pxor mm6, mm6
		movq mm1, qword ptr [DownSample2x + ecx]
		movq mm2, qword ptr [DownSample2x + ecx + 8]
		punpcklbw mm6, mm0
		pxor mm7, mm7
		pmaddwd mm6, mm1
		punpckhbw mm7, mm0
		pmaddwd mm7, mm2
		paddd mm4, mm3
		add edi, 8
		add edx, ebp
		paddd mm6, mm7
		movq mm7, mm4
		movq mm0, mm6
		psrlq mm6, 32
		psrad mm7, VOLUMERAMPPRECISION - 1
		paddd mm0, mm6
		pxor mm6, mm6
		psrad mm0, 15
		packssdw mm7, mm7
		punpckldq mm0, mm0
		punpcklwd mm7, mm6
		packssdw mm0, mm0
		pmaddwd mm0, mm7
		paddd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop_2x
		jmp done
mixloop_3x:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm0, qword ptr [esi + eax]
		pxor mm6, mm6
		movq mm1, qword ptr [DownSample13x + ecx]
		movq mm2, qword ptr [DownSample13x + ecx + 8]
		punpcklbw mm6, mm0
		pxor mm7, mm7
		pmaddwd mm6, mm1
		punpckhbw mm7, mm0
		pmaddwd mm7, mm2
		paddd mm4, mm3
		add edi, 8
		add ebx, ebp
		paddd mm6, mm7
		movq mm7, mm4
		movq mm0, mm6
		psrlq mm6, 32
		psrad mm7, VOLUMERAMPPRECISION - 1
		paddd mm0, mm6
		pxor mm6, mm6
		psrad mm0, 15
		packssdw mm7, mm7
		punpckldq mm0, mm0
		punpcklwd mm7, mm6
		packssdw mm0, mm0
		pmaddwd mm0, mm7
		paddd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop_3x
done:
		mov ecx, MMX_CHANNEL
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		movq qword ptr [ecx + Channel.RampRightVol], mm4
		psrad mm4, VOLUMERAMPPRECISION
		movq qword ptr [ecx + Channel.RightVol], mm4
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_Mono16BitKaiserMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov eax, 0xFFFF
		movd mm6, eax
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov edx, MMX_BUFMAX
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		mov eax, [ecx + CHNOFFS_POS]
		lea esi, [esi + eax * 2 - 6]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		mov ebp, [ecx + CHNOFFS_INC]
		punpckldq mm6, mm6
		movq mm4, dword ptr [ecx + CHNOFFS_RIGHTVOL]
		pand mm4, mm6
		cmp ebp, 0x18000
		jg mixloop_2x
		cmp ebp, -0x80000
		jl mixloop_2x
		cmp ebp, 0x13000
		jg mixloop_3x
		cmp ebp, -0x13000
		jl mixloop_3x
mixloop:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm6, qword ptr [esi + eax * 2]
		movq mm1, qword ptr [KaiserSinc + ecx]
		movq mm7, qword ptr [esi + eax * 2 + 8]
		movq mm2, qword ptr [KaiserSinc + ecx + 8]
		pmaddwd mm6, mm1
		add edi, 8
		pmaddwd mm7, mm2
		add ebx, ebp
		paddd mm6, mm7
		movq mm0, mm6
		psrlq mm6, 32
		paddd mm0, mm6
		psrad mm0, 14
		punpckldq mm0, mm0
		packssdw mm0, mm0
		pmaddwd mm0, mm4
		paddd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		jmp done
mixloop_2x:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm6, qword ptr [esi + eax * 2]
		movq mm1, qword ptr [DownSample2x + ecx]
		movq mm7, qword ptr [esi + eax * 2 + 6]
		movq mm2, qword ptr [DownSample2x + ecx + 8]
		pmaddwd mm6, mm1
		add edi, 8
		pmaddwd mm7, mm2
		add ebx, ebp
		paddd mm6, mm7
		movq mm0, mm6
		psrlq mm6, 32
		paddd mm0, mm6
		psrad mm0, 14
		punpckldq mm0, mm0
		packssdw mm0, mm0
		pmaddwd mm0, mm4
		paddd mm0, qword ptr [edi - 8]
		cmp edi, ebx
		movq qword ptr [edi - 8], mm0
		jb mixloop_2x
		jmp done
mixloop_3x:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		add ecx, 0xFFF0
		movq mm6, qword ptr [esi + eax * 2]
		movq mm1, qword ptr [DownSample13x + ecx]
		movq mm7, qword ptr [esi + eax * 2 + 8]
		movq mm2, qword ptr [DownSample13x + ecx + 8]
		pmaddwd mm6, mm1
		add edi, 8
		pmaddwd mm7, mm2
		add ebx, ebp
		paddd mm6, mm7
		movq mm0, mm6
		psrlq mm6, 32
		paddd mm0, mm6
		psrad mm0, 14
		punpckldq mm0, mm0
		packssdw mm0, mm0
		pmaddwd mm0, mm5
		paddd mm0, qword ptr [edi - 8]
		cmp edi, ebx
		movq qword ptr [edi - 8], mm0
		jb mixloop_3x
done:
		mov ecx, MMX_CHANNEL
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_Mono16BitKaiserRampMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov edx, MMX_BUFMAX
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		mov eax, [ecx + CHNOFFS_POS]
		lea esi, [esi + eax * 2 - 6]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		mov ebp, [ecx + CHNOFFS_INC]
		movq mm4, qword ptr [ecx + Channel.RampRightVol]
		movq mm3, qword ptr [ecx + Channel.RightRamp]
		cmp ebp, 0x18000
		jg mixloop_2x
		cmp ebp, -0x18000
		jl mixloop_2x
		cmp ebp, 0x13000
		jg mixloop_3x
		cmp ebp, -0x13000
		jl mixloop_3x
mixloop:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm6, qword ptr [esi + eax * 2]
		movq mm1, qword ptr [KaiserSinc + ecx]
		movq mm7, qword ptr [esi + eax * 2 + 8]
		movq mm2, qword ptr [KaiserSinc + ecx + 8]
		pmaddwd mm6, mm1
		add edi, 8
		pmaddwd mm7, mm2
		paddd mm4, mm3
		add ebx, ebp
		paddd mm6, mm7
		movq mm7, mm4
		movq mm0, mm6
		psrlq mm6, 32
		psrad mm7, VOLUMERAMPPRECISION
		paddd mm0, mm6
		pxor mm6, mm6
		psrad mm0, 14
		packssdw mm7, mm7
		punpckldq mm0, mm0
		punpcklwd mm7, mm6
		packssdw mm0, mm0
		pmaddwd mm0, mm7
		paddd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		jmp done
mixloop_2x:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm6, qword ptr [esi + eax * 2]
		movq mm1, qword ptr [DownSample2x + ecx]
		movq mm7, qword ptr [esi + eax * 2 + 8]
		movq mm2, qword ptr [DownSample2x + ecx + 8]
		pmaddwd mm6, mm1
		add edi, 8
		pmaddwd mm7, mm2
		paddd mm4, mm3
		add ebx, ebp
		paddd mm6, mm7
		movq mm7, mm4
		movq mm0, mm6
		psrlq mm6, 32
		psrad mm7, VOLUMERAMPPRECISION
		paddd mm0, mm6
		pxor mm6, mm6
		psrad mm0, 14
		packssdw mm7, mm7
		punpckldq mm0, mm0
		punpcklwd mm7, mm6
		packssdw mm0, mm0
		pmaddwd mm0, mm7
		paddd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop_2x
		jmp done
mixloop_3x:
		mov eax, ebx
		mov ecx, ebx
		sar eax, 16
		and ecx, 0xFFF0
		movq mm6, qword ptr [esi + eax * 2]
		movq mm1, qword ptr [DownSample13x + ecx]
		movq mm7, qword ptr [esi + eax * 2 + 8]
		movq mm2, qword ptr [DownSample13x + ecx + 8]
		pmaddwd mm6, mm1
		add edi, 8
		pmaddwd mm7, mm2
		paddd mm4, mm3
		add ebx, ebp
		paddd mm6, mm7
		movq mm7, mm4
		movq mm0, mm6
		psrlq mm6, 32
		psrad mm7, VOLUMERAMPPRECISION
		paddd mm0, mm6
		pxor mm6, mm6
		psrad mm0, 14
		packssdw mm7, mm7
		punpckldq mm0, mm0
		punpcklwd mm7, mm6
		packssdw mm0, mm0
		pmaddwd mm0, mm7
		paddd mm0, qword ptr [edi - 8]
		cmp edi, edx
		movq qword ptr [edi - 8], mm0
		jb mixloop_3x
done:
		mov ecx, MMX_CHANNEL
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		movq qword ptr [ecx + Channel.RampRightVol], mm4
		psrad mm4, VOLUMERAMPPRECISION
		movq qword ptr [ecx + Channel.RightVol], mm4
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_FilterMono8BitLinearRampMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		mov eax, [ecx + CHNOFFS_POS]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		mov ebp, [ecx + CHNOFFS_INC]
		mov edx, [ecx + Channel.RampLength]
		add esi, eax
		or edx, edx
		movq mm4, qword ptr [ecx + Channel.RampRightVol]
		movq mm3, qword ptr [ecx + Channel.RightRamp]
		jnz noramp
		movq mm4, qword ptr [ecx + Channel.RightVol]
		pxor mm3, mm3
		pslld mm4, VOLUMERAMPPRECISION
noramp:
		movq mm6, qword ptr [ecx + Channel.Filter_Y1]
		movd mm0, dword ptr [ecx + Channel.Filter_B0]
		movd mm1, dword ptr [ecx + Channel.Filter_B1]
		punpckldq mm0, mm1
		movd mm7, dword ptr [ecx + Channel.Filter_HP]
		movd mm2, dword ptr [ecx + Channel.Filter_A0]
		punpckldq mm7, mm2
		packssdw mm7, mm0
		mov ecx, MMX_BUFMAX
mixloop:
		mov eax, ebx
		sar eax, 16
		add edi, 8
		movsx edx, byte ptr [esi + eax]
		movsx eax, byte ptr [esi + eax]
		movd mm0, edx
		sub eax, edx
		movzx edx, bh
		imul edx, eax
		pslld mm0, 8
		movd mm1, edx
		movq mm5, mm7
		paddd mm0, mm1
		pxor mm1, mm1
		psrad mm0, 1
		punpcklwd mm5, mm5
		packssdw mm1, mm6
		pand mm5, mm0
		pslld mm0, 16
		por mm0, mm1
		pmaddwd mm0, mm7
		mov eax, 4096
		paddd mm4, mm3
		movd mm1, eax
		paddd mm1, mm0
		punpckhdq mm0, mm0
		paddd mm0, mm1
		psrad mm0, 13
		movq mm1, mm0
		punpckldq mm0, mm0
		psubd mm1, mm5
		movq mm5, qword ptr [edi - 8]
		movq mm2, mm4
		psrad mm2, VOLUMERAMPPRECISION
		packssdw mm2, mm2
		packssdw mm0, mm0
		punpcklwd mm2, mm2
		pmaddwd mm0, mm2
		punpckldq mm1, mm6
		movq mm6, mm1
		add ebx, ebp
		paddd mm0, mm4
		cmp edi, ecx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		mov ecx, MMX_CHANNEL
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		movq qword ptr [ecx + Channel.RampRightVol], mm4
		psrad mm4, VOLUMERAMPPRECISION
		movq qword ptr [ecx + Channel.RightVol], mm4
		movq qword ptr [ecx + Channel.Filter_Y1], mm6
		MMX_LEAVE
	}
}

__declspec(naked) void __cdecl MMX_FilterMono16BitLinearRampMix(Channel *chn, int *Buff, int *BuffMax)
{
	__asm
	{
		MMX_ENTER
		mov ecx, MMX_CHANNEL
		mov edi, MMX_BUFFER
		mov esi, [ecx + CHNOFFS_CURRENTSAMPLE]
		mov eax, [ecx + CHNOFFS_POS]
		movzx ebx, word ptr [ecx + CHNOFFS_POSLO]
		mov ebp, [ecx + CHNOFFS_INC]
		mov edx, [ecx + Channel.RampLength]
		lea esi, [esi + eax * 2]
		or edx, edx
		movq mm4, qword ptr [ecx + Channel.RampRightVol]
		movq mm3, qword ptr [ecx + Channel.RightRamp]
		jnz noramp
		movq mm4, qword ptr [ecx + Channel.RightVol]
		pxor mm3, mm3
		pslld mm4, VOLUMERAMPPRECISION
noramp:
		movq mm6, qword ptr [ecx + Channel.Filter_Y1]
		movd mm0, dword ptr [ecx + Channel.Filter_B0]
		movd mm1, dword ptr [ecx + Channel.Filter_B1]
		punpckldq mm0, mm1
		movd mm7, dword ptr [ecx + Channel.Filter_HP]
		movd mm2, dword ptr [ecx + Channel.Filter_A0]
		punpckldq mm7, mm2
		packssdw mm7, mm0
		mov ecx, MMX_BUFMAX
mixloop:
		mov eax, ebx
		sar eax, 16
		add edi, 8
		movd mm0, dword ptr [esi + eax * 2]
		movzx edx, bh
		mov eax, 0x0100
		sub eax, edx
		shl edx, 16
		or eax, edx
		movd mm1, eax
		pmaddwd mm0, mm1
		movq mm5, mm7
		pxor mm1, mm1
		psrad mm0, 9
		punpcklwd mm5, mm5
		packssdw mm1, mm6
		pand mm4, mm0
		pslld mm0, 16
		por mm0, mm1
		pmaddwd mm0, mm7
		mov eax, 4096
		paddd mm4, mm3
		movd mm1, eax
		paddd mm1, mm0
		punpckhdq mm0, mm0
		paddd mm0, mm1
		psrad mm0, 13
		movq mm1, mm0
		punpckldq mm0, mm0
		psubd mm1, mm5
		movq mm4, qword ptr [edi - 8]
		movq mm2, mm4
		psrad mm2, VOLUMERAMPPRECISION
		packssdw mm2, mm2
		packssdw mm0, mm0
		punpcklwd mm2, mm2
		pmaddwd mm0, mm2
		punpckldq mm1, mm6
		movq mm6, mm1
		add ebx, ebp
		paddd mm0, mm5
		cmp edi, ecx
		movq qword ptr [edi - 8], mm0
		jb mixloop
		mov ecx, MMX_CHANNEL
		mov word ptr [ecx + CHNOFFS_POSLO], bx
		sar ebx, 16
		add dword ptr [ecx + CHNOFFS_POS], ebx
		movq qword ptr [ecx + Channel.RampRightVol], mm4
		psrad mm4, VOLUMERAMPPRECISION
		movq qword ptr [ecx + Channel.RightVol], mm4
		movq qword ptr [ecx + Channel.Filter_Y1], mm6
		MMX_LEAVE
	}
}

#define MMX_FilterMono8BitMix			MMX_FilterMono8BitLinearRampMix
#define MMX_FilterMono16BitMix			MMX_FilterMono16BitLinearRampMix
#define MMX_FilterMono8BitRampMix		MMX_FilterMono8BitLinearRampMix
#define MMX_FilterMono16BitRampMix		MMX_FilterMono16BitLinearRampMix
#define MMX_FilterMono8BitLinearMix		MMX_FilterMono8BitLinearRampMix
#define MMX_FilterMono16BitLinearMix	MMX_FilterMono16BitLinearRampMix

void SSE_StereoMixToFloat(int *Src, float *Out1, float *Out2, UINT Count, float IntToFloat)
{
	__asm
	{
		movss xmm0, IntToFloat
		mov edx, Src
		mov eax, Out1
		mov ebx, Out2
		mov ecx, Count
		shufps xmm0, xmm0, 0x00
		xorps xmm1, xmm1
		xorps xmm2, xmm2
		inc ecx
		shr ecx, 1
main_loop:
		cvtpi2ps xmm1, [edx + 0]
		cvtpi2ps xmm2, [edx + 8]
		add eax, 8
		add ebx, 8
		movlhps xmm1, xmm2
		mulps xmm1, xmm0
		add edx, 16
		shufps xmm1, xmm1, 0xD8
		dec ecx
		movlps qword ptr [eax - 8], xmm1
		movhps qword ptr [eax - 8], xmm1
		jnz main_loop
	}
}

void AMD_StereoMixToFloat(int *Src, float *Out1, float *Out2, UINT Count, float IntToFloat)
{
	__asm
	{
		movd mm0, IntToFloat
		mov edx, Src
		mov edi, Out1
		mov ebx, Out2
		mov ecx, Count
		punpckldq mm0, mm0
		inc ecx
		shr ecx, 1
main_loop:
		movq mm1, qword ptr [edx + 0]
		movq mm2, qword ptr [edx + 8]
		add edi, 8
		add ebx, 8
		add edx, 16
		dec ecx
		pi2fd mm1, mm1
		pi2fd mm2, mm2
		pfmul mm1, mm0
		pfmul mm2, mm0
		movq mm3, mm1
		punpckldq mm3, mm2
		punpckhdq mm1, mm2
		movq qword ptr [edi - 8], mm3
		movq qword ptr [ebx - 8], mm1
		jnz main_loop
		emms
	}
}

void X86_StereoMixToFloat(int *Src, float *Out1, float *Out2, UINT Count, float IntToFloat)
{
	__asm
	{
		mov esi, Src
		mov edi, Out1
		mov ebx, Out2
		mov ecx, Count
		fld IntToFloat
main_loop:
		fild dword ptr [esi + 0]
		fild dword ptr [esi + 4]
		add ebx, 4
		add edi, 4
		fmul st(0), st(2)
		add esi, 8
		fstp dword ptr [ebx - 4]
		fmul st(0), st(1)
		fstp dword ptr [edi - 4]
		dec ecx
		jnz main_loop
		fstp st(0)
	}
}

void AMD_FloatToStereoMix(float *In1, float *In2, int *_Out, UINT Count, float FloatToInt)
{
	__asm
	{
		movd mm0, FloatToInt
		mov eax, In1
		mov ebx, In2
		mov edx, _Out
		mov ecx, Count
		punpckldq mm0, mm0
		inc ecx
		shr ecx, 1
		sub edx, 16
main_loop:
		movq mm1, [eax]
		movq mm2, [ebx]
		movq mm3, mm1
		punpckldq mm1, mm2
		punpckhdq mm3, mm2
		pfmul mm1, mm0
		pfmul mm3, mm0
		pf2id mm1, mm1
		pf2id mm3, mm3
		add edx, 16
		add eax, 8
		add ebx, 8
		dec ecx
		movq qword ptr [edx + 0], mm1
		movq qword ptr [edx + 8], mm3
		jnz main_loop
		emms
	}
}

void X86_FloatToStereoMix(float *In1, float *In2, int *_Out, UINT Count, float FloatToInt)
{
	__asm
	{
		mov esi, In1
		mov ebx, In2
		mov edi, _Out
		mov ecx, Count
		fld FloatToInt
main_loop:
		fld dword ptr [ebx]
		add edi, 8
		fld dword ptr [esi]
		add ebx, 4
		add esi, 4
		fmul st(0), st(2)
		fistp dword ptr [edi - 8]
		fmul st(0), st(1)
		fistp dword ptr [edi - 4]
		dec ecx
		jnz main_loop
		fstp st(0)
	}
}

void __cdecl X86_StereoDCRemoval(int *Buffer, UINT Samples)
{
	int y1l = DCRFlt_Y1l, x1l = DCRFlt_X1l;
	int y1r = DCRFlt_Y1r, x1r = DCRFlt_X1r;
	__asm
	{
		mov esi, Buffer
		mov ecx, Samples
stereo_dcr:
		mov eax, [esi]
		mov ebx, x1l
		mov edx, [esi + 4]
		mov edi, x1r
		add esi, 8
		sub ebx, eax
		mov x1l, eax
		mov eax, ebx
		sar eax, DCR_AMOUNT + 1
		sub edi, edx
		sub eax, ebx
		mov x1r, edx
		add eax, y1l
		mov edx, edi
		sar edx, DCR_AMOUNT + 1
		mov [esi - 8], eax
		sub edx, edi
		mov ebx, eax
		add edx, y1r
		sar ebx, DCR_AMOUNT
		mov [esi - 4], edx
		mov edi, edx
		sub eax, ebx
		sar edi, DCR_AMOUNT
		mov y1l, eax
		sub edx, edi
		dec ecx
		mov y1r, edx
		jnz stereo_dcr
	}
	DCRFlt_Y1l = y1l;
	DCRFlt_X1l = x1l;
	DCRFlt_Y1r = y1r;
	DCRFlt_X1r = x1r;
}

void __cdecl X86_MonoDCRemoval(int *Buffer, UINT Samples)
{
	__asm
	{
		mov esi, Buffer
		mov ecx, Samples
		mov edx, DCRFlt_X1l
		mov edi, DCRFlt_Y1l
stereo_dcr:
		mov eax, [esi]
		mov ebx, edx
		add esi, 4
		sub ebx, eax
		mov edx, eax
		mov eax, ebx
		sar eax, DCR_AMOUNT + 1
		sub eax, ebx
		add eax, edi
		mov [esi - 4], eax
		mov ebx, eax
		sar ebx, DCR_AMOUNT
		sub eax, ebx
		dec ecx
		mov edi, eax
		jnz stereo_dcr
		mov DCRFlt_X1l, edx
		mov DCRFlt_Y1l, edi
	}
}

UINT __cdecl X86_AGC(int *Buffer, UINT Samples, UINT AGC)
{
	UINT result;
	__asm
	{
		mov esi, Buffer
		mov ecx, Samples
		mov edi, AGC
agc_loop:
		mov eax, dword ptr [esi]
		imul edi
		shrd eax, edx, AGC_PRECISION
		add esi, 4
		cmp eax, -0x08100000
		jl agc_update
		cmp eax, 0x08100000
		jg agc_update
agc_recover:
		dec ecx
		mov dword ptr [esi - 4], eax
		jnz agc_loop
		jmp done
agc_update:
		dec edi
		jmp agc_recover
done:
		mov result, edi
	}
	return result;
}

void __cdecl X86_InterleaveFrontRear(int *FrontBuf, int *RearBuf, DWORD Samples)
{
	__asm
	{
		mov ecx, Samples
		mov esi, FrontBuf
		mov edi, RearBuf
		lea esi, [esi + ecx * 4]
		lea edi, [edi + ecx * 4]
		lea ebx, [esi + ecx * 4]
		push ebp
interleave_loop:
		mov eax, dword ptr [esi - 8]
		mov edx, dword ptr [esi - 4]
		sub ebx, 16
		mov ebp, dword ptr [edi - 8]
		mov dword ptr [ebx + 0], eax
		mov dword ptr [ebx + 4], edx
		mov eax, dword ptr [edi - 4]
		sub esi, 8
		sub edi, 8
		dec ecx
		mov dword ptr [ebx + 8], ebp
		mov dword ptr [ebx + 12], eax
		jnz interleave_loop
		pop ebp
	}
}

void __cdecl X86_Dither(int *Buffer, UINT Samples, UINT Bits)
{
	static int DitherA, DitherB;
	__asm
	{
		mov esi, Buffer
		mov eax, Samples
		mov ecx, Bits
		mov edi, DitherA
		mov ebx, DitherB
		add ecx, MIXING_ATTENUATION + 1
		push ebp
		mov ebp, eax
noise_loop:
		rol edi, 1
		mov eax, dword ptr [esi]
		xor edi, 0x10204080
		add esi, 4
		lea edi, [ebx * 4 + edi + 0x78649E7D]
		mov edx, edi
		rol edx, 16
		lea edx, [edx * 4 + edx]
		add ebx, edx
		mov edx, ebx
		sar edx, cl
		add eax, edx
		dec ebp
		mov dword ptr [esi - 4], eax
		jnz noise_loop
		pop ebp
		mov DitherA, edi
		mov DitherB, ebx
	}
}