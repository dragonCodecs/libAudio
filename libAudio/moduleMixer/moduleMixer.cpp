#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "../ProTracker.h"
#include "moduleMixer.h"

#define SNG_ENDREACHED		1

#define CHN_LOOP			0x01
#define CHN_TREMOLO			0x02
#define CHN_ARPEGGIO		0x04
#define CHN_VIBRATO			0x08
#define CHN_VOLUMERAMP		0x10
#define CHN_FASTVOLRAMP		0x20
#define CHN_PORTAMENTO		0x40
#define CHN_GLISSANDO		0x80

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

#define MIX_FILTER			0x01
#define MIX_LINEARSRC		0x02
#define MIX_HQSRC			0x04
#define MIX_KAISERSRC		0x06
#define MIX_FIRFILTERSRC	0x08
#define MIX_RAMP			0x10

#define MAX_VOLUME			64
#define MIXBUFFERSIZE		512

#pragma pack(push, 1)

typedef struct _int16dot16_t
{
	uint16_t Lo;
	int16_t Hi;
} int16dot16_t;

typedef union _int16dot16
{
	int32_t iValue;
	int16dot16_t Value;
} int16dot16;

typedef struct _Channel
{
	uint8_t *Sample, *NewSample;
	uint8_t Note, NewNote, NewSamp;
	uint32_t LoopStart, LoopEnd, Length;
	MODSample *Samp;
	uint8_t RowNote, RowSample, Volume;
	uint8_t FineTune, Flags, Pan;
	uint32_t Period, Pos, PosLo;
	int16dot16 Increment;
	int PortamentoDest;
	uint8_t Arpeggio, LeftVol, RightVol, RampLength;
	uint8_t NewLeftVol, NewRightVol;
	uint16_t RowEffect, PortamentoSlide;
	short LeftRamp, RightRamp;
	int Filter_Y1, Filter_Y2, Filter_Y3, Filter_Y4;
	int Filter_A0, Filter_B0, Filter_B1, Filter_HP;
	uint8_t TremoloDepth, TremoloSpeed, TremoloPos, TremoloType;
	uint8_t VibratoDepth, VibratoSpeed, VibratoPos, VibratoType;
	int DCOffsR, DCOffsL;
} Channel;

typedef struct _MixerState
{
	uint32_t MixRate, MixOutChannels, MixBitsPerSample;
	uint32_t Channels, Samples;
	uint32_t TickCount, BufferCount;
	uint32_t Row, NextRow;
	uint32_t MusicSpeed, MusicTempo;
	uint32_t Pattern, CurrentPattern, NextPattern, RestartPos;
	uint8_t *Orders, MaxOrder, PatternDelay;
	MODSample *Samp;
	uint8_t **SamplePCM;
	uint32_t RowsPerBeat, SamplesPerTick;
	Channel *Chns;
	uint32_t MixChannels, *ChnMix;
	MODPattern *Patterns;
	uint8_t SongFlags, SoundSetup;
	uint8_t PatternLoopCount, PatternLoopStart;
	int MixBuffer[MIXBUFFERSIZE * 2];
	int DCOffsR, DCOffsL;
} MixerState;

#pragma pack(pop)

uint16_t Periods[60] =
{
	1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017, 961, 907,
	856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
	428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
	107, 101, 95, 90, 85, 80, 76, 71, 67, 64, 60, 57
};

uint16_t TunedPeriods[192] = 
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

const uint8_t PreAmpTable[16] =
{
	0x60, 0x60, 0x60, 0x70,
	0x80, 0x88, 0x90, 0x98,
	0xA0, 0xA4, 0xA8, 0xAC,
	0xB0, 0xB4, 0xB8, 0xBC
};

const signed char SinusTable[64] =
{
	0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126,
	127, 126, 125, 122, 117, 112, 106, 98, 90, 81, 71, 60, 49, 37, 25, 12,
	0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117, -122, -125, -126,
	-127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12
};

const signed char RampDownTable[64] =
{
	124, 120, 116, 112, 108, 104, 100, 96, 92, 88, 84, 80, 76, 72, 68, 64,
	60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 0,
	-4, -8, -12, -16, -20, -24, -28, -32, -36, -40, -44, -48, -52, -56, -60, -64,
	-68, -72, -76, -80, -84, -88, -92, -96, -100, -104, -108, -112, -116, -120, -124, -128
};

const signed char SquareTable[64] =
{
	127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
	127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
	-127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127,
	-127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127
};

// Temp. table. Will rewrite this via using a randomisation
// function to generate 64 signed char datapoints. Table will
// have it's pointer stored inside the MixerState
const signed char RandomTable[64] =
{
	98, -127, -43, 88, 102, 41, -65, -94, 125, 20, -71, -86, -70, -32, -16, -96,
	17, 72, 107, -5, 116, -69, -62, -40, 10, -61, 65, 109, -18, -38, -13, -76,
	-23, 88, 21, -94, 8, 106, 21, -112, 6, 109, 20, -88, -30, 9, -127, 118,
	42, -34, 89, -4, -51, -72, 21, -29, 112, 123, 84, -101, -92, 98, -54, -95
};

