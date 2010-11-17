#define _USE_MATH_DEFINES
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "../ProTracker.h"
#include "moduleMixer.h"

#ifndef _WINDOWS
inline int abs(int x)
{
	if (x < 0)
		return -x;
	return x;
}
#endif

#define SNG_ENDREACHED		1

#define CHN_LOOP			0x01
#define CHN_TREMOLO			0x02
#define CHN_ARPEGGIO		0x04
#define CHN_VIBRATO			0x08
#define CHN_VOLUMERAMP		0x10
#define CHN_FASTVOLRAMP		0x20

#define CMD_ARPEGGIO		0
#define CMD_PORTAMENTOUP	1
#define CMD_PORTAMENTODOWN	2
#define CMD_TONEPORTAMENTO	3
#define CMD_VIBRATO			4
#define CMD_TONEPORTAVOL	5
#define CMD_VIBRATOVOL		6
#define CMD_TREMOLO			7
#define CMD_PANNING			8
#define CMD_OFFSET			9
#define CMD_VOLUMESLIDE		10
#define CMD_POSITIONJUMP	11
#define CMD_VOLUME			12
#define CMD_PATTERNBREAK	13
#define CMD_EXTENDED		14
#define CMD_SPEED			15

#define CMDEX_FILTER		0
#define CMDEX_FINEPORTAUP	1
#define CMDEX_FINEPORTADOWN	2
#define CMDEX_GLISSANDO		3
#define CMDEX_VIBRATOWAVE	4
#define CMDEX_FINETUNE		5
#define CMDEX_LOOP			6
#define CMDEX_TREMOLOWAVE	7
#define CMDEX_UNUSED		8
#define CMDEX_RETRIGER		9
#define CMDEX_FINEVOLUP		10
#define CMDEX_FINEVOLDOWN	11
#define CMDEX_CUT			12
#define CMDEX_DELAYSAMP		13
#define CMDEX_DELAYPAT		14
#define CMDEX_INVERTLOOP	15

#define SNDMIX_SPLINESRCMODE	0x01
#define SNDMIX_POLYPHASESRCMODE	0x02
#define SNDMIX_FIRFILTERSRCMODE	0x04

#define MIXNDX_FILTER		0x01
#define MIXNDX_LINEARSRC	0x02
#define MIXNDX_HQSRC		0x04
#define MIXNDX_KAISERSRC	0x06
#define MIXNDX_FIRFILTERSRC	0x08
#define MIXNDX_RAMP			0x10

#define MAX_VOLUME			64
#define MIXBUFFERSIZE 512

typedef struct _Channel
{
	BYTE *Sample, *NewSample;
	BYTE Note, NewNote, NewSamp;
	UINT LoopStart, LoopEnd, Length;
	MODSample *Samp;
	BYTE RowNote, RowSample, Volume;
	BYTE FineTune, Flags, Pan;
	UINT Period, Pos, PosLo;
	int Increment;
	BYTE RetrigCount, RetrigParam;
	BYTE LeftVol, RightVol, RampLength;
	BYTE NewLeftVol, NewRightVol;
	WORD RowEffect;
	short LeftRamp, RightRamp;
	int Filter_Y1, Filter_Y2, Filter_Y3, Filter_Y4;
	int Filter_A0, Filter_B0, Filter_B1, Filter_HP;
} Channel;

typedef struct _MixerState
{
	UINT MixRate, MixOutChannels, MixBitsPerSample;
	UINT Channels, Samples;
	UINT TickCount, BufferCount;
	UINT Row, NextRow;
	UINT MusicSpeed, MusicTempo;
	UINT Pattern, CurrentPattern, NextPattern, RestartPos;
	BOOL PatternTransitionOccured;
	BYTE *Orders, MaxOrder, PatternDelay;
	MODSample *Samp;
	BYTE **SamplePCM;
	UINT RowsPerBeat, SamplesPerTick;
	Channel *Chns;
	UINT MixChannels, *ChnMix;
	MODPattern *Patterns;
	BYTE SongFlags, SoundSetup;
	BYTE PatternLoopCount, PatternLoopStart;
	int MixBuffer[MIXBUFFERSIZE * 4];
} MixerState;

WORD Periods[60] =
{
	1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017, 961, 907,
	856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
	428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
	107, 101, 95, 90, 85, 80, 76, 71, 67, 64, 60, 57
};

