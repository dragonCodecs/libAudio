// Standard mixing functions

// Begin / End loop
#define SNDMIX_BEGINSAMPLELOOP8 \
	signed char *p = NULL; \
	int *vol = NULL; \
	Pos = chn->PosLo; \
	p = (signed char *)(chn->CurrentSample + chn->Pos); \
	if ((chn->Flags & CHN_STEREO) != 0) \
		p += chn->Pos; \
	vol = Buff; \
	do \
	{

#define SNDMIX_BEGINSAMPLELOOP16 \
	signed short *p = NULL; \
	int *vol = NULL; \
	Pos = chn->PosLo; \
	p = (signed short *)(chn->CurrentSample + (chn->Pos * 2)); \
	if ((chn->Flags & CHN_STEREO) != 0) \
		p += chn->Pos; \
	vol = Buff; \
	do \
	{

#define SNDMIX_ENDSAMPLELOOP \
		Pos += chn->Inc; \
	} while (vol < BuffMax); \
	chn->Pos += Pos >> 16; \
	chn->PosLo = Pos & 0xFFFF;

// Begin / End Stereo Filter
#define MIX_BEGIN_STEREO_FILTER \
	int fy1 = chn->Filter_Y1; \
	int fy2 = chn->Filter_Y2; \
	int fy3 = chn->Filter_Y3; \
	int fy4 = chn->Filter_Y4;

#define MIX_END_STEREO_FILTER \
	chn->Filter_Y1 = fy1; \
	chn->Filter_Y2 = fy2; \
	chn->Filter_Y3 = fy3; \
	chn->Filter_Y4 = fy4;

#define SNDMIX_PROCESSSTEREOFILTER \
	int fy = (vol_l * chn->Filter_A0 + fy1 * chn->Filter_B0 + fy2 * chn->Filter_B1 + 4096) >> 13; \
	fy2 = fy1; \
	fy1 = fy - (vol_l & chn->Filter_HP); \
	vol_l = fy; \
	fy = (vol_r * chn->Filter_A0 + fy1 * chn->Filter_B0 + fy4 * chn->Filter_B1 + 4096) >> 13; \
	fy4 = fy3; \
	fy3 = fy - (vol_r & chn->Filter_HP); \
	vol_r = fy;

// Begin / End Mono Filter
#define MIX_BEGIN_FILTER \
	int fy1 = chn->Filter_Y1; \
	int fy2 = chn->Filter_Y2;

#define MIX_END_FILTER \
	chn->Filter_Y1 = fy1; \
	chn->Filter_Y2 = fy2;

#define SNDMIX_PROCESSFILTER \
	int fy = (vol_ * chn->Filter_A0 + fy1 * chn->Filter_B0 + fy2 * chn->Filter_B1 + 4096) >> 13; \
	fy2 = fy1; \
	fy1 = fy - (vol_ & chn->Filter_HP); \
	vol_ = fy;

// Begin / End interface
#define BEGIN_MIX_INTERFACE(func) \
	void __CDECL__ func (Channel *chn, int *Buff, int *BuffMax) \
	{ \
		long Pos;

#define END_MIX_INTERFACE() \
		SNDMIX_ENDSAMPLELOOP \
	}

#define BEGIN_RAMPMIX_INTERFACE(func) \
	BEGIN_MIX_INTERFACE(func) \
		long RampRightVol = chn->RampRightVol; \
		long RampLeftVol = chn->RampLeftVol;

#define END_RAMPMIX_INTERFACE() \
		SNDMIX_ENDSAMPLELOOP \
		chn->RampRightVol = RampRightVol; \
		chn->RightVol = RampRightVol >> VOLUMERAMPPRECISION; \
		chn->RampLeftVol = RampLeftVol; \
		chn->LeftVol = RampLeftVol >> VOLUMERAMPPRECISION; \
	}

#define BEGIN_MIX_STFLT_INTERFACE(func) \
	BEGIN_MIX_INTERFACE(func) \
		MIX_BEGIN_STEREO_FILTER

#define END_MIX_STFLT_INTERFACE() \
		SNDMIX_ENDSAMPLELOOP \
		MIX_END_STEREO_FILTER \
	}

#define BEGIN_RAMPMIX_STFLT_INTERFACE(func) \
	BEGIN_MIX_INTERFACE(func) \
		long RampRightVol = chn->RampRightVol; \
		long RampLeftVol = chn->RampLeftVol; \
		MIX_BEGIN_STEREO_FILTER