const uint32_t LinearSlideUpTable[256] =
{
	65536, 65773, 66010, 66249, 66489, 66729, 66971, 67213,
	67456, 67700, 67945, 68190, 68437, 68685, 68933, 69182,
	69432, 69684, 69936, 70189, 70442, 70697, 70953, 71209,
	71467, 71725, 71985, 72245, 72507, 72769, 73032, 73296,
	73561, 73827, 74094, 74362, 74631, 74901, 75172, 75444,
	75717, 75991, 76265, 76541, 76818, 77096, 77375, 77655,
	77935, 78217, 78500, 78784, 79069, 79355, 79642, 79930,
	80219, 80509, 80800, 81093, 81386, 81680, 81976, 82272,
	82570, 82868, 83168, 83469, 83771, 84074, 84378, 84683,
	84989, 85297, 85605, 85915, 86225, 86537, 86850, 87164,
	87480, 87796, 88113, 88432, 88752, 89073, 89395, 89718,
	90043, 90369, 90695, 91023, 91353, 91683, 92015, 92347,
	92681, 93017, 93353, 93691, 94029, 94370, 94711, 95053,
	95397, 95742, 96088, 96436, 96785, 97135, 97486, 97839,
	98193, 98548, 98904, 99262, 99621, 99981, 100343, 100706,
	101070, 101435, 101802, 102170, 102540, 102911, 103283, 103657,
	104031, 104408, 104785, 105164, 105545, 105926, 106309, 106694,
	107080, 107467, 107856, 108246, 108637, 109030, 109425, 109820,
	110217, 110616, 111016, 111418, 111821, 112225, 112631, 113038,
	113447, 113857, 114269, 114682, 115097, 115514, 115931, 116351,
	116771, 117194, 117618, 118043, 118470, 118898, 119328, 119760,
	120193, 120628, 121064, 121502, 121941, 122382, 122825, 123269,
	123715, 124162, 124611, 125062, 125514, 125968, 126424, 126881,
	127340, 127801, 128263, 128727, 129192, 129660, 130129, 130599,
	131072, 131546, 132021, 132499, 132978, 133459, 133942, 134426,
	134912, 135400, 135890, 136381, 136875, 137370, 137866, 138365,
	138865, 139368, 139872, 140378, 140885, 141395, 141906, 142419,
	142935, 143451, 143970, 144491, 145014, 145538, 146064, 146593,
	147123, 147655, 148189, 148725, 149263, 149803, 150344, 150888,
	151434, 151982, 152531, 153083, 153637, 154192, 154750, 155310,
	155871, 156435, 157001, 157569, 158138, 158710, 159284, 159860,
	160439, 161019, 161601, 162186, 162772, 163361, 163952, 164545
};

const uint16_t LinearSlideDownTable[256] =
{
	65535, 65298, 65063, 64829, 64595, 64362, 64130, 63899,
	63669, 63439, 63211, 62983, 62756, 62530, 62304, 62080,
	61856, 61633, 61411, 61190, 60969, 60750, 60531, 60313,
	60095, 59879, 59663, 59448, 59234, 59020, 58808, 58596,
	58384, 58174, 57964, 57756, 57547, 57340, 57133, 56927,
	56722, 56518, 56314, 56111, 55909, 55708, 55507, 55307,
	55107, 54909, 54711, 54514, 54317, 54122, 53927, 53732,
	53539, 53348, 53155, 52964, 52773, 52583, 52393, 52205,
	52014, 51827, 51640, 51454, 51269, 51084, 50900, 50716,
	50534, 50352, 50170, 49989, 49809, 49630, 49451, 49273,
	49095, 48918, 48742, 48566, 48391, 48217, 48043, 47870,
	47697, 47525, 47354, 47184, 47013, 46844, 46675, 46507,
	46339, 46172, 46006, 45840, 45675, 45510, 45346, 45183,
	45020, 44858, 44696, 44535, 44375, 44215, 44055, 43897,
	43739, 43581, 43424, 43267, 43111, 42956, 42801, 42647,
	42493, 42340, 42188, 42036, 41884, 41733, 41583, 41433,
	41284, 41135, 40987, 40839, 40692, 40545, 40399, 40253,
	40108, 39964, 39820, 39676, 39533, 39391, 39249, 39107,
	38966, 38826, 38686, 38547, 38408, 38269, 38131, 37994,
	37857, 37721, 37585, 37449, 37314, 37180, 37046, 36912,
	36779, 36647, 36515, 36383, 36252, 36121, 35991, 35861,
	35732, 35603, 35475, 35347, 35220, 35093, 34967, 34841,
	34715, 34590, 34465, 34341, 34217, 34094, 33971, 33849,
	33727, 33605, 33484, 33363, 33243, 33123, 33004, 32885,
	32767, 32648, 32531, 32414, 32297, 32180, 32064, 31949,
	31834, 31719, 31605, 31491, 31377, 31264, 31151, 31039,
	30927, 30816, 30705, 30594, 30484, 30374, 30265, 30156,
	30047, 29939, 29831, 29723, 29616, 29509, 29403, 29297,
	29191, 29086, 28981, 28877, 28773, 28669, 28566, 28463,
	28360, 28258, 28156, 28055, 27954, 27853, 27753, 27653,
	27553, 27454, 27355, 27256, 27158, 27060, 26963, 26865,
	26769, 26672, 26576, 26480, 26385, 26290, 26195, 26101
};