WORD TunedPeriods[192] = 
{
	// 0 to 7:
	1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960,907,
	1700,1604,1514,1430,1348,1274,1202,1134,1070,1010,954,900,
	1688,1592,1504,1418,1340,1264,1194,1126,1064,1004,948,894,
	1676,1582,1492,1408,1330,1256,1184,1118,1056,996,940,888,
	1664,1570,1482,1398,1320,1246,1176,1110,1048,990,934,882,
	1652,1558,1472,1388,1310,1238,1168,1102,1040,982,926,874,
	1640,1548,1460,1378,1302,1228,1160,1094,1032,974,920,868,
	1628,1536,1450,1368,1292,1220,1150,1086,1026,968,914,862,
	// -8 to -1:
	1814,1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960,
	1800,1700,1604,1514,1430,1350,1272,1202,1134,1070,1010,954,
	1788,1688,1592,1504,1418,1340,1264,1194,1126,1064,1004,948,
	1774,1676,1582,1492,1408,1330,1256,1184,1118,1056,996,940,
	1762,1664,1570,1482,1398,1320,1246,1176,1110,1048,988,934,
	1750,1652,1558,1472,1388,1310,1238,1168,1102,1040,982,926,
	1736,1640,1548,1460,1378,1302,1228,1160,1094,1032,974,920,
	1724,1628,1536,1450,1368,1292,1220,1150,1086,1026,968,914 
};

const BYTE PreAmpTable[16] =
{
	0x60, 0x60, 0x60, 0x70,
	0x80, 0x88, 0x90, 0x98,
	0xA0, 0xA4, 0xA8, 0xAC,
	0xB0, 0xB4, 0xB8, 0xBC
};

#include "mixFunctions.h"

// BEGIN: Code from OMPT 1.17RC2
// Credit for this code goes to Oliver Lapicque
typedef void (__CDECL__ *MixInterface)(Channel *, int *, int *);

MixInterface MixFunctionTable[32] = 
{
	// Non ramping functions
	Mono8BitMix, FilterMono8BitMix, Mono8BitLinearMix, FilterMono8BitLinearMix,
	Mono8BitHQMix, FilterMono8BitLinearMix, Mono8BitKaiserMix, FilterMono8BitKaiserMix,
	Mono8BitFIRFilterMix, FilterMono8BitFIRFilterMix, NULL, NULL,
	NULL, NULL, NULL, NULL,
	// Ramping functions
	Mono8BitRampMix, FilterMono8BitRampMix, Mono8BitLinearRampMix, FilterMono8BitLinearRampMix,
	Mono8BitHQRampMix, FilterMono8BitLinearRampMix, Mono8BitKaiserRampMix, FilterMono8BitKaiserRampMix,
	Mono8BitFIRFilterRampMix, FilterMono8BitFIRFilterRampMix, NULL, NULL,
	NULL, NULL, NULL, NULL,
};
// END: Code from OMPT 1.17RC2

// Return (a * b) / c [ - no divide error ]
int muldiv(long a, long b, long c)
{
	int sign, result;
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
		mov sign, edx
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
		mov edx, sign
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
		"\tmov edi, edx\n"
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
		"\tmov edx, edi\n"
		"\tor edx, edx\n"
		"\tjge r_neg\n"
		"\tneg eax\n"
		"r_neg:\n"
		".att_syntax\n" : [sign] "=D" (sign), [result] "=a" (result) : [a] "a" (a),
		[b] "b" (b), [c] "c" (c), "D" (sign) : "edx");
#endif
	return result;
}

UINT __CDECL__ Convert32to16(void *_out, int *_in, UINT SampleCount)
{
	UINT result;
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
		"\tpush edi\n"
		"cliploop:\n"
		"\tmov eax, dword ptr [edx]\n"
		"\tadd ebx, 2\n"
		"\tadd eax, (1 << 11)\n"
		"\tadd edx, 4\n"
		"\tcmp eax, (-0x07FFFFFF)\n"
		"\tjl cliplow\n"
		"\tcmp eax, (0x07FFFFFF)\n"
		"\tjg cliphigh\n"
		"cliprecover:\n"
		"\tsar eax, 12\n"
		"\tdec edi\n"
		"\tmov word ptr [ebx - 2], ax\n"
		"\tjnz cliploop\n"
		"\tjmp done\n"
		"cliplow:\n"
		"\tmov eax, (-0x07FFFFFF)\n"
		"\tjmp cliprecover\n"
		"cliphigh:\n"
		"\tmov eax, (0x07FFFFFF)\n"
		"\tjmp cliprecover\n"
		"done:\n"
		"\tpop eax\n"
		"\tadd eax, eax\n"
		".att_syntax\n" : [result] "=a" (result) : [_out] "b" (_out), 
		[_in] "d" (_in), [SampleCount] "D" (SampleCount) : );
#endif
	return result;
}

void ResetChannelPanning(MixerState *p_Mixer)
{
	BYTE i;
	Channel *chn = p_Mixer->Chns;
	for (i = 0; i < p_Mixer->Channels; i++, chn++)
	{
		BYTE j = i % 4;
		if (j == 0 || j == 3)
			chn->Pan = 0;
		else
			chn->Pan = 128;
	}
}

