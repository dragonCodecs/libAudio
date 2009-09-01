// Late reverberation
// Tank diffusers lengths
#define RVBDIF1L_LEN		(149*2)	// 6.8ms
#define RVBDIF1R_LEN		(223*2)	// 10.1ms
#define RVBDIF2L_LEN		(421*2)	// 19.1ms
#define RVBDIF2R_LEN		(647*2)	// 29.3ms
// Tank delay lines lengths
#define RVBDLY1L_LEN		(683*2)	// 30.9ms
#define RVBDLY1R_LEN	    (811*2) // 36.7ms
#define RVBDLY2L_LEN		(773*2)	// 35.1ms
#define RVBDLY2R_LEN	    (1013*2) // 45.9ms
// Tank delay lines mask
#define RVBDLY_MASK			2047

void X86_ReverbDryMix(int *Dry, int *Wet, int DryVol, UINT Samples)
{
	for (UINT i = 0; i < Samples; i++)
	{
		Dry[i * 2 + 0] += (Wet[i * 2 + 0] >> 4) * DryVol;
		Dry[i * 2 + 1] += (Wet[i * 2 + 1] >> 4) * DryVol;
	}
}

UINT X86_ReverbProcessPreFiltering2x(int *Wet, UINT Samples)
{
	UINT OutSamples = 0;
	int lowpass = RefDelay.Coeffs[0];
	int y1_l = LastRvbIn_yl, y1_r = LastRvbIn_yr;
	UINT n = Samples;

	if (LastInPresent == true)
	{
		int x1_l = LastRvbIn_xl, x1_r = LastRvbIn_xr;
		int x2_l = Wet[0], x2_r = Wet[1];
		x1_l = (x1_l + x2_l) >> 13;
		x1_r = (x1_r + x2_r) >> 13;
		y1_l = x1_l + (((x1_l - y1_l) * lowpass) >> 15);
		y1_r = x1_r + (((x1_r - y1_r) * lowpass) >> 15);
		Wet[0] = y1_l;
		Wet[1] = y1_r;
		Wet += 2;
		n--;
		OutSamples = 1;
		LastInPresent = false;
	}
	if ((n & 1) != 0)
	{
		n--;
		LastRvbIn_xl = Wet[n * 2];
		LastRvbIn_xr = Wet[n * 2 + 1];
		LastInPresent = true;
	}
	n >>= 1;
	for (UINT i = 0; i < n; i++)
	{
		int x1_l = Wet[i * 4 + 0];
		int x2_l = Wet[i * 4 + 2];
		int x1_r = Wet[i * 4 + 1];
		int x2_r = Wet[i * 4 + 3];
		x1_l = (x1_l + x2_l) >> 13;
		x1_r = (x1_r + x2_r) >> 13;
		y1_l = x1_l + (((x1_l - y1_l) * lowpass) >> 15);
		y1_r = x1_r + (((x1_r - y1_r) * lowpass) >> 15);
		Wet[i * 2 + 0] = y1_l;
		Wet[i * 2 + 1] = y1_r;
	}
	LastRvbIn_yl = y1_l;
	LastRvbIn_yr = y1_r;
	return OutSamples + n;
}

UINT X86_ReverbProcessPreFiltering1x(int *Wet, UINT Samples)
{
	int lowpass = RefDelay.Coeffs[0];
	int y1_l = LastRvbIn_yl, y1_r = LastRvbIn_yr;

	for (UINT i = 0; i < Samples; i++)
	{
		int x_l = Wet[i * 2 + 0] >> 12;
		int x_r = Wet[i * 2 + 1] >> 12;
		y1_l = x_l + (((x_l - y1_l) * lowpass) >> 15);
		y1_r = x_r + (((x_r - y1_r) * lowpass) >> 15);
		Wet[i * 2 + 0] = y1_l;
		Wet[i * 2 + 1] = y1_r;
	}
	LastRvbIn_yl = y1_l;
	LastRvbIn_yr = y1_r;
	return Samples;
}

