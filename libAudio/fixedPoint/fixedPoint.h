#ifndef __fixedPoint_H__
#define __fixedPoint_H__

#include <inttypes.h>

class fixed64_t
{
private:
	int32_t i;
	uint32_t d;

public:
	fixed64_t(uint32_t a);
	fixed64_t(int32_t a, uint32_t b);

	fixed64_t exp();
	//fixed64_t ln();

	fixed64_t pow2();

	fixed64_t operator *(const fixed64_t &b) const;
	fixed64_t &operator *=(const fixed64_t &b);
	fixed64_t operator /(const fixed64_t &b) const;

	fixed64_t operator +(const fixed64_t &b) const;
	fixed64_t &operator += (const fixed64_t &b);

	operator int();
};

#endif /*__fixedPoint_H__*/