void CreateMixer(MOD_Intern *p_MF)
{
	MixerState *p_Mixer = NULL;
	p_Mixer = (MixerState *)malloc(sizeof(MixerState));
	if (p_Mixer == NULL)
		return;
	memset(p_Mixer, 0x00, sizeof(MixerState));
	p_MF->p_Mixer = p_Mixer;

	p_Mixer->Channels = p_MF->nChannels;
	p_Mixer->Samples = p_MF->nSamples;
	p_Mixer->MaxOrder = p_MF->p_Header->nOrders;
	p_Mixer->Orders = p_MF->p_Header->Orders;
	p_Mixer->Patterns = p_MF->p_Patterns;
	p_Mixer->MusicSpeed = 6;
	p_Mixer->MusicTempo = 125;
	p_Mixer->TickCount = p_Mixer->MusicSpeed;
	p_Mixer->Samp = p_MF->p_Samples;
	p_Mixer->MixRate = p_MF->p_FI->BitRate;
	p_Mixer->MixOutChannels = p_MF->p_FI->Channels;
	p_Mixer->MixBitsPerSample = p_MF->p_FI->BitsPerSample;
	p_Mixer->SamplesPerTick = (p_Mixer->MixRate * 640) / (p_Mixer->MusicTempo << 8);
	p_Mixer->RestartPos = p_MF->p_Header->RestartPos;
	if (p_Mixer->RestartPos > p_Mixer->MaxOrder)
		p_Mixer->RestartPos = 0;
	p_Mixer->SamplePCM = p_MF->p_PCM;
	p_Mixer->Chns = (Channel *)malloc(sizeof(Channel) * p_Mixer->Channels);
	p_Mixer->ChnMix = (UINT *)malloc(sizeof(UINT) * p_Mixer->Channels);
	memset(p_Mixer->Chns, 0x00, sizeof(Channel) * p_Mixer->Channels);
	memset(p_Mixer->ChnMix, 0x00, sizeof(UINT) * p_Mixer->Channels);
	ResetChannelPanning(p_Mixer);
	InitialiseTables();
}

void DestroyMixer(void *Mixer)
{
	MixerState *p_Mixer = (MixerState *)Mixer;
	if (p_Mixer == NULL)
		return;
	free(p_Mixer->Chns);
	free(p_Mixer->ChnMix);
	free(p_Mixer);
}

inline BYTE PeriodToNoteIndex(WORD Period)
{
	BYTE i, min = 0, max = 59;
	do
	{
		i = min + ((max - min) / 2);
		if (Periods[i] == Period)
			return i;
		else if (Periods[i] < Period)
		{
			if (i > 0)
			{
				UINT Dist1 = Period - Periods[i];
				UINT Dist2 = abs(Periods[i - 1] - Period);
				if (Dist1 < Dist2)
					return i;
			}
			max = i - 1;
		}
		else if (Periods[i] > Period)
		{
			if (i < 59)
			{
				UINT Dist1 = Periods[i] - Period;
				UINT Dist2 = abs(Period - Periods[i + 1]);
				if (Dist1 < Dist2)
					return i;
			}
			min = i + 1;
		}
	}
	while(min < max);
	if (min == max)
		return min;
	else
		return -1;
}

void SampleChange(MixerState *p_Mixer, Channel *chn, UINT sample_)
{
	MODSample *smp;
	UINT note, sample = sample_ - 1;

	smp = &p_Mixer->Samp[sample];
	note = chn->NewNote;
	chn->Volume = smp->Volume;
	chn->NewSamp = 0;
	chn->Samp = smp;
	chn->Length = smp->Length * 2;
	chn->LoopStart = (smp->LoopStart < smp->Length ? smp->LoopStart : smp->Length) * 2;
	chn->LoopEnd = (smp->LoopLen > 1 ? chn->LoopStart + (smp->LoopLen * 2) : 0);
	if (chn->LoopEnd != 0)
		chn->Flags |= CHN_LOOP;
	else
		chn->Flags &= ~CHN_LOOP;
	chn->NewSample = p_Mixer->SamplePCM[sample];
	chn->FineTune = smp->FineTune;
	if (chn->LoopEnd > chn->Length)
		chn->LoopEnd = chn->Length;
}

UINT GetPeriodFromNote(BYTE Note, BYTE FineTune)
{
	if (Note == 0xFF)
		return 0;
	if (FineTune != 0)
		return (TunedPeriods[(FineTune * 12) + (Note % 12)] << 2) >> (Note / 12);
	else
		return Periods[Note] << 2;
}

void NoteChange(MixerState *p_Mixer, UINT nChn, BYTE note)
{
	Channel * const chn = &p_Mixer->Chns[nChn];
	MODSample *smp = chn->Samp;
	UINT period;

	chn->Note = note;
	chn->NewSamp = 0;
	period = GetPeriodFromNote(note, chn->FineTune);
	if (smp == NULL)
		return;
	if (period != 0)
	{
		chn->Period = period;
		if (chn->Length == 0)
		{
			chn->Samp = smp;
			chn->NewSample = p_Mixer->SamplePCM[smp - p_Mixer->Samp];
			chn->Length = smp->Length * 2;
			if (smp->LoopLen > 1)
			{
				chn->Flags |= CHN_LOOP;
				chn->LoopStart = (smp->LoopStart < smp->Length ? smp->LoopStart : smp->Length) * 2;
				chn->LoopEnd = (smp->LoopLen > 1 ? chn->LoopStart + (smp->LoopLen * 2) : 0);
			}
			else
			{
				chn->Flags &= ~CHN_LOOP;
				chn->LoopStart = 0;
				chn->LoopEnd = smp->Length * 2;
			}
			chn->Pos = 0;
			chn->PosLo = 0;
		}
		if (chn->Pos > chn->Length)
			chn->Pos = chn->Length;
	}
	chn->RetrigCount = 0;
	chn->Flags |= CHN_FASTVOLRAMP;
	chn->LeftVol = chn->RightVol = 0;
}