void MMX_ProcessPreDelay(SWRVBREFDELAY *PreDelay, int *_In, UINT Samples)
{
	__asm
	{
		mov eax, PreDelay
		mov ecx, _In
		mov esi, Samples
		lea edi, [eax + SWRVBREFDELAY.RefDelayBuffer]
		mov ebx, dword ptr [eax + SWRVBREFDELAY.DelayPos]
		mov edx, dword ptr [eax + SWRVBREFDELAY.PreDifPos]
		movd mm6, dword ptr [eax + SWRVBREFDELAY.Coeffs]
		movd mm7, dword ptr [eax + SWRVBREFDELAY.History]
		movd mm4, dword ptr [eax + SWRVBREFDELAY.PreDifCoeffs]
		lea eax, [eax + SWRVBREFDELAY.PreDifBuffer]
		dec ebx
rvb_loop:
		movq mm0, qword ptr [ecx]
		inc ebx
		add ecx, 8
		packssdw mm0, mm0
		and ebx, SNDMIX_REFLECTIONS_DELAY_MASK
		// Low pass filter
		psubsw mm7, mm0
		pmulhw mm7, mm6
		movd mm5, dword ptr [eax + edx * 4]
		paddsw mm7, mm7
		paddsw mm7, mm0
		// Pre-Diffusion filter
		movq mm0, mm7
		inc edx
		movq mm3, mm5
		and edx, SNDMIX_PREDIFFUSION_DELAY_MASK
		pmulhw mm3, mm4
		movq mm2, mm4
		dec esi
		psubsw mm0, mm3
		pmulhw mm2, mm0
		paddsw mm2, mm5
		movd dword ptr [eax + edx * 4], mm0
		movd dword ptr [edi + ebx * 4], mm2
		jnz rvb_loop
		mov eax, PreDelay
		mov dword ptr [eax + SWRVBREFDELAY.PreDifPos], edx
		movd dword ptr [eax + SWRVBREFDELAY.History], mm7
		emms
	}
}

typedef struct _DUMMYREFARRAY
{
	SWRVBREFLECTION Ref1;
	SWRVBREFLECTION Ref2;
	SWRVBREFLECTION Ref3;
	SWRVBREFLECTION Ref4;
	SWRVBREFLECTION Ref5;
	SWRVBREFLECTION Ref6;
	SWRVBREFLECTION Ref7;
	SWRVBREFLECTION Ref8;
} DUMMYREFARRAY;

