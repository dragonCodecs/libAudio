#include <stdio.h>
#include <math.h>
#include <limits>
#include "fixedPoint.h"

#ifdef __GNUC__
#define unlikely(x) __builtin_expect(x, 0)
#else
#define unlikely(x) x
#endif

fixed64_t::fixed64_t(uint32_t a, uint32_t b, int8_t c) : i(a), d(b), sign(c)
{
}

fixed64_t fixed64_t::exp()
{
	fixed64_t ret(1), x(1);
	uint8_t i;
	uint64_t j;
	for (i = 1, j = 1; i <= 32; i++)
	{
		j *= i;
		x *= *this;
		ret += x / fixed64_t(j);
	}
	return ret;
}

fixed64_t fixed64_t::pow2()
{
	const fixed64_t ln2(0, 2977044472U); // ln(2) to 9dp
	return (*this * ln2).exp();
}

fixed64_t fixed64_t::operator *(const fixed64_t &b) const
{
	uint64_t e = ((uint64_t)b.i) * ((uint64_t)d);
	uint64_t f = ((uint64_t)i) * ((uint64_t)b.d);
	uint32_t g = (((uint64_t)d) * ((uint64_t)b.d)) >> 32;
	uint32_t h = ((uint64_t)i) * ((uint64_t)b.i) + (e >> 32) + (f >> 32);
	g += (e & 0xFFFFFFFF) + (f & 0xFFFFFFFF);
//	printf("% 2.9f * % 2.9f ?= % 2.9f\n", operator double(), b.operator double(), fixed64_t(h, g, sign * b.sign).operator double());
	return fixed64_t(h, g, sign * b.sign);
}

fixed64_t &fixed64_t::operator *=(const fixed64_t &b)
{
	uint64_t e = ((uint64_t)b.i) * ((uint64_t)d);
	uint64_t f = ((uint64_t)i) * ((uint64_t)b.d);
	uint32_t g = (((uint64_t)d) * ((uint64_t)b.d)) >> 32;
	sign *= b.sign;
	i = i * b.i + (e >> 32) + (f >> 32);
	d = (e & 0xFFFFFFFF) + (f & 0xFFFFFFFF) + g;
	return *this;
}

// Quick and dirty code, it makes mistakes on occasion, but pumps the right sequence out for it's use
fixed64_t fixed64_t::operator /(const fixed64_t &b) const
{
	uint8_t g;
	uint32_t q_i, q_d = 0;
	uint64_t e = (((uint64_t)i) << 32) | ((uint64_t)d);
	uint64_t f = (((uint64_t)b.i) << 32) | ((uint64_t)b.d);

	if (e == 0)
		return fixed64_t(0);

	q_i = e / f;
//	printf("%llu %d\t", e, q_i);
	e -= q_i * f;
//	printf("%llu %llu", q_i * f, e);
	g = 0;
	while (e > 0 && f > 0 && g < 32)
	{
		q_d <<= 1;
		if (e >= f)
		{
			e -= f;
			q_d |= 1;
		}
		f >>= 1;
		g++;
	}
//	printf("\t% 2.9f\n", fixed64_t(q_i, q_d << (33 - g), sign * b.sign).operator double());
	return fixed64_t(q_i, q_d << (33 - g), sign * b.sign);
}

// Quick and dirty code, it makes mistakes on occasion, but pumps the right sequence out for it's use
fixed64_t &fixed64_t::operator /=(const fixed64_t &b)
{
	uint8_t g;
	uint64_t e = (((uint64_t)i) << 32) | ((uint64_t)d);
	uint64_t f = (((uint64_t)b.i) << 32) | ((uint64_t)b.d);

	if (e == 0)
	{
		i = 0;
		d = 0;
		return *this;
	}

	sign *= b.sign;
	d = 0;
	i = e / f;
	g = 0;
	while (e > 0 && f > 0 && g < 32)
	{
		d <<= 1;
		if (e >= f)
		{
			e -= f;
			d |= 1;
		}
		f >>= 1;
		g++;
	}
	d <<= (33 - g);
	return *this;
}

fixed64_t fixed64_t::operator +(const fixed64_t &b) const
{
	if (sign != b.sign)
	{
		bool overflow = false;
		int64_t decimal = ((int64_t)d) - ((int64_t)b.d);
		int64_t integer = ((int64_t)i) - ((int64_t)b.i);
		if (sign < 0)
		{
			decimal = -decimal;
			integer = -integer;
		}
		if (decimal < 0)
		{
			overflow = true;
			decimal = (1LL << 32) + decimal;
		}
		return fixed64_t((integer < 0 ? -integer : integer) - (overflow == true ? 1 : 0),
			decimal, (integer < 0 ? -1 : 1));
	}
	else
	{
		uint32_t decimal = d + b.d;
		return fixed64_t(i + b.i + (decimal < (d | b.d) ? 1 : 0), decimal, sign);
	}
}

fixed64_t &fixed64_t::operator +=(const fixed64_t &b)
{
	if (sign != b.sign)
	{
		bool overflow = false;
		int64_t decimal = ((int64_t)d) - ((int64_t)b.d);
		int64_t integer = ((int64_t)i) - ((int64_t)b.i);
		if (sign < 0)
		{
			decimal = -decimal;
			integer = -integer;
		}
		if (decimal < 0)
		{
			overflow = true;
			decimal = (1LL << 32) + decimal;
		}
		sign = (integer < 0 ? -1 : 1);
		i = (integer < 0 ? -integer : integer) - (overflow == true ? 1 : 0);
		d = decimal;
	}
	else
	{
		uint32_t decimal = d + b.d;
		bool overflow = decimal < (d | b.d);
		i += b.i + (overflow == true ? 1 : 0);
		d = decimal;
	}
	return *this;
}

uint8_t fixed64_t::ulog2(uint64_t value) const noexcept
{
		if (unlikely(!value))
				return std::numeric_limits<uint8_t>::max();
#if defined(__ICC)
		return (sizeof(uint8_t) * 8) - _lzcnt_u64(value);
#elif defined(__GNUC__)
		return (sizeof(uint8_t) * 8) - __builtin_clzl(value);
#elif defined(_MSC_VER)
		return (sizeof(uint8_t) * 8) - __lzcnt64(value);
#else
		uint8_t result = 0;
		if (value <= UINT64_C(0x00000000FFFFFFFF))
				result += 32, value <<= 32;
		if (value <= UINT64_C(0x0000FFFFFFFFFFFF))
				result += 16, value <<= 16;
		if (value <= UINT64_C(0x00FFFFFFFFFFFFFF))
				result += 8, value <<= 8;
		if (value <= UINT64_C(0x0FFFFFFFFFFFFFFF))
				result += 4, value <<= 4;
		if (value <= UINT64_C(0x3FFFFFFFFFFFFFFF))
				result += 2, value <<= 2;
		if (value <= UINT64_C(0x7FFFFFFFFFFFFFFF))
				++result;
		return (sizeof(uint8_t) * 8) - result;
#endif
}

fixed64_t::operator int() const
{
	return sign * (i + (d >> 31));
}

fixed64_t::operator uint32_t() const
{
	return i + (d >> 31);
}

fixed64_t::operator double() const
{
	return sign * (double)((((uint64_t)i) << 32) | (int64_t)((uint64_t)d)) / 4294967296.0;
}