void ProcessExtendedCommand(MixerState *p_Mixer, Channel *chn, BYTE param)
{
	BYTE cmd = (param & 0xF0) >> 8;
	param &= 0x0F;
	switch (cmd)
	{
		case CMDEX_FILTER:
		{
			break;
		}
		case CMDEX_FINEPORTAUP:
		{
			break;
		}
		case CMDEX_FINEPORTADOWN:
		{
			break;
		}
		case CMDEX_GLISSANDO:
		{
			break;
		}
		case CMDEX_VIBRATOWAVE:
		{
			break;
		}
		case CMDEX_FINETUNE:
		{
			break;
		}
		case CMDEX_TREMOLOWAVE:
		{
			break;
		}
		case CMDEX_RETRIGER:
		{
			break;
		}
		case CMDEX_FINEVOLUP:
		{
			break;
		}
		case CMDEX_FINEVOLDOWN:
		{
			break;
		}
		case CMDEX_CUT:
		{
			break;
		}
		case CMDEX_INVERTLOOP:
		{
			break;
		}
	}
}

int PatternLoop(MixerState *p_Mixer, UINT param)
{
	if (param != 0)
	{
		if (p_Mixer->PatternLoopCount != 0)
		{
			p_Mixer->PatternLoopCount--;
			if (p_Mixer->PatternLoopCount == 0)
			{
				// Reset the default start position for the next
				// CMDEX_LOOP
				p_Mixer->PatternLoopStart = 0;
				return -1;
			}
		}
		else
			p_Mixer->PatternLoopCount = param;
		return p_Mixer->PatternLoopStart;
	}
	else
		p_Mixer->PatternLoopStart = p_Mixer->Row;
	return -1;
}

void VolumeSlide(MixerState *p_Mixer, Channel *chn, BYTE param)
{
	if (p_Mixer->TickCount != 0)
	{
		short NewVolume = chn->Volume;
		if ((param & 0xF0) != 0)
			NewVolume += (short)((param & 0xF0) >> 8);
		else
			NewVolume -= (short)(param & 0x0F);
		chn->Flags |= CHN_FASTVOLRAMP;
		if (NewVolume < 0)
			NewVolume = 0;
		else if (NewVolume > 64)
			NewVolume = 64;
		chn->Volume = (BYTE)NewVolume;
	}
}