void MMX_ProcessReflections(SWRVBREFDELAY *PreDelay, short *RefOut, int *_Out, UINT Samples)
{
	__asm
	{
		push ebp
		mov edi, PreDelay
		mov eax, RefOut
		mov ebp, Samples
		push eax
		lea esi, [edi + SWRVBREFDELAY.RefDelayBuffer]
		mov eax, dword ptr [edi + SWRVBREFDELAY.DelayPos]
		lea edi, [edi + SWRVBREFDELAY.Reflections]
		mov ebx, eax
		mov ecx, eax
		mov edx, eax
		sub eax, dword ptr [edi + DUMMYREFARRAY.Ref1.Delay]
		movq mm4, qword ptr [edi + DUMMYREFARRAY.Ref1.Gains]
		sub ebx, dword ptr [edi + DUMMYREFARRAY.Ref2.Delay]
		movq mm5, qword ptr [edi + DUMMYREFARRAY.Ref2.Gains]
		sub ecx, dword ptr [edi + DUMMYREFARRAY.Ref3.Delay]
		movq mm6, qword ptr [edi + DUMMYREFARRAY.Ref3.Gains]
		sub edx, dword ptr [edi + DUMMYREFARRAY.Ref4.Delay]
		movq mm7, qword ptr [edi + DUMMYREFARRAY.Ref4.Gains]
		pop edi
		and eax, SNDMIX_REFLECTIONS_DELAY_MASK
		and ebx, SNDMIX_REFLECTIONS_DELAY_MASK
		and ecx, SNDMIX_REFLECTIONS_DELAY_MASK
		and edx, SNDMIX_REFLECTIONS_DELAY_MASK
ref_loop1:
		movd mm3, dword ptr [esi + edx * 4]
		movd mm2, dword ptr [esi + ecx * 4]
		movd mm1, dword ptr [esi + ebx * 4]
		movd mm0, dword ptr [esi + eax * 4]
		inc edx
		inc ecx
		inc ebx
		inc eax
		punpckldq mm3, mm3
		punpckldq mm2, mm2
		pmaddwd mm3, mm7
		pmaddwd mm2, mm6
		punpckldq mm1, mm1
		punpckldq mm0, mm0
		pmaddwd mm1, mm5
		pmaddwd mm0, mm4
		and eax, SNDMIX_REFLECTIONS_DELAY_MASK
		and ebx, SNDMIX_REFLECTIONS_DELAY_MASK
		and ecx, SNDMIX_REFLECTIONS_DELAY_MASK
		and edx, SNDMIX_REFLECTIONS_DELAY_MASK
		paddd mm2, mm3
		paddd mm0, mm1
		paddd mm0, mm2
		psrad mm0, 15
		packssdw mm0, mm0
		add edi, 4
		dec ebp
		movd dword ptr [edi - 4], mm0
		jnz ref_loop1
		mov edi, PreDelay
		mov eax, RefOut
		mov edx, _Out
		mov ebp, Samples
		movd mm7, dword ptr [edi + SWRVBREFDELAY.ReflectionsGain]
		pxor mm0, mm0
		push eax
		punpcklwd mm7, mm0
		mov eax, dword ptr [edi + SWRVBREFDELAY.DelayPos]
		lea edi, [edi + SWRVBREFDELAY.Reflections]
		mov ebx, eax
		mov ecx, eax
		sub eax, dword ptr [edi + DUMMYREFARRAY.Ref5.Delay]
		movq mm4, qword ptr [edi + DUMMYREFARRAY.Ref5.Gains]
		sub ebx, dword ptr [edi + DUMMYREFARRAY.Ref6.Delay]
		movq mm5, qword ptr [edi + DUMMYREFARRAY.Ref6.Gains]
		sub ecx, dword ptr [edi + DUMMYREFARRAY.Ref7.Delay]
		movq mm6, qword ptr [edi + DUMMYREFARRAY.Ref7.Gains]
		pop edi
		and ecx, SNDMIX_REFLECTIONS_DELAY_MASK
		and ebx, SNDMIX_REFLECTIONS_DELAY_MASK
		and eax, SNDMIX_REFLECTIONS_DELAY_MASK
		psrad mm7, 3
ref_loop2:
		movd mm2, dword ptr [esi + ecx * 4]
		movd mm1, dword ptr [esi + ebx * 4]
		movd mm0, dword ptr [esi + eax * 4]
		movd mm3, dword ptr [edi]
		punpckldq mm2, mm2
		punpckldq mm1, mm1
		punpckldq mm0, mm0
		pmaddwd mm2, mm6
		pmaddwd mm1, mm5
		pmaddwd mm0, mm4
		inc ecx
		inc ebx
		inc eax
		and ecx, SNDMIX_REFLECTIONS_DELAY_MASK
		and ebx, SNDMIX_REFLECTIONS_DELAY_MASK
		and eax, SNDMIX_REFLECTIONS_DELAY_MASK
		paddd mm0, mm2
		paddd mm0, mm1
		psrad mm0, 15
		packssdw mm0, mm0
		paddsw mm0, mm3
		add edi, 4
		add edx, 8
		movd dword ptr [edi - 4], mm0
		punpcklwd mm0, mm0
		pmaddwd mm0, mm7
		dec ebp
		movq qword ptr [edx - 8], mm0
		jnz ref_loop2
		pop ebp
		emms
	}
}

