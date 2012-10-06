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

#endif /*__moduleMixer_H__*/
