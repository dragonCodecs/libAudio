#ifndef __moduleMixer_H__
#define __moduleMixer_H__

#define CHN_LOOP			0x0001
#define CHN_TREMOLO			0x0002
#define CHN_ARPEGGIO		0x0004
#define CHN_VIBRATO			0x0008
#define CHN_VOLUMERAMP		0x0010
#define CHN_FASTVOLRAMP		0x0020
#define CHN_PORTAMENTO		0x0040
#define CHN_GLISSANDO		0x0080
#define CHN_TREMOR			0x0100
#define CHN_PANBRELLO		0x0200
#define CHN_NOTEOFF			0x0400
#define CHN_NOTEFADE		0x0800
#define CHN_LPINGPONG		0x1000

#define CLIPINT(num, min, max) \
	if (num < min) \
		num = min; \
	else if (num > max) \
		num = max

template<typename T> inline void clipInt(T &num, const T min, const T max)
{
	if (num < min)
		num = min;
	else if (num > max)
		num = max;
}

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
	e = (uint64_t)a * (uint64_t)b;
	if ((uint64_t)c < e)
		result = (int32_t)(e / (uint64_t)c);
	else
		result = 0x7FFFFFFF;
	if (d < 0)
		result = -result;
	return result;
}

#endif /*__moduleMixer_H__*/