#define END_RAMPMIX_STFLT_INTERFACE() \
		SNDMIX_ENDSAMPLELOOP \
		MIX_END_STEREO_FILTER \
		chn->RampRightVol = RampRightVol; \
		chn->RightVol = RampRightVol >> VOLUMERAMPPRECISION; \
		chn->RampLeftVol = RampLeftVol; \
		chn->LeftVol = RampLeftVol >> VOLUMERAMPPRECISION; \
	}

#define BEGIN_MIX_FLT_INTERFACE(func) \
	BEGIN_MIX_INTERFACE(func) \
		MIX_BEGIN_FILTER

#define END_MIX_FLT_INTERFACE() \
		SNDMIX_ENDSAMPLELOOP \
		MIX_END_FILTER \
	}

#define BEGIN_RAMPMIX_FLT_INTERFACE(func) \
	BEGIN_MIX_INTERFACE(func) \
		long RampRightVol = chn->RampRightVol; \
		long RampLeftVol = chn->RampLeftVol; \
		MIX_BEGIN_FILTER

#define END_RAMPMIX_FLT_INTERFACE() \
		SNDMIX_ENDSAMPLELOOP \
		MIX_END_FILTER \
		chn->RampRightVol = RampRightVol; \
		chn->RightVol = RampRightVol >> VOLUMERAMPPRECISION; \
		chn->RampLeftVol = RampLeftVol; \
		chn->LeftVol = RampLeftVol >> VOLUMERAMPPRECISION; \
	}

#define BEGIN_FASTRAMPMIX_INTERFACE(func) \
	BEGIN_MIX_INTERFACE(func) \
		long RampRightVol = chn->RampRightVol;

#define END_FASTRAMPMIX_INTERFACE() \
		SNDMIX_ENDSAMPLELOOP \
		chn->RampRightVol = RampRightVol; \
		chn->RampLeftVol = RampRightVol; \
		chn->LeftVol = chn->RightVol = RampRightVol >> VOLUMERAMPPRECISION; \
	}

// Mono
#define SNDMIX_GETMONOVOL8NOIDO \
	int vol_ = p[Pos >> 16] << 8;

#define SNDMIX_GETMONOVOL16NOIDO \
	int vol_ = p[Pos >> 16] << 8;

#define SNDMIX_GETMONOVOL8LINEAR \
	int posHi = Pos >> 16; \
	int posLo = (Pos >> 8) & 0xFF; \
	int SrcVol = p[posHi]; \
	int DestVol = p[posHi + 1]; \
	int vol_ = (SrcVol << 8) + (posLo * (DestVol - SrcVol));

#define SNDMIX_GETMONOVOL16LINEAR \
	int posHi = Pos >> 16; \
	int posLo = (Pos >> 8) & 0xFF; \
	int SrcVol = p[posHi]; \
	int DestVol = p[posHi + 1]; \
	int vol_ = SrcVol + ((posLo * (DestVol - SrcVol)) >> 8);

#define SNDMIX_GETMONOVOL8HQSRC \
	int posHi = Pos >> 16; \
	int posLo = (Pos >> 6) & 0x03FC; \
	int vol_ = (FastSinc[posLo] * p[posHi - 1] + FastSinc[posLo + 1] * p[posHi] + \
		FastSinc[posLo + 2] * p[posHi + 1] + FastSinc[posLo + 3] * p[posHi + 2]) >> 6;

#define SNDMIX_GETMONOVOL16HQSRC \
	int posHi = Pos >> 16; \
	int posLo = (Pos >> 6) & 0x03FC; \
	int vol_ = (FastSinc[posLo] * p[posHi - 1] + FastSinc[posLo + 1]  * p[posHi] + \
		FastSinc[posLo + 2] * p[posHi + 1] + FastSinc[posLo + 3] * p[posHi + 2]) >> 14;

#define SNDMIX_GETMONOVOL8KAISER \
	int posHi = Pos >> 16; \
	short *posLo = (short *)(sinc + (Pos & 0xFFF0)); \
	int vol_ = (posLo[0] * p[posHi - 3] + posLo[1] * p[posHi - 2] + \
		posLo[2] * p[posHi - 1] + posLo[3] * p[posHi] + \
		posLo[4] * p[posHi + 1] + posLo[5] * p[posHi + 2] + \
		posLo[6] * p[posHi + 3] + posLo[7] * p[posHi + 4]) >> 6;