// BEGIN: Code from OMPT 1.17RC2
// Credit for this code goes to Oliver Lapicque
#include "mixFunctions.h"

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

void ResetChannelPanning(MixerState *p_Mixer)
{
	uint8_t i;
	Channel *chn = p_Mixer->Chns;
	for (i = 0; i < p_Mixer->Channels; i++, chn++)
	{
		uint8_t j = i % 4;
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
	p_Mixer->ChnMix = (uint32_t *)malloc(sizeof(uint32_t) * p_Mixer->Channels);
	memset(p_Mixer->Chns, 0x00, sizeof(Channel) * p_Mixer->Channels);
	memset(p_Mixer->ChnMix, 0x00, sizeof(uint32_t) * p_Mixer->Channels);
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

inline uint8_t PeriodToNoteIndex(uint16_t Period)
{
	uint8_t i, min = 0, max = 59;
	do
	{
		i = min + ((max - min) / 2);
		if (Periods[i] == Period)
			return i;
		else if (Periods[i] < Period)
		{
			if (i > 0)
			{
				uint32_t Dist1 = Period - Periods[i];
				uint32_t Dist2 = abs(Periods[i - 1] - Period);
				if (Dist1 < Dist2)
					return i;
			}
			max = i - 1;
		}
		else if (Periods[i] > Period)
		{
			if (i < 59)
			{
				uint32_t Dist1 = Periods[i] - Period;
				uint32_t Dist2 = abs(Period - Periods[i + 1]);
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

void SampleChange(MixerState *p_Mixer, Channel *chn, uint32_t sample_)
{
	MODSample *smp;
	uint32_t note, sample = sample_ - 1;

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

inline uint32_t GetPeriodFromNote(uint8_t Note, uint8_t FineTune)
{
	if (Note == 0xFF)
		return 0;
	if (FineTune != 0)
		return (TunedPeriods[(FineTune * 12) + (Note % 12)] << 2) >> (Note / 12);
	else
		return Periods[Note] << 2;
}

void NoteChange(MixerState *p_Mixer, uint32_t nChn, uint8_t note, uint8_t cmd)
{
	uint32_t period;
	Channel * const chn = &p_Mixer->Chns[nChn];
	MODSample *smp = chn->Samp;

	chn->Note = note;
	chn->NewSamp = 0;
	period = GetPeriodFromNote(note, chn->FineTune);
	if (smp == NULL)
		return;
	if (period != 0)
	{
		if (cmd != CMD_TONEPORTAMENTO && cmd != CMD_TONEPORTAVOL)
			chn->Period = period;
		chn->PortamentoDest = period;
		if (chn->PortamentoDest == chn->Period || chn->Length == 0)
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
			if ((chn->TremoloType & 0x04) != 0)
				chn->TremoloPos = 0;
		}
		if (chn->Pos > chn->Length)
			chn->Pos = chn->Length;
		chn->PosLo = 0;
	}
	if (chn->PortamentoDest == chn->Period)
		chn->Flags |= CHN_FASTVOLRAMP;
	chn->LeftVol = chn->RightVol = 0;
}

void ProcessExtendedCommand(MixerState *p_Mixer, BOOL RunCmd, Channel *chn, uint32_t i, uint8_t param)
{
	uint8_t cmd = (param & 0xF0) >> 8;
	param &= 0x0F;
	switch (cmd)
	{
		case CMDEX_FILTER:
		{
			// Could implement a 7Khz lowpass filter
			// For this, or just do what MPT does - ignore it.
			// XXX: Currently we ignore this.
			break;
		}
		case CMDEX_FINEPORTAUP:
		{
			if (p_Mixer->TickCount == 0)
			{
				if (chn->Period != 0 && param != 0)
				{
					chn->Period -= param << 2;
					if (chn->Period < 56)
						chn->Period = 56;
				}
			}
			break;
		}
		case CMDEX_FINEPORTADOWN:
		{
			if (p_Mixer->TickCount == 0)
			{
				if (chn->Period != 0 && param != 0)
				{
					chn->Period += param << 2;
					if (chn->Period > 7040)
						chn->Period = 7040;
				}
			}
			break;
		}
		case CMDEX_GLISSANDO:
		{
			if (param == 0)
				chn->Flags &= ~CHN_GLISSANDO;
			else if (param == 1)
				chn->Flags |= CHN_GLISSANDO;
			break;
		}
		case CMDEX_VIBRATOWAVE:
		{
			chn->VibratoType = param & 0x07;
			break;
		}
		case CMDEX_FINETUNE:
		{
			chn->FineTune = param;
			chn->Period = GetPeriodFromNote(chn->Note, chn->FineTune);
			break;
		}
		case CMDEX_TREMOLOWAVE:
		{
			chn->TremoloType = param & 0x07;
			break;
		}
		case CMDEX_RETRIGER:
		{
			if (param != 0 && (p_Mixer->TickCount % param) == 0)
				NoteChange(p_Mixer, i, chn->NewNote, 0);
			break;
		}
		case CMDEX_FINEVOLUP:
		{
			if (param != 0 && RunCmd == FALSE)
				chn->Volume += param;
			break;
		}
		case CMDEX_FINEVOLDOWN:
		{
			if (param != 0 && RunCmd == FALSE)
				chn->Volume -= param;
			break;
		}
		case CMDEX_CUT:
		{
			if (p_Mixer->TickCount == param)
			{
				chn->Volume = 0;
				chn->Flags |= CHN_FASTVOLRAMP;
			}
			break;
		}
		case CMDEX_INVERTLOOP:
		{
			break;
		}
	}
}

inline int PatternLoop(MixerState *p_Mixer, uint32_t param)
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

inline void VolumeSlide(BOOL DoSlide, Channel *chn, uint8_t param)
{
	if (DoSlide != FALSE)
	{
		short NewVolume = chn->Volume;
		if ((param & 0xF0) != 0)
			NewVolume += (param & 0xF0) >> 8;
		else
			NewVolume -= param & 0x0F;
		chn->Flags |= CHN_FASTVOLRAMP;
		if (NewVolume < 0)
			NewVolume = 0;
		else if (NewVolume > 64)
			NewVolume = 64;
		chn->Volume = NewVolume & 0xFF;
	}
}

inline void PortamentoUp(MixerState *p_Mixer, BOOL DoSlide, Channel *chn, uint8_t param)
{
	if (DoSlide || p_Mixer->MusicSpeed == 1)
	{
		chn->Period -= param;
		if (chn->Period < 56)
			chn->Period = 56;
	}
}

inline void PortamentoDown(MixerState *p_Mixer, BOOL DoSlide, Channel *chn, uint8_t param)
{
	if (DoSlide || p_Mixer->MusicSpeed == 1)
	{
		chn->Period += param;
		if (chn->Period > 7040)
			chn->Period = 7040;
	}
}

inline void TonePortamento(MixerState *p_Mixer, Channel *chn, uint8_t param)
{
	if (param != 0)
		chn->PortamentoSlide = param << 2;
	chn->Flags |= CHN_PORTAMENTO;
	if (chn->Period != 0 && chn->PortamentoDest != 0)
	{
		if ((int)chn->Period < chn->PortamentoDest)
		{
			int Delta;
			if ((chn->Flags & CHN_GLISSANDO) != 0)
			{
				uint8_t Slide = (uint8_t)(chn->PortamentoSlide >> 2);
				Delta = muldiv(chn->Period, LinearSlideUpTable[Slide], 32768) - chn->Period;
				if (Delta < 1)
					Delta = 1;
			}
			else
				Delta = chn->PortamentoSlide;
			Delta = chn->Period + Delta;
			if (Delta < 56)
				Delta = 56;
			if (Delta > chn->PortamentoDest)
				Delta = chn->PortamentoDest;
			chn->Period = (uint32_t)Delta;
		}
		else if ((int)chn->Period > chn->PortamentoDest)
		{
			int Delta;
			if ((chn->Flags & CHN_GLISSANDO) != 0)
			{
				uint8_t Slide = (uint8_t)(chn->PortamentoSlide >> 2);
				Delta = -muldiv(chn->Period, LinearSlideDownTable[Slide], 32768) - chn->Period;
				if (Delta < 1)
					Delta = 1;
			}
			else
				Delta = chn->PortamentoSlide;
			Delta = chn->Period - Delta;
			if (Delta < chn->PortamentoDest)
				Delta = chn->PortamentoDest;
			if (Delta > 7040)
				Delta = 7040;
			chn->Period = (uint32_t)Delta;
		}
	}
}

BOOL ProcessEffects(MixerState *p_Mixer)
{
	int PositionJump = -1, BreakRow = -1, PatternLoopRow = -1;
	Channel *chn = p_Mixer->Chns;
	uint32_t i;
	for (i = 0; i < p_Mixer->Channels; i++, chn++)
	{
		uint8_t sample = chn->RowSample;
		uint8_t cmd = (chn->RowEffect & 0xF00) >> 8;
		uint8_t param = (chn->RowEffect & 0xFF);
		uint32_t StartTick = 0;

		chn->Flags &= ~CHN_FASTVOLRAMP;
		if (cmd == CMD_EXTENDED)
		{
			uint8_t excmd = (param & 0xF0) >> 4;
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
			uint8_t note = chn->RowNote;
			if (sample != 0)
				chn->NewSamp = sample;
			if (note == 0xFF && sample != 0)
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
				NoteChange(p_Mixer, i, note, cmd);
			}
		}
		if (cmd != 0 || (cmd == 0 && param != 0))
		{
			switch (cmd)
			{
				case CMD_ARPEGGIO:
				{
					if (p_Mixer->TickCount != 0 || chn->Period == 0 || chn->Note == 0xFF || param == 0)
						break;
					chn->Flags |= CHN_ARPEGGIO;
					chn->Arpeggio = param;
					break;
				}
				case CMD_PORTAMENTOUP:
				{
					if (param != 0)
						PortamentoUp(p_Mixer, p_Mixer->TickCount > StartTick, chn, param);
					break;
				}
				case CMD_PORTAMENTODOWN:
				{
					if (param != 0)
						PortamentoDown(p_Mixer, p_Mixer->TickCount > StartTick, chn, param);
					break;
				}
				case CMD_TONEPORTAMENTO:
				{
					TonePortamento(p_Mixer, chn, param);
					break;
				}
				case CMD_VIBRATO:
				{
					if ((param & 0x0F) != 0)
						chn->VibratoDepth = (param & 0x0F) * 4;
					if ((param & 0xF0) != 0)
						chn->VibratoSpeed = param >> 4;
					chn->Flags |= CHN_VIBRATO;
					break;
				}
				case CMD_TONEPORTAVOL:
				{
					if (param != 0)
						VolumeSlide(TRUE, chn, param);
					TonePortamento(p_Mixer, chn, 0);
					break;
				}
				case CMD_VIBRATOVOL:
				{
					if (param != 0)
						VolumeSlide(TRUE, chn, param);
					chn->Flags |= CHN_VIBRATO;
					break;
				}
				case CMD_TREMOLO:
				{
					if ((param & 0x0F) != 0)
						chn->TremoloDepth = param & 0x0F;
					if ((param & 0xF0) != 0)
						chn->TremoloSpeed = param >> 4;
					chn->Flags |= CHN_TREMOLO;
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
					if (p_Mixer->TickCount != 0)
						break;
					chn->Pos = ((uint32_t)param) << 9;
					chn->PosLo = 0;
					if (chn->Pos > chn->Length)
						chn->Pos = chn->Length;
					break;
				}
				case CMD_VOLUMESLIDE:
				{
					if (param != 0)
						VolumeSlide(p_Mixer->TickCount > StartTick, chn, param);
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
						uint8_t NewVolume = param;
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
					ProcessExtendedCommand(p_Mixer, p_Mixer->TickCount > StartTick, chn, i, param);
					break;
				}
				case CMD_SPEED:
				{
					/* Speed rules (as per http://www.aes.id.au/modformat.html)
					 * NewSpeed == 0 => NewSpeed = 1
					 * NewSpeed <= 32 => Speed = NewSpeed (TPR)
					 * NewSpeed > 32 => Tempo = NewSpeed (BPM)
					 */
					uint8_t NewSpeed = param;
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
			if ((uint32_t)PositionJump < p_Mixer->CurrentPattern || ((uint32_t)PositionJump == p_Mixer->CurrentPattern && (uint32_t)BreakRow <= p_Mixer->Row))
				Jump = FALSE;
			if (Jump == TRUE && (PositionJump != p_Mixer->CurrentPattern || BreakRow != p_Mixer->Row))
			{
				if (PositionJump != p_Mixer->CurrentPattern)
					p_Mixer->PatternLoopCount = p_Mixer->PatternLoopStart = 0;
				p_Mixer->NextPattern = PositionJump;
				p_Mixer->NextRow = BreakRow;
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
		uint32_t i;
		MODCommand (*Commands)[64];
		Channel *chn = p_Mixer->Chns;
		p_Mixer->TickCount = 0;
		p_Mixer->PatternDelay = 0;
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
		}
		Commands = p_Mixer->Patterns[p_Mixer->Pattern].Commands;
		for (i = 0; i < p_Mixer->Channels; i++, chn++)
		{
			uint16_t Period = Commands[i][p_Mixer->Row].Period;
			if (Period == 0)
				chn->RowNote = -1;
			else
				chn->RowNote = PeriodToNoteIndex(Period);
			chn->RowSample = Commands[i][p_Mixer->Row].Sample;
			if (chn->RowSample > p_Mixer->Samples)
				chn->RowSample = 0;
			chn->RowEffect = Commands[i][p_Mixer->Row].Effect;
			chn->Flags &= ~(CHN_TREMOLO | CHN_ARPEGGIO | CHN_VIBRATO | CHN_PORTAMENTO | CHN_GLISSANDO);
		}
	}
	if (p_Mixer->MusicSpeed == 0)
		p_Mixer->MusicSpeed = 1;
	return ProcessEffects(p_Mixer);
}

BOOL ReadNote(MixerState *p_Mixer)
{
	Channel *chn;
	uint32_t i;
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
 		chn->Increment.iValue = 0;
		if (chn->Period != 0 && chn->Length != 0)
		{
			int inc, period;
			short vol = chn->Volume;
			if ((chn->Flags & CHN_TREMOLO) != 0)
			{
				uint8_t TremoloPos = chn->TremoloPos;
				if (vol > 0)
				{
					uint8_t TremoloType = chn->TremoloType & 0x03;
					uint8_t TremoloDepth = chn->TremoloDepth << 2;
					if (TremoloType == 1)
						vol += (RampDownTable[TremoloPos] * TremoloDepth) >> 8;
					else if (TremoloType == 2)
						vol += (SquareTable[TremoloPos] * TremoloDepth) >> 8;
					else if (TremoloType == 3)
						vol += (RandomTable[TremoloPos] * chn->TremoloDepth) >> 8;
					else
						vol += (SinusTable[TremoloPos] * TremoloDepth) >> 8;
				}
				if (p_Mixer->TickCount != 0)
					chn->TremoloPos = (TremoloPos + chn->TremoloSpeed) & 0x3F;
			}
			if (vol < 0)
				vol = 0;
			if (vol > 64)
				vol = 64;
			chn->Volume = vol;
			if (chn->Period < 56)
				chn->Period = 56;
			period = chn->Period;
			if ((chn->Flags & CHN_ARPEGGIO) != 0)
			{
				uint8_t n = p_Mixer->TickCount % 3;
				if (n == 1)
					period = GetPeriodFromNote(chn->Note + (chn->Arpeggio >> 4), chn->FineTune);
				else if (n == 2)
					period = GetPeriodFromNote(chn->Note + (chn->Arpeggio & 0x0F), chn->FineTune);
			}
			if ((chn->Flags & CHN_VIBRATO) != 0)
			{
				char Delta;
				uint8_t VibratoPos = chn->VibratoPos;
				uint8_t VibratoType = chn->VibratoType & 0x03;
				if (VibratoType == 1)
					Delta = RampDownTable[VibratoPos];
				else if (VibratoType == 2)
					Delta = SquareTable[VibratoPos];
				else if (VibratoType == 3)
					Delta = RandomTable[VibratoPos];
				else
					Delta = SinusTable[VibratoPos];
				period += (short)(((int)Delta * (int)chn->VibratoDepth) >> 7);
				chn->VibratoPos = (VibratoPos + chn->VibratoSpeed) & 0x3F;
			}
			// 14 << 2 == 56
			if (period < 56)
				period = 56;
			// 1760 << 2 == 7040
			if (period > 7040)
				period = 7040;
			inc = muldiv(14187580UL / period, 0x10000, p_Mixer->MixRate);
			chn->Increment.iValue = (inc + 1) & ~3;
		}
		if (chn->Volume != 0 || chn->LeftVol != 0 || chn->RightVol != 0)
			chn->Flags |= CHN_VOLUMERAMP;
		else
			chn->Flags &= ~CHN_VOLUMERAMP;
		chn->NewLeftVol = chn->NewRightVol = 0;
		if (chn->Increment.Value.Hi + 1 >= (int)(chn->LoopEnd - chn->LoopStart))
			chn->Flags &= ~CHN_LOOP;
		chn->Sample = ((chn->NewSample != NULL && chn->Length != 0 && chn->Increment.iValue != 0) ? chn->NewSample : NULL);
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
			// Do we need to ramp the volume up or down?
			if ((chn->Flags & CHN_VOLUMERAMP) != 0 && (chn->LeftVol != chn->NewLeftVol || chn->RightVol != chn->NewRightVol))
			{
				int LeftDelta, RightDelta;
				uint32_t RampLength = 1;
				// Calculate Volume deltas
				LeftDelta = chn->NewLeftVol - chn->LeftVol;
				RightDelta = chn->NewRightVol - chn->RightVol;
				// Check if we need to calculate the RampLength, and do so if need be
				if ((chn->LeftVol | chn->RightVol) != 0 && (chn->NewLeftVol | chn->NewRightVol) != 0 && (chn->Flags & CHN_FASTVOLRAMP) != 0)
				{
					RampLength = p_Mixer->BufferCount;
					// Clipping:
					if (RampLength > 512)
						RampLength = 512;
					else if (RampLength < 1)
						RampLength = 1;
				}
				// Calculate value to add to the volume to get it closer to the new volume during ramping
				chn->LeftRamp = LeftDelta / RampLength;
				chn->RightRamp = RightDelta / RampLength;
				// Normalise the current volume so that the ramping won't under or over shoot
				chn->LeftVol = chn->NewLeftVol - (chn->LeftRamp * RampLength);
				chn->RightVol = chn->NewRightVol - (chn->RightRamp * RampLength);
				// If the ramp values aren't 0 (ramping already done?)
				if ((chn->LeftRamp | chn->RightRamp) != 0)
					chn->RampLength = RampLength;
				else
				{
					// Otherwise the ramping is done, so don't need to make the mixer functions do it for us
					chn->Flags &= ~CHN_VOLUMERAMP;
					chn->LeftVol = chn->NewLeftVol;
					chn->RightVol = chn->NewRightVol;
				}
			}
			// No? ok, scratch the ramping.
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

uint32_t GetResamplingFlag(MixerState *p_Mixer)
{
	if ((p_Mixer->SoundSetup & SNDMIX_SPLINESRCMODE) != 0)
		return MIX_HQSRC;
	else if ((p_Mixer->SoundSetup & SNDMIX_POLYPHASESRCMODE) != 0)
		return MIX_KAISERSRC;
	else if ((p_Mixer->SoundSetup & SNDMIX_FIRFILTERSRCMODE) != 0)
		return MIX_FIRFILTERSRC;
	return 0;
}

inline uint32_t GetSampleCount(Channel *chn, uint32_t Samples)
{
	uint32_t Pos, PosLo, SampleCount;
	uint32_t LoopStart = ((chn->Flags & CHN_LOOP) != 0 ? chn->LoopStart : 0);
	int16dot16 Increment = chn->Increment;
	if (Samples == 0 || Increment.iValue == 0 || chn->Length == 0)
		return 0;
	if (chn->Pos < LoopStart)
	{
		if (Increment.iValue < 0)
		{
			int Delta = ((LoopStart - chn->Pos) << 16) - (chn->PosLo & 0xFFFF);
			chn->Pos = LoopStart | (Delta >> 16);
			chn->PosLo = Delta & 0xFFFF;
			if (chn->Pos < LoopStart || chn->Pos >= (LoopStart + chn->Length) / 2)
			{
				chn->Pos = LoopStart;
				chn->PosLo = 0;
			}
			Increment.iValue = -Increment.iValue;
			chn->Increment.iValue = Increment.iValue;
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
		if (Increment.iValue < 0)
		{
			Increment.iValue = -Increment.iValue;
			chn->Increment.iValue = Increment.iValue;
		}
		chn->Pos += LoopStart - chn->Length;
		if (chn->Pos < LoopStart)
			chn->Pos = LoopStart;
	}
	Pos = chn->Pos;
	if (Pos < LoopStart)
	{
		if (Pos < 0 || Increment.iValue < 0)
			return 0;
	}
	if (Pos >= chn->Length)
		return 0;
	PosLo = chn->PosLo;
	SampleCount = Samples;
	if (Increment.iValue < 0)
	{
		int16dot16 Inv = Increment;
		uint32_t MaxSamples;
		uint32_t DeltaHi, DeltaLo, PosDest;
		Inv.iValue = -Inv.iValue;
		MaxSamples = 16384 / (Inv.Value.Hi + 1);
		if (MaxSamples < 2)
			MaxSamples = 2;
		if (Samples > MaxSamples)
			Samples = MaxSamples;
		DeltaHi = Inv.Value.Hi * Samples;
		DeltaLo = Inv.Value.Lo * Samples;
		PosDest = Pos - DeltaHi + ((PosLo - DeltaLo) >> 16);
		if (PosDest < LoopStart)
			SampleCount = ((((Pos - LoopStart) << 16) + PosLo - 1) / Inv.iValue);
	}
	else
	{
		uint32_t MaxSamples = 16384 / (Increment.Value.Hi + 1);
		uint32_t DeltaHi, DeltaLo, PosDest;
		if (MaxSamples < 2)
			MaxSamples = 2;
		if (Samples > MaxSamples)
			Samples = MaxSamples;
		DeltaHi = Increment.Value.Hi * Samples;
		DeltaLo = Increment.Value.Lo * Samples;
		PosDest = Pos + DeltaHi + ((PosLo + DeltaLo) >> 16);
		if (PosDest >= chn->Length)
//			SampleCount = chn->Length - chn->Pos;
			SampleCount = (((chn->Length - Pos) << 16) - PosLo) / Increment.iValue;
	}
	if (SampleCount <= 1)
		return 1;
	if (SampleCount > Samples)
		return Samples;
	return SampleCount;
}

inline int MixDone(MixerState *p_Mixer, uint8_t *Buffer, uint32_t Read, uint32_t Max, uint32_t SampleSize)
{
	if (Read != 0)
		memset(Buffer, 0, Read * SampleSize);
	return Max - Read;
}

inline void FixDCOffset(int *p_DCOffsL, int *p_DCOffsR, int *buff, uint32_t samples)
{
	int DCOffsL = *p_DCOffsL;
	int DCOffsR = *p_DCOffsR;
	while (samples != 0 && (DCOffsR | DCOffsL) != 0)
	{
		int OffsL = -DCOffsL;
		int OffsR = -DCOffsR;
#ifdef _WINDOWS
		__asm
		{
			sar OffsL, 31
			sar OffsR, 31
		}
#else
		asm(".intel_syntax noprefix\n"
			"\tsar %%eax, 31\n"
			"\tsar %%ebx, 31\n"
			".att_syntax\n" : [OffsL] "=a" (OffsL), [OffsR] "=b" (OffsR) :
				"a" (OffsL), "b" (OffsR) : );
#endif
		OffsL &= 0xFF;
		OffsR &= 0xFF;
		OffsL += DCOffsL;
		OffsR += DCOffsR;
#ifdef _WINDOWS
		__asm
		{
			sar OffsL, 8
			sar OffsR, 8
		}
#else
		asm(".intel_syntax noprefix\n"
			"\tsar %%eax, 8\n"
			"\tsar %%ebx, 8\n"
			".att_syntax\n" : [OffsL] "=a" (OffsL), [OffsR] "=b" (OffsR) :
				"a" (OffsL), "b" (OffsR) : );
#endif
		DCOffsL -= OffsL;
		DCOffsR -= OffsR;
		buff[0] += DCOffsR;
		buff[1] += DCOffsL;
		buff += 2;
		samples--;
	}
	*p_DCOffsL = DCOffsL;
	*p_DCOffsR = DCOffsR;
}

inline void DCFixingFill(int *MixBuffer, uint32_t samples, MixerState *p_Mixer)
{
	int *buff = MixBuffer;
	for (uint32_t i = 0; i < samples; i++)
	{
		buff[0] = 0;
		buff[1] = 0;
		buff += 2;
	}
	FixDCOffset(&p_Mixer->DCOffsL, &p_Mixer->DCOffsR, MixBuffer, samples);
}

void CreateStereoMix(MixerState *p_Mixer, uint32_t count)
{
	int SampleCount;
	uint32_t i, Flags, rampSamples;
	if (count == 0)
		return;
	Flags = GetResamplingFlag(p_Mixer);
	for (i = 0; i < p_Mixer->MixChannels; i++)
	{
		uint32_t samples = count;
		int *buff = p_Mixer->MixBuffer;
		Channel * const chn = &p_Mixer->Chns[p_Mixer->ChnMix[i]];
		if (chn->Sample == NULL)
			continue;
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
				FixDCOffset(&chn->DCOffsL, &chn->DCOffsR, buff, samples);
				p_Mixer->DCOffsL += chn->DCOffsL;
				p_Mixer->DCOffsR += chn->DCOffsR;
				chn->DCOffsL = chn->DCOffsR = 0;
				samples = 0;
				continue;
			}
			if (chn->RampLength == 0 && (chn->LeftVol | chn->RightVol) == 0)
				buff += SampleCount * 2;
			else
			{
				MixInterface MixFunc = MixFunctionTable[Flags | (chn->RampLength != 0 ? MIX_RAMP : 0)];
				int *BuffMax = buff + (SampleCount * 2);
				chn->DCOffsR = -((BuffMax - 2)[0]);
				chn->DCOffsL = -((BuffMax - 2)[1]);
				MixFunc(chn, buff, BuffMax);
				chn->DCOffsR += ((BuffMax - 2)[0]);
				chn->DCOffsL += ((BuffMax - 2)[1]);
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
					chn->Flags &= ~(CHN_FASTVOLRAMP | CHN_VOLUMERAMP);
				}
			}
		}
		while (samples > 0);
	}
}

long Read(void *Mixer, uint8_t *Buffer, uint32_t BuffLen)
{
	MixerState *p_Mixer = (MixerState *)Mixer;
	uint32_t SampleSize = p_Mixer->MixBitsPerSample / 8 * p_Mixer->MixOutChannels;
	uint32_t Max = BuffLen / SampleSize, Read, Count, SampleCount;
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
		DCFixingFill(MixBuffer, SampleCount, p_Mixer);
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
