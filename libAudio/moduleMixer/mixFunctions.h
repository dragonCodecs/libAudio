// Standard mixing functions
#ifndef __mixFunctions_H__
#define __mixFunctions_H__ 1

#define WFIR_QUANTBITS		15
#define WFIR_QUANTSCALE		(1 << WFIR_QUANTBITS)
#define WFIR_SHIFT			(WFIR_QUANTBITS - 8)
#define WFIR_FRACBITS		12
#define WFIR_LUTLEN			((1 << (WFIR_FRACBITS + 1)) + 1)
#define WFIR_LOG2WIDTH		3
#define WFIR_WIDTH			(1 << WFIR_LOG2WIDTH)
#define WFIR_SMPSPERWING	((WFIR_WIDTH - 1) >> 1)

#define WFIR_HANN			0
#define WFIR_HAMMING		1
#define	WFIR_BLACKMANEXACT	2
#define WFIR_BLACKMAN3T61	3
#define WFIR_BLACKMAN3T67	4
#define WFIR_BLACKMAN4T92	5
#define WFIR_BLACKMAN4T74	6
#define WFIR_KAISER4T		7

#define WFIR_FRACSHIFT		(16 - (WFIR_FRACBITS + 1 + WFIR_LOG2WIDTH))
#define WFIR_FRACMASK		(((1 << (17 - WFIR_FRACSHIFT)) - 1) &~ ((1 << WFIR_LOG2WIDTH) - 1))
#define WFIR_FRACHALVE		(1 << (16 - (WFIR_FRACBITS + 2)))

#define SINC_PHASES			4096
#define M_EPS				1e-8