#define SNDMIX_GETMONOVOL16KAISER \
	int posHi = Pos >> 16; \
	short *posLo = (short *)(sinc + (Pos & 0xFFF0)); \
	int vol_ = (posLo[0] * p[posHi - 3] + posLo[1] * p[posHi - 2] + \
		posLo[2] * p[posHi - 1] + posLo[3] * p[posHi] + \
		posLo[4] * p[posHi + 1] + posLo[5] * p[posHi + 2] + \
		posLo[6] * p[posHi + 3] + posLo[7] * p[posHi + 4]) >> 14;

#define SNDMIX_GETMONOVOL8FIRFILTER \
	int posHi = Pos >> 16; \
	int posLo = Pos & 0xFFFF; \
	int firIdx = ((posLo + WFIR_FRACHALVE) >> WFIR_FRACSHIFT) & WFIR_FRACMASK; \
	int vol_ = WindowedFIR::lut[firIdx + 0] * p[posHi - 3]; \
	vol_ += WindowedFIR::lut[firIdx + 1] * p[posHi - 2]; \
	vol_ += WindowedFIR::lut[firIdx + 2] * p[posHi - 1]; \
	vol_ += WindowedFIR::lut[firIdx + 3] * p[posHi + 0]; \
	vol_ += WindowedFIR::lut[firIdx + 4] * p[posHi + 1]; \
	vol_ += WindowedFIR::lut[firIdx + 5] * p[posHi + 2]; \
	vol_ += WindowedFIR::lut[firIdx + 6] * p[posHi + 3]; \
	vol_ += WindowedFIR::lut[firIdx + 7] * p[posHi + 4]; \
	vol_ >>= WFIR_8SHIFT;

#define SNDMIX_GETMONOVOL16FIRFILTER \
	int posHi = Pos >> 16; \
	int posLo = Pos & 0xFFFF; \
	int firIdx = ((posLo + WFIR_FRACHALVE) >> WFIR_FRACSHIFT) & WFIR_FRACMASK; \
	int vol1 = WindowedFIR::lut[firIdx + 0] * p[posHi - 3]; \
	vol1 += WindowedFIR::lut[firIdx + 1] * p[posHi - 2]; \
	vol1 += WindowedFIR::lut[firIdx + 2] * p[posHi - 1]; \
	vol1 += WindowedFIR::lut[firIdx + 3] * p[posHi + 0]; \
	int vol2 = WindowedFIR::lut[firIdx + 4] * p[posHi + 1]; \
	vol2 += WindowedFIR::lut[firIdx + 5] * p[posHi + 2]; \
	vol2 += WindowedFIR::lut[firIdx + 6] * p[posHi + 3]; \
	vol2 += WindowedFIR::lut[firIdx + 7] * p[posHi + 4]; \
	int vol_ = ((vol1 >> 1) + (vol2 >> 2)) >> (WFIR_16BITSHIFT - 1);

// Stereo
#define SNDMIX_GETSTEREOVOL8NOIDO \
	int vol_l = p[(Pos >> 16) * 2] << 8; \
	int vol_r = p[(Pos >> 16) * 2 + 1] << 8;

#define SNDMIX_GETSTEREOVOL16NOIDO \
	int vol_l = p[(Pos >> 16) * 2]; \
	int vol_r = p[(Pos >> 16) * 2 + 1];

#define SNDMIX_GETSTEREOVOL8LINEAR \
	int posHi = Pos >> 16; \
	int posLo = (Pos >> 8) & 0xFF; \
	int SrcVol_l = p[posHi * 2 + 0]; \
	int vol_l = (SrcVol_l << 8) + (posLo * (p[posHi * 2 + 2] - SrcVol_l)); \
	int SrcVol_r = p[posHi * 2 + 1]; \
	int vol_r = (SrcVol_r << 8) + (posLo * (p[posHi * 2 + 3] - SrcVol_r));