BOOL ProcessEffects(MixerState *p_Mixer)
{
	int PositionJump = -1, BreakRow = -1, PatternLoopRow = -1;
	Channel *chn = p_Mixer->Chns;
	UINT i;
	for (i = 0; i < p_Mixer->Channels; i++, chn++)
	{
		BYTE sample = chn->RowSample;
		BYTE cmd = (chn->RowEffect & 0xF00) >> 8;
		BYTE param = (chn->RowEffect & 0xFF);
		UINT StartTick = 0;

		chn->Flags &= ~CHN_FASTVOLRAMP;
		if (cmd == CMD_EXTENDED)
		{
			BYTE excmd = (param & 0xF0) >> 4;
			if (excmd == CMDEX_DELAYSAMP)
				StartTick = param & 0x0F;
			else if (p_Mixer->TickCount == 0)
			{
				if (excmd == CMDEX_LOOP)
				{
					int loop = PatternLoop(p_Mixer, param & 0x0F);
					if (loop >= 0)
						PatternLoopRow = loop;
				}
				else if (excmd == CMDEX_DELAYPAT)
					p_Mixer->PatternDelay = param & 0x0F;
			}
		}

		if (p_Mixer->TickCount == StartTick)
		{
			BYTE note = chn->RowNote;
			if (sample != 0)
				chn->NewSamp = sample;
			if (note == -1 && sample != 0)
				chn->Volume = p_Mixer->Samp[sample - 1].Volume;
			chn->NewNote = note;
			if (sample != 0)
			{
				MODSample *smp = chn->Samp;
				SampleChange(p_Mixer, chn, sample);
				chn->NewSamp = 0;
			}
			if (note != 0xFF)
			{
				if (sample == 0 && chn->NewSamp != 0)
				{
					SampleChange(p_Mixer, chn, chn->NewSamp);
					chn->NewSamp = 0;
				}
				NoteChange(p_Mixer, i, note);
			}
		}
		if (cmd != 0 || (cmd == 0 && param != 0))
		{
			switch (cmd)
			{
				case CMD_ARPEGGIO:
				{
					break;
				}
				case CMD_PORTAMENTOUP:
				{
					break;
				}
				case CMD_PORTAMENTODOWN:
				{
					break;
				}
				case CMD_TONEPORTAMENTO:
				{
					break;
				}
				case CMD_VIBRATO:
				{
					break;
				}
				case CMD_TONEPORTAVOL:
				{
					break;
				}
				case CMD_VIBRATOVOL:
				{
					break;
				}
				case CMD_TREMOLO:
				{
					break;
				}
				case CMD_PANNING:
				{
					if (p_Mixer->TickCount != 0)
						break;
					chn->Pan = param;
					chn->Flags |= CHN_FASTVOLRAMP;
					break;
				}
				case CMD_OFFSET:
				{
					break;
				}
				case CMD_VOLUMESLIDE:
				{
					if (param != 0)
						VolumeSlide(p_Mixer, chn, param);
					break;
				}
				case CMD_POSITIONJUMP:
				{
					PositionJump = param;
					if (PositionJump > p_Mixer->MaxOrder)
						PositionJump = 0;
					break;
				}
				case CMD_VOLUME:
				{
					if (p_Mixer->TickCount == 0)
					{
						BYTE NewVolume = param;
						if (NewVolume > 64)
							NewVolume = 64;
						chn->Volume = NewVolume;
						chn->Flags |= CHN_FASTVOLRAMP;
					}
					break;
				}
				case CMD_PATTERNBREAK:
				{
					BreakRow = param;
					BreakRow = ((BreakRow >> 8) * 10) + (BreakRow & 0x0F);
					if (BreakRow > 63)
						BreakRow = 63;
					break;
				}
				case CMD_EXTENDED:
				{
					ProcessExtendedCommand(p_Mixer, chn, param);
					break;
				}
				case CMD_SPEED:
				{
					/* Speed rules (as per http://www.aes.id.au/modformat.html
					 * NewSpeed == 0 => NewSpeed = 1
					 * NewSpeed <= 32 => Speed = NewSpeed (TPR)
					 * NewSpeed > 32 => Tempo = NewSpeed (BPM)
					 */
					BYTE NewSpeed = param;
					if (NewSpeed == 0)
						NewSpeed = 1;
					if (NewSpeed <= 32)
						p_Mixer->MusicSpeed = NewSpeed;
					else
						p_Mixer->MusicTempo = NewSpeed;
					break;
				}
			}
		}
	}
	if (p_Mixer->TickCount == 0)
	{
		if (PatternLoopRow >= 0)
		{
			p_Mixer->NextPattern = p_Mixer->CurrentPattern;
			p_Mixer->NextRow = PatternLoopRow;
			if (p_Mixer->PatternDelay != 0)
				p_Mixer->NextRow++;
		}
		else if (BreakRow >= 0 || PositionJump >= 0)
		{
			BOOL Jump = TRUE;
			if (PositionJump < 0)
				PositionJump = p_Mixer->CurrentPattern + 1;
			if (BreakRow < 0)
				BreakRow = 0;
			if ((UINT)PositionJump < p_Mixer->CurrentPattern || ((UINT)PositionJump == p_Mixer->CurrentPattern && (UINT)BreakRow <= p_Mixer->Row))
				Jump = FALSE;
			if (Jump == TRUE && (PositionJump != p_Mixer->CurrentPattern || BreakRow != p_Mixer->Row))
			{
				if (PositionJump != p_Mixer->CurrentPattern)
					p_Mixer->PatternLoopCount = p_Mixer->PatternLoopStart = 0;
				p_Mixer->NextPattern = PositionJump;
				p_Mixer->NextRow = BreakRow;
				p_Mixer->PatternTransitionOccured = TRUE;
			}
		}
	}
	return TRUE;
}