short FastSinc[1024] =
{
    0, 16384,     0,     0,   -31, 16383,    32,     0,   -63, 16381,    65,     0,   -93, 16378,   100,    -1,
 -124, 16374,   135,    -1,  -153, 16368,   172,    -3,  -183, 16361,   209,    -4,  -211, 16353,   247,    -5,
 -240, 16344,   287,    -7,  -268, 16334,   327,    -9,  -295, 16322,   368,   -12,  -322, 16310,   410,   -14,
 -348, 16296,   453,   -17,  -374, 16281,   497,   -20,  -400, 16265,   541,   -23,  -425, 16248,   587,   -26,
 -450, 16230,   634,   -30,  -474, 16210,   681,   -33,  -497, 16190,   729,   -37,  -521, 16168,   778,   -41,
 -543, 16145,   828,   -46,  -566, 16121,   878,   -50,  -588, 16097,   930,   -55,  -609, 16071,   982,   -60,
 -630, 16044,  1035,   -65,  -651, 16016,  1089,   -70,  -671, 15987,  1144,   -75,  -691, 15957,  1199,   -81,
 -710, 15926,  1255,   -87,  -729, 15894,  1312,   -93,  -748, 15861,  1370,   -99,  -766, 15827,  1428,  -105,
 -784, 15792,  1488,  -112,  -801, 15756,  1547,  -118,  -818, 15719,  1608,  -125,  -834, 15681,  1669,  -132,
 -850, 15642,  1731,  -139,  -866, 15602,  1794,  -146,  -881, 15561,  1857,  -153,  -896, 15520,  1921,  -161,
 -911, 15477,  1986,  -168,  -925, 15434,  2051,  -176,  -939, 15390,  2117,  -184,  -952, 15344,  2184,  -192,
 -965, 15298,  2251,  -200,  -978, 15251,  2319,  -208,  -990, 15204,  2387,  -216, -1002, 15155,  2456,  -225,
-1014, 15106,  2526,  -234, -1025, 15055,  2596,  -242, -1036, 15004,  2666,  -251, -1046, 14952,  2738,  -260,
-1056, 14899,  2810,  -269, -1066, 14846,  2882,  -278, -1075, 14792,  2955,  -287, -1084, 14737,  3028,  -296,
-1093, 14681,  3102,  -306, -1102, 14624,  3177,  -315, -1110, 14567,  3252,  -325, -1118, 14509,  3327,  -334,
-1125, 14450,  3403,  -344, -1132, 14390,  3480,  -354, -1139, 14330,  3556,  -364, -1145, 14269,  3634,  -374,
-1152, 14208,  3712,  -384, -1157, 14145,  3790,  -394, -1163, 14082,  3868,  -404, -1168, 14018,  3947,  -414,
-1173, 13954,  4027,  -424, -1178, 13889,  4107,  -434, -1182, 13823,  4187,  -445, -1186, 13757,  4268,  -455,
-1190, 13690,  4349,  -465, -1193, 13623,  4430,  -476, -1196, 13555,  4512,  -486, -1199, 13486,  4594,  -497,
-1202, 13417,  4676,  -507, -1204, 13347,  4759,  -518, -1206, 13276,  4842,  -528, -1208, 13205,  4926,  -539,
-1210, 13134,  5010,  -550, -1211, 13061,  5094,  -560, -1212, 12989,  5178,  -571, -1212, 12915,  5262,  -581,
-1213, 12842,  5347,  -592, -1213, 12767,  5432,  -603, -1213, 12693,  5518,  -613, -1213, 12617,  5603,  -624,
-1212, 12542,  5689,  -635, -1211, 12466,  5775,  -645, -1210, 12389,  5862,  -656, -1209, 12312,  5948,  -667,
-1208, 12234,  6035,  -677, -1206, 12156,  6122,  -688, -1204, 12078,  6209,  -698, -1202, 11999,  6296,  -709,
-1200, 11920,  6384,  -720, -1197, 11840,  6471,  -730, -1194, 11760,  6559,  -740, -1191, 11679,  6647,  -751,
-1188, 11598,  6735,  -761, -1184, 11517,  6823,  -772, -1181, 11436,  6911,  -782, -1177, 11354,  6999,  -792,
-1173, 11271,  7088,  -802, -1168, 11189,  7176,  -812, -1164, 11106,  7265,  -822, -1159, 11022,  7354,  -832,
-1155, 10939,  7442,  -842, -1150, 10855,  7531,  -852, -1144, 10771,  7620,  -862, -1139, 10686,  7709,  -872,
-1134, 10602,  7798,  -882, -1128, 10516,  7886,  -891, -1122, 10431,  7975,  -901, -1116, 10346,  8064,  -910,
-1110, 10260,  8153,  -919, -1103, 10174,  8242,  -929, -1097, 10088,  8331,  -938, -1090, 10001,  8420,  -947,
-1083,  9915,  8508,  -956, -1076,  9828,  8597,  -965, -1069,  9741,  8686,  -973, -1062,  9654,  8774,  -982,
-1054,  9566,  8863,  -991, -1047,  9479,  8951,  -999, -1039,  9391,  9039, -1007, -1031,  9303,  9127, -1015,
-1024,  9216,  9216, -1024, -1015,  9127,  9303, -1031, -1007,  9039,  9391, -1039,  -999,  8951,  9479, -1047,
 -991,  8863,  9566, -1054,  -982,  8774,  9654, -1062,  -973,  8686,  9741, -1069,  -965,  8597,  9828, -1076,
 -956,  8508,  9915, -1083,  -947,  8420, 10001, -1090,  -938,  8331, 10088, -1097,  -929,  8242, 10174, -1103,
 -919,  8153, 10260, -1110,  -910,  8064, 10346, -1116,  -901,  7975, 10431, -1122,  -891,  7886, 10516, -1128,
 -882,  7798, 10602, -1134,  -872,  7709, 10686, -1139,  -862,  7620, 10771, -1144,  -852,  7531, 10855, -1150,
 -842,  7442, 10939, -1155,  -832,  7354, 11022, -1159,  -822,  7265, 11106, -1164,  -812,  7176, 11189, -1168,
 -802,  7088, 11271, -1173,  -792,  6999, 11354, -1177,  -782,  6911, 11436, -1181,  -772,  6823, 11517, -1184,
 -761,  6735, 11598, -1188,  -751,  6647, 11679, -1191,  -740,  6559, 11760, -1194,  -730,  6471, 11840, -1197,
 -720,  6384, 11920, -1200,  -709,  6296, 11999, -1202,  -698,  6209, 12078, -1204,  -688,  6122, 12156, -1206,
 -677,  6035, 12234, -1208,  -667,  5948, 12312, -1209,  -656,  5862, 12389, -1210,  -645,  5775, 12466, -1211,
 -635,  5689, 12542, -1212,  -624,  5603, 12617, -1213,  -613,  5518, 12693, -1213,  -603,  5432, 12767, -1213,
 -592,  5347, 12842, -1213,  -581,  5262, 12915, -1212,  -571,  5178, 12989, -1212,  -560,  5094, 13061, -1211,
 -550,  5010, 13134, -1210,  -539,  4926, 13205, -1208,  -528,  4842, 13276, -1206,  -518,  4759, 13347, -1204,
 -507,  4676, 13417, -1202,  -497,  4594, 13486, -1199,  -486,  4512, 13555, -1196,  -476,  4430, 13623, -1193,
 -465,  4349, 13690, -1190,  -455,  4268, 13757, -1186,  -445,  4187, 13823, -1182,  -434,  4107, 13889, -1178,
 -424,  4027, 13954, -1173,  -414,  3947, 14018, -1168,  -404,  3868, 14082, -1163,  -394,  3790, 14145, -1157,
 -384,  3712, 14208, -1152,  -374,  3634, 14269, -1145,  -364,  3556, 14330, -1139,  -354,  3480, 14390, -1132,
 -344,  3403, 14450, -1125,  -334,  3327, 14509, -1118,  -325,  3252, 14567, -1110,  -315,  3177, 14624, -1102,
 -306,  3102, 14681, -1093,  -296,  3028, 14737, -1084,  -287,  2955, 14792, -1075,  -278,  2882, 14846, -1066,
 -269,  2810, 14899, -1056,  -260,  2738, 14952, -1046,  -251,  2666, 15004, -1036,  -242,  2596, 15055, -1025,
 -234,  2526, 15106, -1014,  -225,  2456, 15155, -1002,  -216,  2387, 15204,  -990,  -208,  2319, 15251,  -978,
 -200,  2251, 15298,  -965,  -192,  2184, 15344,  -952,  -184,  2117, 15390,  -939,  -176,  2051, 15434,  -925,
 -168,  1986, 15477,  -911,  -161,  1921, 15520,  -896,  -153,  1857, 15561,  -881,  -146,  1794, 15602,  -866,
 -139,  1731, 15642,  -850,  -132,  1669, 15681,  -834,  -125,  1608, 15719,  -818,  -118,  1547, 15756,  -801,
 -112,  1488, 15792,  -784,  -105,  1428, 15827,  -766,   -99,  1370, 15861,  -748,   -93,  1312, 15894,  -729,
  -87,  1255, 15926,  -710,   -81,  1199, 15957,  -691,   -75,  1144, 15987,  -671,   -70,  1089, 16016,  -651,
  -65,  1035, 16044,  -630,   -60,   982, 16071,  -609,   -55,   930, 16097,  -588,   -50,   878, 16121,  -566,
  -46,   828, 16145,  -543,   -41,   778, 16168,  -521,   -37,   729, 16190,  -497,   -33,   681, 16210,  -474,
  -30,   634, 16230,  -450,   -26,   587, 16248,  -425,   -23,   541, 16265,  -400,   -20,   497, 16281,  -374,
  -17,   453, 16296,  -348,   -14,   410, 16310,  -322,   -12,   368, 16322,  -295,    -9,   327, 16334,  -268,
   -7,   287, 16344,  -240,    -5,   247, 16353,  -211,    -4,   209, 16361,  -183,    -3,   172, 16368,  -153,
   -1,   135, 16374,  -124,    -1,   100, 16378,   -93,     0,    65, 16381,   -63,     0,    32, 16383,   -31,
};

