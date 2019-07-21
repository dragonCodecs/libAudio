#ifndef SLIDE_TEST_COMMON__HXX
#define SLIDE_TEST_COMMON__HXX

#include <fixedPoint/fixedPoint.h>
#include <crunch++.h>

CRUNCHpp_API bool isTTY;
#define COLOUR(Code) "\x1B[" Code "m"
#define NORMAL COLOUR("0;39")
#define WARNING COLOUR("1;33")
#define NEWLINE NORMAL "\n"

inline uint32_t unsign(int32_t num) noexcept
	{ return (num < 0 ? -uint32_t(num) : num); }

inline void displayWarning(const uint32_t compSlide, const uint32_t tableSlide) noexcept
{
	if (isTTY)
		printf(WARNING "Warning: %u != %u but within error margin" NEWLINE, compSlide, tableSlide);
	else
		printf("Warning: %u != %u but within error margin\n", compSlide, tableSlide);
}

#endif /*SLIDE_TEST_COMMON__HXX*/