BOOL ProcessRow(MixerState *p_Mixer)
{
	p_Mixer->TickCount++;
	if (p_Mixer->TickCount >= p_Mixer->MusicSpeed * (p_Mixer->PatternDelay + 1))
	{
		UINT i;
		MODCommand (*Commands)[64];
		Channel *chn = p_Mixer->Chns;
		p_Mixer->TickCount = 0;
		p_Mixer->Row = p_Mixer->NextRow;
		if (p_Mixer->CurrentPattern != p_Mixer->NextPattern)
			p_Mixer->CurrentPattern = p_Mixer->NextPattern;
		if (p_Mixer->CurrentPattern >= p_Mixer->MaxOrder)
			return FALSE;
		p_Mixer->Pattern = p_Mixer->Orders[p_Mixer->CurrentPattern];
		p_Mixer->NextPattern = p_Mixer->CurrentPattern;
		if (p_Mixer->Row >= 64)
			p_Mixer->Row = 0;
		p_Mixer->NextRow = p_Mixer->Row + 1;
		if (p_Mixer->NextRow >= 64)
		{
			p_Mixer->NextPattern = p_Mixer->CurrentPattern + 1;
			p_Mixer->NextRow = 0;
			p_Mixer->PatternTransitionOccured = TRUE;
		}
		Commands = p_Mixer->Patterns[p_Mixer->Pattern].Commands;
		for (i = 0; i < p_Mixer->Channels; i++, chn++)
		{
			WORD Period = Commands[i][p_Mixer->Row].Period;
			if (Period == 0)
				chn->RowNote = -1;
			else
				chn->RowNote = PeriodToNoteIndex(Period);
			chn->RowSample = Commands[i][p_Mixer->Row].Sample;
			if (chn->RowSample > p_Mixer->Samples)
				chn->RowSample = 0;
			chn->RowEffect = Commands[i][p_Mixer->Row].Effect;
		}
	}
	if (p_Mixer->MusicSpeed == 0)
		p_Mixer->MusicSpeed = 1;
	return ProcessEffects(p_Mixer);
}

inline UINT GetFreqFromPeriod(UINT period)
{
	if (period == 0)
		return 0;
	return 14187580L / period;
}

BOOL ReadNote(MixerState *p_Mixer)
{
	Channel *chn;
	UINT i;
	if (ProcessRow(p_Mixer) == FALSE)
		return FALSE;

	if (p_Mixer->MusicTempo == 0)
		return FALSE;
	p_Mixer->BufferCount = (p_Mixer->MixRate * 640) / (p_Mixer->MusicTempo << 8);
	p_Mixer->SamplesPerTick = p_Mixer->BufferCount;
	p_Mixer->MixChannels = 0;
	chn = p_Mixer->Chns;
	for (i = 0; i < p_Mixer->Channels; i++, chn++)
	{
		chn->Increment = 0;
		if (chn->Period != 0 && chn->Length != 0)
		{
			UINT inc, freq;
			int vol = chn->Volume, period;
			if (vol > 64)
				vol = 64;
			if ((chn->Flags & CHN_TREMOLO) != 0)
			{
				// Tremolo handling here
			}
			if (vol != 0)
				chn->Volume = vol;
			if (chn->Period < 14)
				chn->Period = 14;
			period = chn->Period;
			if ((chn->Flags & CHN_ARPEGGIO) != 0)
			{
				// Arpeggio handling here
			}
			if ((chn->Flags & CHN_VIBRATO) != 0)
			{
				// Vibrato handling here
			}
			if (period > 3424)
				period = 3424;
			freq = GetFreqFromPeriod(period);
			inc = muldiv(freq, 0x10000, p_Mixer->MixRate);
			if (inc >= 0xFFB0 && inc <= 0x10090)
				inc = 0x10000;
			if (inc > 0xFF0000)
				inc = 0xFF0000;
			chn->Increment = (inc + 1) & ~3;
		}
		if (chn->Volume != 0 || chn->LeftVol != 0 || chn->RightVol != 0)
			chn->Flags |= CHN_VOLUMERAMP;
		else
			chn->Flags &= ~CHN_VOLUMERAMP;
		chn->NewLeftVol = chn->NewRightVol = 0;
		if ((chn->Increment >> 16) + 1 >= (int)(chn->LoopEnd - chn->LoopStart))
			chn->Flags &= ~CHN_LOOP;
		chn->Sample = ((chn->NewSample != NULL && chn->Length != 0 && chn->Increment != 0) ? chn->NewSample : NULL);
		if (chn->Sample != NULL)
		{
			if (p_Mixer->MixOutChannels == 2)
			{
				chn->NewLeftVol = (chn->Volume * chn->Pan) >> 7;
				chn->NewRightVol = (chn->Volume * (128 - chn->Pan)) >> 7;
			}
			else
				chn->NewLeftVol = chn->NewRightVol = chn->Volume;
			chn->RightRamp = chn->LeftRamp = 0;
			if ((chn->Flags & CHN_VOLUMERAMP) != 0 && (chn->LeftVol != chn->NewLeftVol || chn->RightVol != chn->NewRightVol))
			{
				int LeftDelta, RightDelta;
				UINT RampLength = 42;
				LeftDelta = (chn->NewLeftVol - chn->LeftVol);
				RightDelta = (chn->NewRightVol - chn->RightVol);
				if ((chn->LeftVol | chn->RightVol) != 0 && (chn->NewLeftVol | chn->NewRightVol) != 0 && (chn->Flags & CHN_FASTVOLRAMP) != 0)
				{
					RampLength = p_Mixer->BufferCount;
					if (RampLength > 512)
						RampLength = 512;
					else if (RampLength < 42)
						RampLength = 42;
				}
				chn->LeftRamp = LeftDelta / RampLength;
				chn->RightRamp = RightDelta / RampLength;
				chn->LeftVol = chn->NewLeftVol - (chn->LeftRamp * RampLength);
				chn->RightVol = chn->NewRightVol - (chn->RightRamp * RampLength);
				if ((chn->LeftRamp | chn->RightRamp) != 0)
					chn->RampLength = RampLength;
				else
				{
					chn->Flags &= ~CHN_VOLUMERAMP;
					chn->LeftVol = chn->NewLeftVol;
					chn->RightVol = chn->NewRightVol;
				}
			}
			else
			{
				chn->Flags &= ~CHN_VOLUMERAMP;
				chn->LeftVol = chn->NewLeftVol;
				chn->RightVol = chn->NewRightVol;
			}
			p_Mixer->ChnMix[p_Mixer->MixChannels++] = i;
		}
		else
			chn->LeftVol = chn->RightVol = chn->Length = 0;
	}

	return TRUE;
}