short *KaiserSinc;
short *DownSample13x;
short *DownSample2x;
signed short *WFIRlutTab;
BOOL InitTables = FALSE;
double WFIRCutoff = 0.99;
BYTE WFIRType = WFIR_KAISER4T;

float WFIRCoef(int Cnr, float Offs, float Cut, int Width, int Type)
{
	double WidthM1 = Width - 1;
	double WidthM1Half = WidthM1 * 0.5;
	double PosU = (Cnr - Offs);
	double Pos = PosU - WidthM1Half;
	double Idl = 2.0 * M_PI * WidthM1;
	double Wc, Si;

	if (fabs(Pos) < M_EPS)
	{
		Wc = 1.0;
		Si = Cut;
	}
	else
	{
		switch (Type)
		{
			case WFIR_HANN:
			{
				Wc = 0.5 - 0.5 * cos(Idl * PosU);
				break;
			}
			case WFIR_HAMMING:
			{
				Wc = 0.54 - 0.45 * cos(Idl * PosU);
				break;
			}
			case WFIR_BLACKMANEXACT:
			{
				Wc = 0.42 - 0.50 * cos(Idl * PosU) + 0.08 * cos(2.0 * Idl * PosU);
				break;
			}
			case WFIR_BLACKMAN3T61:
			{
				Wc = 0.44959 - 0.49254 * cos(Idl * PosU) + 0.05677 * cos(2.0 * Idl * PosU);
				break;
			}
			case WFIR_BLACKMAN3T67:
			{
				Wc = 0.42323 - 0.49755 * cos(Idl * PosU) + 0.07922 * cos(2.0 * Idl * PosU);
				break;
			}
			case WFIR_BLACKMAN4T92:
			{
				Wc = 0.35875 - 0.48829 * cos(Idl * PosU) + 0.14128 * cos(2.0 * Idl * PosU) - 0.01168 * cos(3.0 * Idl * PosU);
				break;
			}
			case WFIR_BLACKMAN4T74:
			{
				Wc = 0.40217 - 0.49703 * cos(Idl * PosU) + 0.09392 * cos(2.0 * Idl * PosU) - 0.00183 * cos(3.0 * Idl * PosU);
				break;
			}
			case WFIR_KAISER4T:
			{
				Wc = 0.40243 - 0.49804 * cos(Idl * PosU) + 0.09831 * cos(2.0 * Idl * PosU) - 0.00122 * cos(3.0 * Idl * PosU);
				break;
			}
			default:
				Wc = 1.0;
		}
		Pos *= M_PI;
		Si = sin(Cut * Pos) / Pos;
	}
	return (float)(Wc * Si);
}