#define SNDMIX_GETSTEREOVOL16LINEAR \
	int posHi = Pos >> 16; \
	int posLo = (Pos >> 8) & 0xFF; \
	int SrcVol_l = p[posHi * 2 + 0]; \
	int vol_l = SrcVol_l + ((posLo * (p[posHi * 2 + 2] - SrcVol_l)) >> 8); \
	int SrcVol_r = p[posHi * 2 + 1]; \
	int vol_r = SrcVol_r + ((posLo * (p[posHi * 2 + 3] - SrcVol_r)) >> 8);

#define SNDMIX_GETSTEREOVOL8HQSRC \
	int posHi = Pos >> 16; \
	int posLo = (Pos >> 6) & 0x03FC; \
	int vol_l = (FastSinc[posLo] * p[posHi * 2 - 2] + FastSinc[posLo + 1] * p[posHi * 2] + \
		FastSinc[posLo + 2] * p[posHi * 2 + 2] + FastSinc[posLo + 3] * p[posHi * 2 + 4]) >> 6; \
	int vol_r = (FastSinc[posLo] * p[posHi * 2 - 1] + FastSinc[posLo + 1] * p[posHi * 2 + 1] + \
		FastSinc[posLo + 2] * p[posHi * 2 + 3] + FastSinc[posLo + 3] * p[posHi * 2 + 5]) >> 6;

#define SNDMIX_GETSTEREOVOL16HQSRC \
	int posHi = Pos >> 16; \
	int posLo = (Pos >> 6) & 0x03FC; \
	int vol_l = (FastSinc[posLo] * p[posHi * 2 - 2] + FastSinc[posLo + 1] * p[posHi * 2] + \
		FastSinc[posLo + 2] * p[posHi * 2 + 2] + FastSinc[posLo + 3] * p[posHi * 2 + 4]) >> 14; \
	int vol_r = (FastSinc[posLo] * p[posHi * 2 - 1] + FastSinc[posLo + 1] * p[posHi * 2 + 1] + \
		FastSinc[posLo + 2] * p[posHi * 2 + 3] + FastSinc[posLo + 3] * p[posHi * 2 + 5]) >> 14;

#define SNDMIX_GETSTEREOVOL8KAISER \
	int posHi = Pos >> 16; \
	short *posLo = (short *)(sinc + (Pos & 0xFFF0)); \
	int vol_l = (posLo[0] * p[posHi * 2 - 6] + posLo[1] * p[posHi * 2 - 4] + \
		posLo[2] * p[posHi * 2 - 2] + posLo[3] * p[posHi * 2 + 0] + \
		posLo[4] * p[posHi * 2 + 2] + posLo[5] * p[posHi * 2 + 4] + \
		posLo[6] * p[posHi * 2 + 6] + posLo[7] * p[posHi * 2 + 8]) >> 6; \
	int vol_r = (posLo[0] * p[posHi * 2 - 5] + posLo[1] * p[posHi * 2 - 3] + \
		posLo[2] * p[posHi * 2 - 1] + posLo[3] * p[posHi * 2 + 1] + \
		posLo[4] * p[posHi * 2 + 5] + posLo[5] * p[posHi * 2 + 4] + \
		posLo[6] * p[posHi * 2 + 7] + posLo[7] * p[posHi * 2 + 9]) >> 6;

#define SNDMIX_GETSTEREOVOL16KAISER \
	int posHi = Pos >> 16; \
	short *posLo = (short *)(sinc + (Pos & 0xFFF0)); \
	int vol_l = (posLo[0] * p[posHi * 2 - 6] + posLo[1] * p[posHi * 2 - 4] + \
		posLo[2] * p[posHi * 2 - 2] + posLo[3] * p[posHi * 2 + 0] + \
		posLo[4] * p[posHi * 2 + 2] + posLo[5] * p[posHi * 2 + 4] + \
		posLo[6] * p[posHi * 2 + 6] + posLo[7] * p[posHi * 2 + 8]) >> 14; \
	int vol_r = (posLo[0] * p[posHi * 2 - 5] + posLo[1] * p[posHi * 2 - 3] + \
		posLo[2] * p[posHi * 2 - 1] + posLo[3] * p[posHi * 2 + 1] + \
		posLo[4] * p[posHi * 2 + 5] + posLo[5] * p[posHi * 2 + 4] + \
		posLo[6] * p[posHi * 2 + 7] + posLo[7] * p[posHi * 2 + 9]) >> 14;

