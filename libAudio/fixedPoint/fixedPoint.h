// SPDX-License-Identifier: BSD-3-Clause
#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <cstdint>

struct fixed64_t final
{
private:
	uint32_t i;
	uint32_t d;
	int8_t sign;

private:
	uint8_t ulog2(uint64_t n) const noexcept;

public:
	constexpr fixed64_t(const uint32_t a, const uint32_t b = 0, const int8_t _sign = 1) noexcept :
		i{a}, d{b}, sign{_sign} { }

	fixed64_t exp() const noexcept;
	//fixed64_t ln();

	fixed64_t pow2() const noexcept;

	fixed64_t operator *(const fixed64_t &b) const;
	fixed64_t &operator *=(const fixed64_t &b);
	fixed64_t operator /(const fixed64_t &b) const;
	fixed64_t &operator /=(const fixed64_t &b);

	fixed64_t operator +(const fixed64_t &b) const;
	fixed64_t &operator +=(const fixed64_t &b);

	operator uint32_t() const;
	operator int32_t() const;
	operator int16_t() const;
	operator double() const;
};

inline fixed64_t operator *(const uint32_t a, const fixed64_t &b)
	{ return fixed64_t{a} * b; }
inline fixed64_t operator /(const uint8_t a, const fixed64_t &b)
	{ return fixed64_t{a} / b; }

#endif /*FIXED_POINT_H*/