void InitWFIRTable()
{
	int Pcl;
	float PclLen = (float)(1 << WFIR_FRACBITS);
	float Norm = 1.0F / (2.0F * PclLen);
	float Cut = (float)WFIRCutoff;
	float Scale = WFIR_QUANTSCALE;
	WFIRlutTab = (signed short *)malloc(sizeof(signed short) * WFIR_LUTLEN * WFIR_WIDTH);

	for (Pcl = 0; Pcl < WFIR_LUTLEN; Pcl++)
	{
		float Gain, Coefs[WFIR_WIDTH];
		float Offs = (Pcl - PclLen) * Norm;
		int Cc, Idx = Pcl << WFIR_LOG2WIDTH;

		for (Cc = 0, Gain = 0.0F; Cc < WFIR_WIDTH; Cc++)
			Gain += (Coefs[Cc] = WFIRCoef(Cc, Offs, Cut, WFIR_WIDTH, WFIRType));
		Gain = 1.0F / Gain;
		for (Cc = 0; Cc < WFIR_WIDTH; Cc++)
		{
			float Coef = (float)floor(0.5 + Scale * Coefs[Cc] * Gain);
			WFIRlutTab[Idx + Cc] = (signed short)(Coef < -Scale ? -Scale : (Coef > Scale ? Scale : Coef));
		}
	}
}