#define SNDMIX_GETSTEREOVOL8FIRFILTER \
	int posHi = Pos >> 16; \
	int posLo = Pos & 0xFFFF; \
	int firIdx = ((posLo + WFIR_FRACHALVE) >> WFIR_FRACSHIFT) & WFIR_FRACMASK; \
	int vol_l = WindowedFIR::lut[firIdx + 0] * p[(posHi - 3) * 2 + 0]; \
	vol_l += WindowedFIR::lut[firIdx + 1] * p[(posHi - 2) * 2 + 0]; \
	vol_l += WindowedFIR::lut[firIdx + 2] * p[(posHi - 1) * 2 + 0]; \
	vol_l += WindowedFIR::lut[firIdx + 3] * p[(posHi + 0) * 2 + 0]; \
	vol_l += WindowedFIR::lut[firIdx + 4] * p[(posHi + 1) * 2 + 0]; \
	vol_l += WindowedFIR::lut[firIdx + 5] * p[(posHi + 2) * 2 + 0]; \
	vol_l += WindowedFIR::lut[firIdx + 6] * p[(posHi + 3) * 2 + 0]; \
	vol_l += WindowedFIR::lut[firIdx + 7] * p[(posHi + 4) * 2 + 0]; \
	vol_l >>= WFIR_8SHIFT; \
	int vol_r = WindowedFIR::lut[firIdx + 0] * p[(posHi - 3) * 2 + 1]; \
	vol_r += WindowedFIR::lut[firIdx + 1] * p[(posHi - 2) * 2 + 1]; \
	vol_r += WindowedFIR::lut[firIdx + 2] * p[(posHi - 1) * 2 + 1]; \
	vol_r += WindowedFIR::lut[firIdx + 3] * p[(posHi + 0) * 2 + 1]; \
	vol_r += WindowedFIR::lut[firIdx + 4] * p[(posHi + 1) * 2 + 1]; \
	vol_r += WindowedFIR::lut[firIdx + 5] * p[(posHi + 2) * 2 + 1]; \
	vol_r += WindowedFIR::lut[firIdx + 6] * p[(posHi + 3) * 2 + 1]; \
	vol_r += WindowedFIR::lut[firIdx + 7] * p[(posHi + 4) * 2 + 1]; \
	vol_r >>= WFIR_8SHIFT;

#define SNDMIX_GETSTEREOVOL16FIRFILTER \
	int posHi = Pos >> 16; \
	int posLo = Pos & 0xFFFF; \
	int firIdx = ((posLo + WFIR_FRACHALVE) >> WFIR_FRACSHIFT) & WFIR_FRACMASK; \
	int vol1_l = WindowedFIR::lut[firIdx + 0] * p[(posHi - 3) * 2 + 0]; \
	vol1_l += WindowedFIR::lut[firIdx + 1] * p[(posHi - 2) * 2 + 0]; \
	vol1_l += WindowedFIR::lut[firIdx + 2] * p[(posHi - 1) * 2 + 0]; \
	vol1_l += WindowedFIR::lut[firIdx + 3] * p[(posHi + 0) * 2 + 0]; \
	int vol2_l = WindowedFIR::lut[firIdx + 4] * p[(posHi + 1) * 2 + 0]; \
	vol2_l += WindowedFIR::lut[firIdx + 5] * p[(posHi + 2) * 2 + 0]; \
	vol2_l += WindowedFIR::lut[firIdx + 6] * p[(posHi + 3) * 2 + 0]; \
	vol2_l += WindowedFIR::lut[firIdx + 7] * p[(posHi + 4) * 2 + 0]; \
	int vol_l = ((vol1_l >> 1) + (vol2_l >> 1)) >> (WFIR_16BITSHIFT - 1); \
	int vol1_r = WindowedFIR::lut[firIdx + 0] * p[(posHi - 3) * 2 + 1]; \
	vol1_r += WindowedFIR::lut[firIdx + 1] * p[(posHi - 2) * 2 + 1]; \
	vol1_r += WindowedFIR::lut[firIdx + 2] * p[(posHi - 1) * 2 + 1]; \
	vol1_r += WindowedFIR::lut[firIdx + 3] * p[(posHi + 0) * 2 + 1]; \
	int vol2_r = WindowedFIR::lut[firIdx + 4] * p[(posHi + 1) * 2 + 1]; \
	vol2_r += WindowedFIR::lut[firIdx + 5] * p[(posHi + 2) * 2 + 1]; \
	vol2_r += WindowedFIR::lut[firIdx + 6] * p[(posHi + 3) * 2 + 1]; \
	vol2_r += WindowedFIR::lut[firIdx + 7] * p[(posHi + 4) * 2 + 1]; \
	int vol_r = ((vol1_r >> 1) + (vol2_r >> 1)) >> (WFIR_16BITSHIFT - 1);

