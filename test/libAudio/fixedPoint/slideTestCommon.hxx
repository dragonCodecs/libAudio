#ifndef SLIDE_TEST_COMMON__HXX
#define SLIDE_TEST_COMMON__HXX

#include <fixedPoint/fixedPoint.h>
#include <crunch++.h>

namespace crunch { CRUNCHpp_API bool isTTY; }
#define COLOUR(Code) "\x1B[" Code "m"
#define NORMAL COLOUR("0;39")
#define WARNING COLOUR("1;33")
#define HIGHLIGHT COLOUR("1;34")
#define NEWLINE NORMAL "\n"

using crunch::isTTY;
size_t warnCount{0};

inline uint32_t unsign(int32_t num) noexcept
	{ return (num < 0 ? -uint32_t(num) : num); }

inline void displayWarning(const uint32_t compSlide, const uint32_t tableSlide) noexcept
{
	if (isTTY)
		printf(WARNING "Warning: %u != %u but within error margin" NEWLINE, compSlide, tableSlide);
	else
		printf("Warning: %u != %u but within error margin\n", compSlide, tableSlide);
	++warnCount;
}

inline void displayWarningCount() noexcept
{
	if (warnCount == 0)
		return;
	if (isTTY)
		printf(HIGHLIGHT "%zu warnings for suite" NEWLINE, warnCount);
	else
		printf("%zu warnings for suite\n", warnCount);
}

#endif /*SLIDE_TEST_COMMON__HXX*/