double Zero(double y)
{
	double s = 1, ds = 1, d = 0;

	do
	{
		d = d + 2;
		ds = ds * (y * y) / (d * d);
		s = s + ds;
	}
	while (ds > 1e-7 * s);
	return s;
}

void getsinc(short *Sinc, double Beta, double LowPassFactor)
{
	double ZeroBeta = Zero(Beta);
	double LPAt = 4.0 * atan(1.0) * LowPassFactor;
	Sinc = (short *)malloc(sizeof(short) * SINC_PHASES * 8);
	for (int i = 0; i < 8 * SINC_PHASES; i++)
	{
		double FSinc;
		int x = 7 - (i & 7), n;
		x = (x * SINC_PHASES) + (i >> 3);
		if (x == 4 * SINC_PHASES)
			FSinc = 1.0;
		else
		{
			double y = (x - (4 * SINC_PHASES)) * (1.0 / SINC_PHASES);
			FSinc = sin(y * LPAt) * Zero(Beta * sqrt(1 - y * y * (1.0 / 16.0))) / (ZeroBeta * y * LPAt);
		}
		n = (int)(FSinc * LowPassFactor * (16384 * 256));
		*Sinc = (n + 0x80) >> 8;
		Sinc++;
	}
}

void InitialiseTables()
{
	InitWFIRTable();
	getsinc(KaiserSinc, 9.6377, WFIRCutoff);
	getsinc(DownSample13x, 8.5, 0.5);
	getsinc(DownSample2x, 2.7625, 0.425);
}

// Begin / End loop
#define SNDMIX_BEGINSAMPLELOOP \
	signed char *p = NULL; \
	int *vol = NULL; \
	Pos = chn->PosLo; \
	p = (signed char *)(chn->Sample + chn->Pos); \
	vol = Buff; \
	do \
	{

#define SNDMIX_ENDSAMPLELOOP \
		Pos += chn->Increment; \
	} \
	while (vol < BuffMax); \
	chn->Pos += Pos >> 16; \
	chn->PosLo = Pos & 0xFFFF;

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
	void __CDECL__ func(Channel *chn, int *Buff, int *BuffMax) \
	{ \
		UINT Pos;

#define END_MIX_INTERFACE() \
		SNDMIX_ENDSAMPLELOOP \
	}

#define BEGIN_RAMPMIX_INTERFACE(func) \
	BEGIN_MIX_INTERFACE(func) \
		UINT RampRightVol = chn->RightVol; \
		UINT RampLeftVol = chn->LeftVol;

#define END_RAMPMIX_INTERFACE() \
		SNDMIX_ENDSAMPLELOOP \
		chn->RightVol = RampRightVol; \
		chn->LeftVol = RampLeftVol; \
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
		UINT RampRightVol = chn->RightVol; \
		UINT RampLeftVol = chn->LeftVol; \
		MIX_BEGIN_FILTER

#define END_RAMPMIX_FLT_INTERFACE() \
		SNDMIX_ENDSAMPLELOOP \
		MIX_END_FILTER \
		chn->RightVol = RampRightVol; \
		chn->LeftVol = RampLeftVol; \
	}

// Mono
#define SNDMIX_GETMONOVOLNOIDO \
	int vol_ = p[Pos >> 16] << 8;