// vol
#define SNDMIX_STOREMONOVOL \
	vol[0] += vol_ * chn->RightVol; \
	vol[1] += vol_ * chn->LeftVol; \
	vol += 2;

#define SNDMIX_STORESTEREOVOL \
	vol[0] += vol_l * chn->RightVol; \
	vol[1] += vol_r * chn->LeftVol; \
	vol += 2;

#define SNDMIX_STOREFASTMONOVOL \
	int v = vol_ * chn->RightVol; \
	vol[0] += v; \
	vol[1] += v; \
	vol += 2;

#define SNDMIX_RAMPMONOVOL \
	RampLeftVol += chn->LeftRamp; \
	RampRightVol += chn->RightRamp; \
	vol[0] += vol_ * (RampRightVol >> VOLUMERAMPPRECISION); \
	vol[1] += vol_ * (RampLeftVol >> VOLUMERAMPPRECISION); \
	vol += 2;

#define SNDMIX_RAMPSTEREOVOL \
	RampLeftVol += chn->LeftRamp; \
	RampRightVol += chn->RightRamp; \
	vol[0] += vol_l * (RampRightVol >> VOLUMERAMPPRECISION); \
	vol[1] += vol_r * (RampLeftVol >> VOLUMERAMPPRECISION); \
	vol += 2;

#define SNDMIX_RAMPFASTMONOVOL \
	RampRightVol += chn->RightRamp; \
	int fastvol = vol_ * (RampRightVol >> VOLUMERAMPPRECISION); \
	vol[0] += fastvol; \
	vol[1] += fastvol; \
	vol += 2;

// sinc
#define SNDMIX_INITSINCTABLE \
	char *sinc = (char *)(chn->Inc > 0x13000 || chn->Inc < -0x13000 ? (chn->Inc > 0x18000 || chn->Inc < -0x18000 ? \
	DownSample2x : DownSample13x) : KaiserSinc);

// Interfaces
// Mono
BEGIN_MIX_INTERFACE(Mono8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono8BitHQMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8HQSRC
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitHQMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16HQSRC
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8HQSRC
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16HQSRC
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono8BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8KAISER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16KAISER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8KAISER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16KAISER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono8BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16FIRFILTER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16FIRFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

// Stereo
BEGIN_MIX_INTERFACE(Stereo8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8NOIDO
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16NOIDO
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8NOIDO
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16NOIDO
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8LINEAR
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16LINEAR
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8LINEAR
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16LINEAR
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo8BitHQMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8HQSRC
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitHQMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16HQSRC
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo8BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8HQSRC
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16HQSRC
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo8BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8KAISER
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16KAISER
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo8BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8KAISER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16KAISER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo8BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16FIRFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo8BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16FIRFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

// Filter Intefaces
// Mono
BEGIN_MIX_FLT_INTERFACE(FilterMono8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono8BitHQMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8HQSRC
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitHQMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16HQSRC
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8HQSRC
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16HQSRC
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono8BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8KAISER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16KAISER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8KAISER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16KAISER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono8BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16FIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16FIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_MIX_FLT_INTERFACE()

// Stereo
BEGIN_MIX_STFLT_INTERFACE(FilterStereo8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8NOIDO
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16NOIDO
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8NOIDO
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16NOIDO
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8LINEAR
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16LINEAR
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8LINEAR
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16LINEAR
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo8BitHQMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8HQSRC
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitHQMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16HQSRC
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo8BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8HQSRC
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FitlerStereo16BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16HQSRC
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo8BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8KAISER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16KAISER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo8BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8KAISER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo16BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16KAISER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo8BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16FIRFILTER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo8BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo16BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16FIRFILTER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

// Fast Mono
BEGIN_MIX_INTERFACE(FastMono8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(FastMono16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(FastMono8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(FastMono16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE(FastMono8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE(FastMono16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE(FastMono8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE(FastMono16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()