void MMX_ProcessLateReverb(SWLATEREVERB *Reverb, short *RefOut, int *MixOut, UINT Samples)
{
	__asm
	{
		push ebp
		mov ebx, Reverb
		mov esi, RefOut
		mov edi, MixOut
		mov ebp, Samples
		mov ecx, dword ptr [ebx + SWLATEREVERB.DelayPos]
		movq mm3, qword ptr [ebx + SWLATEREVERB.RvbOutGains]
		movq mm5, qword ptr [ebx + SWLATEREVERB.DifCoeffs]
		movq mm6, qword ptr [ebx + SWLATEREVERB.DecayLP]
		movq mm7, qword ptr [ebx + SWLATEREVERB.LPHistory]
rvb_loop:
		movd mm0, dword ptr [esi]
		sub ecx, RVBDLY2L_LEN
		lea edx, [ecx + RVBDLY2L_LEN - RVBDLY2R_LEN]
		and ecx, RVBDLY_MASK
		and edx, RVBDLY_MASK
		movd mm1, dword ptr [ebx + SWLATEREVERB.Delay2 + ecx * 4]
		movd mm2, dword ptr [ebx + SWLATEREVERB.Delay2 + edx * 4]
		add ecx, RVBDLY2L_LEN - RVBDIF1R_LEN
		and ecx, RVBDLY_MASK
		movzx eax, word ptr [ebx + SWLATEREVERB.Diffusion1 + ecx * 4 + 2]
		punpckldq mm0, mm0
		punpckldq mm1, mm2
		psraw mm0, 2
		psubsw mm7, mm1
		pmulhw mm7, mm6
		movq mm1, qword ptr [ebx + SWLATEREVERB.DecayDC]
		paddsw mm7, mm7
		paddsw mm7, mm1
		pmaddwd mm1, mm7
		add ecx, RVBDIF1R_LEN - RVBDIF1L_LEN
		add esi, 4
		and ecx, RVBDLY_MASK
		shl eax, 16
		movzx edx, word ptr [ebx + SWLATEREVERB.Diffusion1 + ecx * 4]
		add ecx, RVBDIF1L_LEN
		and ecx, RVBDLY_MASK
		or eax, edx
		psrad mm1, 15
		packssdw mm1, mm1
		paddsw mm1, mm0
		movd mm2, eax
		pmulhw mm2, mm5
		movq mm4, mm1
		movq mm0, mm5
		psubsw mm1, mm2
		movd mm2, eax
		pmulhw mm0, mm1
		movd dword ptr [ebx + SWLATEREVERB.Diffusion1 + ecx * 4], mm1
		paddsw mm0, mm2
		mov eax, ecx
		movd dword ptr [ebx + SWLATEREVERB.Delay1 + ecx * 4], mm0
		sub ecx, RVBDLY1R_LEN
		sub eax, RVBDLY1L_LEN
		and ecx, RVBDLY_MASK
		and eax, RVBDLY_MASK
		punpckldq mm0, mm0
		paddsw mm4, mm0
		movd mm0, dword ptr [ebx + SWLATEREVERB.Delay1 + ecx * 4]
		movd mm1, dword ptr [ebx + SWLATEREVERB.Delay1 + eax * 4]
		add ecx, RVBDLY1R_LEN - RVBDIF2R_LEN
		and ecx, RVBDLY_MASK
		punpckldq mm1, mm0
		movzx eax, word ptr [ebx + SWLATEREVERB.Diffusion2 + ecx * 4 + 2]
		paddsw mm4, mm1
		pmaddwd mm1, qword ptr [ebx + SWLATEREVERB.Dif2InGains]
		add ecx, RVBDIF2R_LEN - RVBDIF2L_LEN
		and ecx, RVBDLY_MASK
		psrad mm1, 15
		movzx edx, word ptr [ebx + SWLATEREVERB.Diffusion2 + ecx * 4]
		packssdw mm1, mm1
		psubsw mm4, mm1
		shl eax, 16
		or eax, edx
		add ecx, RVBDIF2L_LEN
		and ecx, RVBDLY_MASK
		movd mm2, eax
		pmulhw mm2, mm5
		movq mm0, mm5
		psubsw mm1, mm2
		movd mm2, eax
		pmulhw mm0, mm1
		movd dword ptr [ebx + SWLATEREVERB.Diffusion2 + ecx * 4], mm1
		movq mm1, qword ptr [edi]
		paddsw mm0, mm2
		paddsw mm4, mm0
		movd dword ptr [ebx + SWLATEREVERB.Delay2 + ecx * 4], mm0
		pmaddwd mm4, mm3
		inc ecx
		add edi, 8
		and ecx, RVBDLY_MASK
		paddd mm4, mm1
		dec ebp
		movq qword ptr [edi - 8], mm4
		jnz rvb_loop
		pop ebp
		movq qword ptr [ebx + SWLATEREVERB.LPHistory], mm7
		mov dword ptr [ebx + SWLATEREVERB.DelayPos], ecx
		emms
	}
}

