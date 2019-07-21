#ifndef __fixedPoint_H__
#define __fixedPoint_H__

#include <inttypes.h>

class fixed64_t
{
private:
	uint32_t i;
	uint32_t d;
	int8_t sign;

private:
	uint8_t ulog2(uint64_t n) const noexcept;

public:
	constexpr fixed64_t(uint32_t a, uint32_t b = 0, int8_t _sign = 1) noexcept : i{a}, d{b}, sign{_sign} { }

	fixed64_t exp();
	//fixed64_t ln();

	fixed64_t pow2();

	fixed64_t operator *(const fixed64_t &b) const;
	fixed64_t &operator *=(const fixed64_t &b);
	fixed64_t operator /(const fixed64_t &b) const;
	fixed64_t &operator /=(const fixed64_t &b);

	fixed64_t operator +(const fixed64_t &b) const;
	fixed64_t &operator +=(const fixed64_t &b);

	operator int() const;
	operator uint32_t() const;
	operator double() const;
};

#endif /*__fixedPoint_H__*/
