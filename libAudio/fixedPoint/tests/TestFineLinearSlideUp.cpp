#include <stdio.h>
#include "../fixedPoint.h"
#include <sys/ioctl.h>
#include <unistd.h>

uint32_t ConsoleWidth;
	#define COLOUR(Code) "\033[" Code "m"
	#define COLOUR_NORMAL COLOUR("0;39")
	#define COLOUR_SUCCESS COLOUR("1;32")
	#define COLOUR_WARNING COLOUR("1;33")
	#define COLOUR_FAILURE COLOUR("1;31")
	#define COLOUR_BRACKET COLOUR("1;34")
	#define CURS_UP "\033[1A\033[0G"
	#define SET_COL "\033[%uG"
	#define NEWLINE	"\n" COLOUR_NORMAL
	#define PRINT_RESULT(Colour, Text) printf(CURS_UP SET_COL COLOUR_BRACKET "[" Colour " " Text " " COLOUR_BRACKET "]" NEWLINE, ConsoleWidth)
	#define PRINT_PASS() \
	{ \
		PRINT_RESULT(COLOUR_SUCCESS, "PASS"); \
		pass++; \
	}
	#define PRINT_WARN() \
	{ \
		PRINT_RESULT(COLOUR_WARNING, "WARN"); \
		warn++; \
	}
	#define PRINT_FAIL() PRINT_RESULT(COLOUR_FAILURE, "FAIL")
void GetConsoleWidth()
{
	struct winsize win;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &win);
	ConsoleWidth = win.ws_col;
	if (ConsoleWidth == 0)
		ConsoleWidth = 80;
	ConsoleWidth -= 8;
}

/* The following function generates this */
const uint32_t LinearSlideUpTable[16] =
{
	65536, 65595, 65654, 65714,	65773, 65832, 65892, 65951,
	66011, 66071, 66130, 66190, 66250, 66309, 66369, 66429
};

inline uint32_t LinearSlideUp(uint8_t slide)
{
	const fixed64_t c4(4);
	const fixed64_t c192(192);
	const fixed64_t c65536(65536);
	fixed64_t ret = ((fixed64_t(slide) / c4) / c192).pow2() * c65536;
	return ret.operator int();
}

inline uint32_t unsign(int32_t num)
{
	return (num < 0 ? -(uint32_t)num : num);
}

int main(int argc, char **argv)
{
	uint16_t i, pass, warn;
	GetConsoleWidth();
	for (pass = 0, warn = 0, i = 0; i < 16; i++)
	{
		uint32_t j = LinearSlideUp(i);
		printf("%u | %u\n", j, LinearSlideUpTable[i]);
		if (j != LinearSlideUpTable[i])
		{
			if (unsign(LinearSlideUpTable[i] - j) < 2)
				PRINT_WARN()
			else
				PRINT_FAIL();
		}
		else
			PRINT_PASS();
	}

	printf("Results:\n"
		"\t" COLOUR_SUCCESS "PASS" COLOUR_NORMAL ": " COLOUR_BRACKET "%u" NEWLINE
		"\t" COLOUR_WARNING "WARN" COLOUR_NORMAL ": " COLOUR_BRACKET "%u" NEWLINE
		"\t" COLOUR_FAILURE "FAIL" COLOUR_NORMAL ": " COLOUR_BRACKET "%u" NEWLINE, pass, warn, i - warn - pass);
	return 0;
}