UINT GetResamplingFlag(MixerState *p_Mixer)
{
	if ((p_Mixer->SoundSetup & SNDMIX_SPLINESRCMODE) != 0)
		return MIXNDX_HQSRC;
	else if ((p_Mixer->SoundSetup & SNDMIX_POLYPHASESRCMODE) != 0)
		return MIXNDX_KAISERSRC;
	else if ((p_Mixer->SoundSetup & SNDMIX_FIRFILTERSRCMODE) != 0)
		return MIXNDX_FIRFILTERSRC;
	return 0;
}

inline UINT GetSampleCount(Channel *chn, UINT Samples)
{
	UINT Pos, PosLo, SampleCount;
	UINT LoopStart = ((chn->Flags & CHN_LOOP) != 0 ? chn->LoopStart : 0);
	int Increment = chn->Increment;
	if (Samples == 0 || Increment == 0 || chn->Length == 0)
		return 0;
	if (chn->Pos < LoopStart)
	{
		if (Increment < 0)
		{
			int Delta = ((LoopStart - chn->Pos) << 16) - (chn->PosLo & 0xFFFF);
			chn->Pos = LoopStart | (Delta >> 16);
			chn->PosLo = Delta & 0xFFFF;
			if (chn->Pos < LoopStart || chn->Pos >= (LoopStart + chn->Length) / 2)
			{
				chn->Pos = LoopStart;
				chn->PosLo = 0;
			}
			Increment = -Increment;
			chn->Increment = Increment;
			if ((chn->Flags & CHN_LOOP) == 0 || chn->Pos >= chn->Length)
			{
				chn->Pos = chn->Length;
				chn->PosLo = 0;
				return 0;
			}
		}
		else if (chn->Pos < 0)
			chn->Pos = 0;
	}
	else if (chn->Pos >= chn->Length)
	{
		if ((chn->Flags & CHN_LOOP) == 0)
			return 0;
		if (Increment < 0)
		{
			Increment = -Increment;
			chn->Increment = Increment;
		}
		chn->Pos += LoopStart - chn->Length;
		if (chn->Pos < LoopStart)
			chn->Pos = LoopStart;
	}
	Pos = chn->Pos;
	if (Pos < LoopStart)
	{
		if (Pos < 0 || Increment < 0)
			return 0;
	}
	if (Pos >= chn->Length)
		return 0;
	PosLo = chn->PosLo;
	SampleCount = Samples;
	if (Increment < 0)
	{
		UINT Inv = -Increment;
		UINT MaxSamples = 16384 / ((Inv >> 16) + 1);
		UINT DeltaHi, DeltaLo, PosDest;
		if (MaxSamples < 2)
			MaxSamples = 2;
		if (Samples > MaxSamples)
			Samples = MaxSamples;
		DeltaHi = (Inv >> 16) * (Samples - 1);
		DeltaLo = (Increment & 0xFFFF) * (Samples - 1);
		PosDest = Pos - DeltaHi + ((PosLo - DeltaLo) >> 16);
		if (PosDest < LoopStart)
			SampleCount = ((((Pos - LoopStart) << 16) + PosLo - 1) / Inv) + 1;
	}
	else
	{
		UINT MaxSamples = 16384 / ((Increment >> 16) + 1);
		UINT DeltaHi, DeltaLo, PosDest;
		if (MaxSamples < 2)
			MaxSamples = 2;
		if (Samples > MaxSamples)
			Samples = MaxSamples;
		DeltaHi = (Increment >> 16) * (Samples - 1);
		DeltaLo = (Increment & 0xFFFF) * (Samples - 1);
		PosDest = Pos + DeltaHi + ((PosLo + DeltaLo) >> 16);
		if (PosDest >= chn->Length)
			SampleCount = ((((chn->Length - Pos) << 16) - PosLo - 1) / Increment) + 1;
	}
	if (SampleCount <= 1)
		return 1;
	if (SampleCount > Samples)
		return Samples;
	return SampleCount;
}

inline int MixDone(MixerState *p_Mixer, BYTE *Buffer, UINT Read, UINT Max, UINT SampleSize)
{
	if (Read != 0)
		memset(Buffer, (p_Mixer->MixBitsPerSample == 8 ? 0x80 : 0), Read * SampleSize);
	return Max - Read;
}

