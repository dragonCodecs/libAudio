#include <stdio.h>
#include "fixedPoint.h"

fixed64_t::fixed64_t(uint32_t a) : d(0)
{
	i = (int32_t)a;
}

fixed64_t::fixed64_t(int32_t a, uint32_t b) : i(a), d(b)
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
	double e = ((double)i) + ((double)d) / 4294967295.0;
	double f = ((double)b.i) + ((double)b.d) / 4294967295.0;
	double g = e * f;
	return fixed64_t((int32_t)g, (uint32_t)((g - ((int32_t)g)) * 4294967295.0));
}

fixed64_t &fixed64_t::operator *=(const fixed64_t &b)
{
	double e = ((double)i) + ((double)d) / 4294967295.0;
	double f = ((double)b.i) + ((double)b.d) / 4294967295.0;
	double g = e * f;
	i = (int32_t)g;
	d = (uint32_t)((g - ((int32_t)g)) * 4294967295.0);
	return *this;
}

fixed64_t fixed64_t::operator /(const fixed64_t &b) const
{
	double e = ((double)i) + ((double)d) / 4294967295.0;
	double f = ((double)b.i) + ((double)b.d) / 4294967295.0;
	double g = (f == 0 ? 0 : e / f);
	return fixed64_t((int32_t)g, (uint32_t)((g - ((int32_t)g)) * 4294967295.0));
}

fixed64_t fixed64_t::operator +(const fixed64_t &b) const
{
	uint64_t e = (((int64_t)i) << 32) | d;
	uint64_t f = (((int64_t)b.i) << 32) | b.d;
	uint64_t g = e + f;
	return fixed64_t(g >> 32, g & 0xFFFFFFFF);
}

fixed64_t &fixed64_t::operator +=(const fixed64_t &b)
{
	uint64_t e = (((int64_t)i) << 32) | d;
	uint64_t f = (((int64_t)b.i) << 32) | b.d;
	uint64_t g = e + f;
	i = g >> 32;
	d = g & 0xFFFFFFFF;
	return *this;
}

fixed64_t::operator int()
{
	return i;
}
