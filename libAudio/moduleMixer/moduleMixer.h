#ifndef libAudio_moduleMixer__H
#define libAudio_moduleMixer__H

#include "../fixedPoint/fixedPoint.h"

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
#define CHN_FPINGPONG		0x2000
#define CHN_SURROUND		0x4000

template<typename T> inline void clipInt(T &num, const T min, const T max)
{
	if (num < min)
		num = min;
	else if (num > max)
		num = max;
}

template<> inline void clipInt<uint8_t>(uint8_t &num, const uint8_t min, const uint8_t max)
{
	if ((num & 0x80U) != 0 || (min != 0 && num < min))
		num = min;
	else if (num > max)
		num = max;
}

template<> inline void clipInt<uint16_t>(uint16_t &num, const uint16_t min, const uint16_t max)
{
	if ((num & 0x8000U) != 0 || (min != 0 && num < min))
		num = min;
	else if (num > max)
		num = max;
}

template<> inline void clipInt<uint32_t>(uint32_t &num, const uint32_t min, const uint32_t max)
{
	if ((num & 0x80000000U) != 0 || (min != 0 && num < min))
		num = min;
	else if (num > max)
		num = max;
}

// Return (a * b) / c [ - no divide error ]
inline int32_t muldiv(int32_t a, int32_t b, int32_t c)
{
	int32_t result;
	int32_t d = a;
	if (a < 0)
		a = -a;
	d ^= b;
	if (b < 0)
		b = -b;
	d ^= c;
	if (c < 0)
		c = -c;
	const uint64_t e = uint64_t(a) * uint64_t(b);
	if (uint64_t(c) < e)
		result = int32_t(e / uint64_t(c));
	else
		return 0x7FFFFFFF;
	if (d < 0)
		result = -result;
	return result;
}

inline int32_t linearSlideUp(uint8_t slide) noexcept
{
	constexpr fixed64_t c192{192};
	constexpr fixed64_t c65536{65536};
	return (slide / c192).pow2() * c65536;
}

// Returns ((period * 65536 * 2^(slide / 192)) + 32768) / 65536 using fixed-point maths
inline uint32_t linearSlideUp(uint32_t period, uint8_t slide) noexcept
{
	const fixed64_t c192(192);
	const fixed64_t c32768(32768);
	const fixed64_t c65536(65536);
	return ((period * (slide / c192).pow2() * c65536) + c32768) / c65536;
}

inline int32_t linearSlideDown(uint8_t slide) noexcept
{
	constexpr fixed64_t c192{192};
	constexpr fixed64_t c65535{65535};
	return (fixed64_t{slide, 0, -1} / c192).pow2() * c65535;
}

// Returns ((period * 65535 * 2^(-slide / 192)) + 32768) / 65536 using fixed-point maths
inline uint32_t linearSlideDown(uint32_t period, uint8_t slide) noexcept
{
	const fixed64_t c192(192);
	const fixed64_t c32768(32768);
	const fixed64_t c65535(65535);
	const fixed64_t c65536(65536);
	return ((period * (fixed64_t(slide, 0, -1) / c192).pow2() * c65535) + c32768) / c65536;
}

// Returns ((period * 65536 * 2^((slide / 4) / 192)) + 32768) / 65536 using fixed-point maths
inline uint32_t fineLinearSlideUp(uint32_t period, uint8_t slide)
{
	const fixed64_t c4(4);
	const fixed64_t c192(192);
	const fixed64_t c32768(32768);
	const fixed64_t c65536(65536);
	return ((period * ((slide / c4) / c192).pow2() * c65536) + c32768) / c65536;
}

// Returns ((period * 65535 * 2^((-slide / 4) / 192)) + 32768) / 65536 using fixed-point maths
inline uint32_t fineLinearSlideDown(uint32_t period, uint8_t slide)
{
	const fixed64_t c4(4);
	const fixed64_t c192(192);
	const fixed64_t c32768(32768);
	const fixed64_t c65535(65535);
	const fixed64_t c65536(65536);
	return ((period * ((fixed64_t(slide, 0, -1) / c4) / c192).pow2() * c65535) + c32768) / c65536;
}

#endif /*libAudio_moduleMixer__H*/