void MMX_ReverbDCRemoval(int *Buffer, UINT Samples)
{
	__asm
	{
		movq mm4, DCRRvb_Y1
		movq mm1, DCRRvb_X1
		mov ecx, Buffer
		mov edx, Samples
stereo_dcr:
		movq mm5, qword ptr [ecx]
		add ecx, 8
		psubd mm1, mm5
		movq mm0, mm1
		psrad mm0, DCR_AMOUNT + 1
		psubd mm0, mm1
		paddd mm4, mm0
		dec edx
		movq qword ptr [ecx - 8], mm4
		movq mm0, mm4
		psrad mm0, DCR_AMOUNT
		movq mm1, mm5
		psubd mm4, mm0
		jnz stereo_dcr
		movq DCRRvb_Y1, mm4
		movq DCRRvb_X1, mm4
		emms
	}
}

void X86_ReverbProcessPostFiltering2x(int *Rvb, int *Dry, UINT Samples)
{
	UINT n0 = Samples, n;
	int x1_l = LastRvbOut_xl, x1_r = LastRvbOut_xr;

	if (LastOutPresent)
	{
		Dry[0] += x1_l;
		Dry[1] += x1_r;
		Dry += 2;
		n0--;
		LastOutPresent = false;
	}
	n = n0 >> 1;
	for (UINT i = 0; i < n; i++)
	{
		int x_l = Rvb[i * 2 + 0], x_r = Rvb[i * 2 + 1];
		Dry[i * 4 + 0] += (x_l + x1_l) >> 1;
		Dry[i * 4 + 1] += (x_r + x1_r) >> 1;
		Dry[i * 4 + 2] += x_l;
		Dry[i * 4 + 3] += x_r;
		x1_l = x_l;
		x1_r = x_r;
	}
	if ((n0 & 1) != 0)
	{
		int x_l = Rvb[n * 2 + 0], x_r = Rvb[n * 2 + 1];
		Dry[n * 4 + 0] += (x_l + x1_l) >> 1;
		Dry[n * 4 + 1] += (x_r + x1_r) >> 1;
		x1_l = x_l;
		x1_r = x_r;
		LastOutPresent = true;
	}
	LastRvbOut_xl = x1_l;
	LastRvbOut_xr = x1_r;
}

void MMX_ReverbProcessPostFiltering1x(int *Rvb, int *Dry, UINT Samples)
{
	__asm
	{
		movq mm4, DCRRvb_Y1
		movq mm1, DCRRvb_X1
		mov ebx, Dry
		mov ecx, Rvb
		mov edx, Samples
stereo_dcr:
		movq mm5, qword ptr [ecx]
		movq mm3, qword ptr [ebx]
		add ecx, 8
		psubd mm1, mm5
		add ebx, 8
		movq mm0, mm1
		psrad mm0, DCR_AMOUNT + 1
		psubd mm0, mm1
		paddd mm4, mm0
		dec edx
		paddd mm3, mm4
		movq mm0, mm4
		psrad mm0, DCR_AMOUNT
		movq mm1, mm5
		psubd mm4, mm0
		movq qword ptr [ebx - 8], mm3
		jnz stereo_dcr
		movq DCRRvb_Y1, mm4
		movq DCRRvb_X1, mm5
		emms
	}
}

void ReverbShutdown()
{
	LastInPresent = false;
	LastOutPresent = false;
	LastRvbIn_xl = LastRvbIn_xr = 0;
	LastRvbIn_yl = LastRvbIn_yr = 0;
	LastRvbOut_xl = LastRvbOut_xr = 0;
	DCRRvb_X1 = DCRRvb_Y1 = 0;

	memset(LateReverb.Diffusion1, 0x00, sizeof(LateReverb.Diffusion1));
	memset(LateReverb.Diffusion2, 0x00, sizeof(LateReverb.Diffusion2));
	memset(LateReverb.Delay1, 0x00, sizeof(LateReverb.Delay1));
	memset(LateReverb.Delay2, 0x00, sizeof(LateReverb.Delay2));
	memset(RefDelay.RefDelayBuffer, 0x00, sizeof(RefDelay.RefDelayBuffer));
	memset(RefDelay.PreDifBuffer, 0x00, sizeof(RefDelay.PreDifBuffer));
	memset(RefDelay.RefOut, 0x00, sizeof(RefDelay.RefOut));
}