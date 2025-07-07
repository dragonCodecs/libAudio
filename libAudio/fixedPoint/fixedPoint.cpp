// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2012-2023 Rachel Mant <git@dragonmux.network>
#include <cmath>
#include <limits>
#include "fixedPoint.h"
//#include "console.hxx"

#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifdef __GNUC__
#define unlikely(x) __builtin_expect(x, 0)
#else
#define unlikely(x) x
#endif

fixed64_t fixed64_t::exp() const noexcept
{
	fixed64_t ret{1};
	fixed64_t x{1};
	uint32_t offset{1};
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	for (uint8_t bit{1}; bit <= 32U; ++bit)
	{
		offset *= bit;
		x *= *this;
		ret += x / fixed64_t{offset};
	}
	return ret;
}

fixed64_t fixed64_t::pow2() const noexcept
{
	constexpr fixed64_t ln2{0, 2977044472U}; // ln(2) to 9dp
	return (*this * ln2).exp();
}

fixed64_t fixed64_t::operator *(const fixed64_t &b) const
{
	uint64_t e = uint64_t{b.i} * uint64_t{d};
	uint64_t f = uint64_t{i} * uint64_t{b.d};
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	uint32_t g = (uint64_t{d} * uint64_t{b.d}) >> 32U;
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	uint32_t h = uint64_t{i} * uint64_t{b.i} + uint32_t(e >> 32U) + uint32_t(f >> 32U);
	g += uint32_t(e) + uint32_t(f);
//	printf("% 2.9f * % 2.9f ?= % 2.9f\n", operator double(), b.operator double(), fixed64_t(h, g, sign * b.sign).operator double());
	return {h, g, int8_t(sign * b.sign)};
}

fixed64_t &fixed64_t::operator *=(const fixed64_t &b)
{
	uint64_t e = uint64_t{b.i} * uint64_t{d};
	uint64_t f = uint64_t{i} * uint64_t{b.d};
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	uint32_t g = (uint64_t{d} * uint64_t{b.d}) >> 32U;
	sign *= b.sign;
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	i = i * b.i + uint32_t(e >> 32U) + uint32_t(f >> 32U);
	d = uint32_t(e) + uint32_t(f) + g;
	return *this;
}

// Quick and dirty code, it makes mistakes on occasion, but pumps the right sequence out for it's use
fixed64_t fixed64_t::operator /(const fixed64_t &b) const
{
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	uint64_t e = (uint64_t{i} << 32U) | uint64_t{d};
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	uint64_t f = (uint64_t{b.i} << 32U) | uint64_t{b.d};

	if (e == 0)
		return {0};

	auto q_i = uint32_t(e / f);
//	printf("%llu %d\t", e, q_i);
	e -= q_i * f;
//	printf("%llu %llu", q_i * f, e);
	uint8_t g{};
	uint32_t q_d{};
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	while (e > 0U && f > 0U && g < 32U)
	{
		q_d <<= 1U;
		if (e >= f)
		{
			e -= f;
			q_d |= 1U;
		}
		f >>= 1U;
		g++;
	}
//	printf("\t% 2.9f\n", fixed64_t(q_i, q_d << (33 - g), sign * b.sign).operator double());
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	return {q_i, uint32_t(uint64_t{q_d} << (33U - g)), int8_t(sign * b.sign)};
}

// Quick and dirty code, it makes mistakes on occasion, but pumps the right sequence out for it's use
fixed64_t &fixed64_t::operator /=(const fixed64_t &b)
{
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	uint64_t e = (uint64_t{i} << 32U) | uint64_t{d};
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	uint64_t f = (uint64_t{b.i} << 32U) | uint64_t{b.d};

	if (e == 0)
	{
		i = 0;
		d = 0;
		return *this;
	}

	sign *= b.sign;
	d = 0;
	i = uint32_t(e / f);
	uint8_t g{};
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	while (e > 0U && f > 0U && g < 32U)
	{
		d <<= 1U;
		if (e >= f)
		{
			e -= f;
			d |= 1U;
		}
		f >>= 1U;
		++g;
	}
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	d <<= 1U;
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	d <<= 32U - g;
	return *this;
}

fixed64_t fixed64_t::operator +(const fixed64_t &b) const
{
	if (sign != b.sign)
	{
		int64_t decimal = int64_t{d} - int64_t{b.d};
		int64_t integer = int64_t{i} - int64_t{b.i};
		if (sign < 0)
		{
			decimal = -decimal;
			integer = -integer;
		}
		const bool overflow = decimal < 0;
		if (overflow)
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
			decimal = (1ULL << 32U) + decimal;
		return {uint32_t(integer < 0 ? -integer : integer) - (overflow ? 1U : 0U),
			uint32_t(decimal), int8_t(integer < 0 ? -1 : 1)};
	}
	else
	{
		const uint32_t decimal = d + b.d;
		return {i + b.i + (decimal < d ? 1 : 0), decimal, sign};
	}
}

fixed64_t &fixed64_t::operator +=(const fixed64_t &b)
{
	if (sign != b.sign)
	{
		int64_t decimal = int64_t{d} - int64_t{b.d};
		int64_t integer = int64_t{i} - int64_t{b.i};
		if (sign < 0)
		{
			decimal = -decimal;
			integer = -integer;
		}
		const bool overflow = decimal < 0;
		if (overflow)
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
			decimal = (1ULL << 32U) + decimal;
		sign = (integer < 0 ? -1 : 1);
		i = uint32_t(integer < 0 ? -integer : integer) - (overflow ? 1 : 0);
		d = uint32_t(decimal);
	}
	else
	{
		const uint32_t decimal = d + b.d;
		const bool overflow = decimal < d;
		i += b.i + (overflow ? 1 : 0);
		d = decimal;
	}
	return *this;
}

uint8_t fixed64_t::ulog2(uint64_t value) const noexcept
{
		if (unlikely(!value))
				return std::numeric_limits<uint8_t>::max();
#if defined(__ICC)
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
		return uint8_t(sizeof(uint8_t) * 8U) - uint8_t(_lzcnt_u64(value));
#elif defined(__GNUC__)
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
		return uint8_t(sizeof(uint8_t) * 8U) - uint8_t(__builtin_clzl(value));
#elif defined(_MSC_VER)
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
		return uint8_t(sizeof(uint8_t) * 8U) - uint8_t(__lzcnt64(value));
#else
		uint8_t result{};
		if (value <= UINT64_C(0x00000000FFFFFFFF))
				result += 32, value <<= 32U;
		if (value <= UINT64_C(0x0000FFFFFFFFFFFF))
				result += 16, value <<= 16U;
		if (value <= UINT64_C(0x00FFFFFFFFFFFFFF))
				result += 8, value <<= 8U;
		if (value <= UINT64_C(0x0FFFFFFFFFFFFFFF))
				result += 4, value <<= 4U;
		if (value <= UINT64_C(0x3FFFFFFFFFFFFFFF))
				result += 2, value <<= 2U;
		if (value <= UINT64_C(0x7FFFFFFFFFFFFFFF))
				++result;
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
		return uint8_t(sizeof(uint8_t) * 8U) - result;
#endif
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
fixed64_t::operator uint32_t() const { return i + (d >> 31U); }
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
fixed64_t::operator int32_t() const { return sign * (i + (d >> 31U)); }
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
fixed64_t::operator int16_t() const { return static_cast<int16_t>(sign * (i + (d >> 31U))); }
fixed64_t::operator double() const
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	{ return sign * double((uint64_t{i} << 32U) | d) / 4294967296.0; }