#define SNDMIX_GETMONOVOLLINEAR \
	int posHi = Pos >> 16; \
	int posLo = (Pos >> 8) & 0xFF; \
	int SrcVol = p[posHi]; \
	int DestVol = p[posHi + 1]; \
	int vol_ = (SrcVol << 8) + (posLo * (DestVol - SrcVol));

#define SNDMIX_GETMONOVOLHQSRC \
	int posHi = Pos >> 16; \
	int posLo = (Pos >> 6) & 0x03FC; \
	int vol_ = (FastSinc[posLo] * p[posHi - 1] + FastSinc[posLo + 1] * p[posHi] + \
		FastSinc[posLo + 2] * p[posHi + 1] + FastSinc[posLo + 3] * p[posHi + 2]) >> 6;

#define SNDMIX_GETMONOVOLKAISER \
	int posHi = Pos >> 16; \
	short *posLo = (short *)(sinc + (Pos & 0xFFF0)); \
	int vol_ = (posLo[0] * p[posHi - 3] + posLo[1] * p[posHi - 2] + \
		posLo[2] * p[posHi - 1] + posLo[3] * p[posHi] + \
		posLo[4] * p[posHi + 1] + posLo[5] * p[posHi + 2] + \
		posLo[6] * p[posHi + 3] + posLo[7] * p[posHi + 4]) >> 6;

#define SNDMIX_GETMONOVOLFIRFILTER \
	int posHi = Pos >> 16; \
	int posLo = Pos & 0xFFFF; \
	int firIdx = ((posLo + WFIR_FRACHALVE) >> WFIR_FRACSHIFT) & WFIR_FRACMASK; \
	int vol_ = WFIRlutTab[firIdx + 0] * p[posHi - 3]; \
	vol_ += WFIRlutTab[firIdx + 1] * p[posHi - 2]; \
	vol_ += WFIRlutTab[firIdx + 2] * p[posHi - 1]; \
	vol_ += WFIRlutTab[firIdx + 3] * p[posHi + 0]; \
	vol_ += WFIRlutTab[firIdx + 4] * p[posHi + 1]; \
	vol_ += WFIRlutTab[firIdx + 5] * p[posHi + 2]; \
	vol_ += WFIRlutTab[firIdx + 6] * p[posHi + 3]; \
	vol_ += WFIRlutTab[firIdx + 7] * p[posHi + 4]; \
	vol_ >>= WFIR_SHIFT;

// Volume
#define SNDMIX_STOREMONOVOL \
	vol[0] += vol_ * (chn->RightVol << 4); \
	vol[1] += vol_ * (chn->LeftVol << 4); \
	vol += 2;

#define SNDMIX_RAMPMONOVOL \
	RampLeftVol += chn->LeftRamp; \
	RampRightVol += chn->RightRamp; \
	vol[0] += vol_ * (RampRightVol << 4); \
	vol[1] += vol_ * (RampLeftVol << 4); \
	vol += 2;

// sinc
#define SNDMIX_INITSINCTABLE \
	char *sinc = (char *)(chn->Increment > 0x13000 || chn->Increment < -0x13000 ? \
		(chn->Increment > 0x18000 || chn->Increment < -0x18000 ? DownSample2x : DownSample13x) : KaiserSinc);

// Interfaces
// Mono
BEGIN_MIX_INTERFACE(Mono8BitMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLNOIDO
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLNOIDO
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLLINEAR
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLLINEAR
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono8BitHQMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLHQSRC
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLHQSRC
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono8BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLKAISER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLKAISER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono8BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLFIRFILTER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLFIRFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

// Filter Interfaces
// Mono
BEGIN_MIX_FLT_INTERFACE(FilterMono8BitMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLNOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLNOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLLINEAR
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLLINEAR
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono8BitHQMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLHQSRC
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLHQSRC
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono8BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLKAISER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLKAISER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono8BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLFIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP
	SNDMIX_GETMONOVOLFIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_MIX_FLT_INTERFACE()

#endif /*__mixFunctions_H__*/