void CreateStereoMix(MixerState *p_Mixer, UINT count)
{
	UINT i, channelsUsed = 0, channelsMixed = 0;
	if (count == 0)
		return;
	for (i = 0; i < p_Mixer->MixChannels; i++)
	{
		int SampleCount;
		UINT samples, rampSamples, Flags;
		Channel * const chn = &p_Mixer->Chns[p_Mixer->ChnMix[i]];
		int *buff = p_Mixer->MixBuffer;
		if (chn->Sample == NULL)
		//if (chn->Sample == NULL || p_Mixer->ChnMix[i] != 0)
		//if (chn->Sample == NULL || p_Mixer->ChnMix[i] == 0 || p_Mixer->ChnMix[i] == 3)
			continue;
		samples = count;
		channelsUsed++;
		Flags = GetResamplingFlag(p_Mixer);
		do
		{
			rampSamples = samples;
			if (chn->RampLength > 0)
			{
				if (rampSamples > chn->RampLength)
					rampSamples = chn->RampLength;
			}
			SampleCount = GetSampleCount(chn, rampSamples);
			if (SampleCount <= 0)
			{
				chn->Sample = NULL;
				chn->Length = chn->Pos = chn->PosLo = 0;
				chn->RampLength = 0;
				//EndChannelOffs(chn, buff, samples);
				samples = 0;
				continue;
			}
			if (chn->RampLength == 0 && (chn->LeftVol | chn->RightVol) == 0)
			{
				int delta = (chn->Increment * SampleCount) + chn->PosLo;
				chn->PosLo = delta & 0xFFFF;
				chn->Pos += delta >> 16;
				buff += SampleCount * 2;
			}
			else
			{
				MixInterface MixFunc = (chn->RampLength != 0 ? MixFunctionTable[Flags | MIXNDX_RAMP] : MixFunctionTable[Flags]);
				int *BuffMax = buff + (SampleCount * 2);
				MixFunc(chn, buff, BuffMax);
				buff = BuffMax;
			}
			samples -= SampleCount;
			if (chn->RampLength != 0)
			{
				chn->RampLength -= SampleCount;
				if (chn->RampLength <= 0)
				{
					chn->RampLength = 0;
					chn->LeftVol = chn->NewLeftVol;
					chn->RightVol = chn->NewRightVol;
					chn->LeftRamp = chn->RightRamp = 0;
				}
			}
		}
		while (samples > 0);
	}
}

long Read(void *Mixer, BYTE *Buffer, UINT BuffLen)
{
	MixerState *p_Mixer = (MixerState *)Mixer;
	UINT SampleSize = p_Mixer->MixBitsPerSample / 8 * p_Mixer->MixOutChannels;
	UINT Max = BuffLen / SampleSize, Read, Count, SampleCount;
	int *MixBuffer = p_Mixer->MixBuffer;

	if (Max == 0)
		return 0;
	Read = Max;
	if ((p_Mixer->SongFlags & SNG_ENDREACHED) != 0)
		return MixDone(p_Mixer, Buffer, Read, Max, SampleSize);
	while (Read > 0)
	{
		if (p_Mixer->BufferCount == 0)
		{
			// Deal with song fading
			if (ReadNote(p_Mixer) == FALSE)
			{
				if (p_Mixer->CurrentPattern >= p_Mixer->MaxOrder)
				{
					p_Mixer->SongFlags |= SNG_ENDREACHED;
					break;
				}
				// Song fading
			}
		}
		Count = p_Mixer->BufferCount;
		if (Count > MIXBUFFERSIZE)
			Count = MIXBUFFERSIZE;
		if (Count > Read)
			Count = Read;
		if (Count == 0)
			break;
		SampleCount = Count;
		// Reset the sound buffer.
		// XXX: This may be wrong relative to how MPT does this?
		memset(MixBuffer, 0x00, sizeof(int) * MIXBUFFERSIZE * 4);
		// MixOutChannels can only be one or two
		if (p_Mixer->MixOutChannels == 2)
		{
			SampleCount *= 2;
			CreateStereoMix(p_Mixer, Count);
			// Reverb processing?
		}
		else
		{
			CreateStereoMix(p_Mixer, Count);
			// Reverb processing?
			// MonoFromStereo?
		}
		Buffer += Convert32to16(Buffer, MixBuffer, SampleCount);
		Read -= Count;
		p_Mixer->BufferCount -= Count;
	}
	return MixDone(p_Mixer, Buffer, Read, Max, SampleSize);
}

long FillMODBuffer(MOD_Intern *p_MF, long toRead)
{
	long read;
	if (p_MF->p_Mixer == NULL)
		return -1;
	read = Read(p_MF->p_Mixer, p_MF->buffer, toRead);
	read *= (p_MF->p_FI->BitsPerSample / 8) * p_MF->p_FI->Channels;
	if (read == 0)
		return -2;
	return read;
}
