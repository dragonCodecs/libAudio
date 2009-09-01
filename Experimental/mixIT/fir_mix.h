#define _USE_MATH_DEFINES
#include <math.h>

#define WFIR_QUANTBITS		15
#define WFIR_QUANTSCALE		(1 << WFIR_QUANTBITS)
#define WFIR_8SHIFT			(WFIR_QUANTBITS - 8)
#define WFIR_16BITSHIFT		WFIR_QUANTBITS
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

#define M_EPS				1e-8
#define M_BESSELEPS			1e-21

#define WFIR_FRACSHIFT		(16 - (WFIR_FRACBITS + 1 + WFIR_LOG2WIDTH))
#define WFIR_FRACMASK		(((1 << (17 - WFIR_FRACSHIFT)) - 1) &~ ((1 << WFIR_LOG2WIDTH) - 1))
#define WFIR_FRACHALVE		(1 << (16 - (WFIR_FRACBITS + 2)))

// doubel WFIRCutoff = 0.97;
double WFIRCutoff = 0.99;
BYTE WFIRType = WFIR_KAISER4T;

class WindowedFIR
{
public:
	WindowedFIR()
	{
	}

	~WindowedFIR()
	{
	}

	static float coef(int Cnr, float Offs, float Cut, int Width, int Type)
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

	static void InitTable()
	{
		int Pcl;
		float PclLen = (float)(1 << WFIR_FRACBITS);
		float Norm = 1.0F / (2.0F * PclLen);
		float Cut = (float)WFIRCutoff;
		float Scale = WFIR_QUANTSCALE;

		for (Pcl = 0; Pcl < WFIR_LUTLEN; Pcl++)
		{
			float Gain, Coefs[WFIR_WIDTH];
			float Offs = (Pcl - PclLen) * Norm;
			int Cc, Idx = Pcl << WFIR_LOG2WIDTH;

			for (Cc = 0, Gain = 0.0F; Cc < WFIR_WIDTH; Cc++)
				Gain += (Coefs[Cc] = coef(Cc, Offs, Cut, WFIR_WIDTH, WFIRType));
			Gain = 1.0F / Gain;
			for (Cc = 0; Cc < WFIR_WIDTH; Cc++)
			{
				float Coef = (float)floor(0.5 + Scale * Coefs[Cc] * Gain);
				lut[Idx + Cc] = (signed short)(Coef < -Scale ? -Scale : (Coef > Scale ? Scale : Coef));
			}
		}
	}

public:
	static signed short lut[WFIR_LUTLEN * WFIR_WIDTH];
};

signed short WindowedFIR::lut[WFIR_LUTLEN * WFIR_WIDTH];