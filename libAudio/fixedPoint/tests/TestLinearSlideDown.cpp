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
const uint16_t LinearSlideDownTable[256] =
{
	65535, 65298, 65063, 64829, 64595, 64362, 64130, 63899,
	63669, 63439, 63211, 62983, 62756, 62530, 62304, 62080,
	61856, 61633, 61411, 61190, 60969, 60750, 60531, 60313,
	60095, 59879, 59663, 59448, 59234, 59020, 58808, 58596,
	58384, 58174, 57964, 57756, 57547, 57340, 57133, 56927,
	56722, 56518, 56314, 56111, 55909, 55708, 55507, 55307,
	55107, 54909, 54711, 54514, 54317, 54122, 53927, 53732,
	53539, 53348, 53155, 52964, 52773, 52583, 52393, 52205,
	52014, 51827, 51640, 51454, 51269, 51084, 50900, 50716,
	50534, 50352, 50170, 49989, 49809, 49630, 49451, 49273,
	49095, 48918, 48742, 48566, 48391, 48217, 48043, 47870,
	47697, 47525, 47354, 47184, 47013, 46844, 46675, 46507,
	46339, 46172, 46006, 45840, 45675, 45510, 45346, 45183,
	45020, 44858, 44696, 44535, 44375, 44215, 44055, 43897,
	43739, 43581, 43424, 43267, 43111, 42956, 42801, 42647,
	42493, 42340, 42188, 42036, 41884, 41733, 41583, 41433,
	41284, 41135, 40987, 40839, 40692, 40545, 40399, 40253,
	40108, 39964, 39820, 39676, 39533, 39391, 39249, 39107,
	38966, 38826, 38686, 38547, 38408, 38269, 38131, 37994,
	37857, 37721, 37585, 37449, 37314, 37180, 37046, 36912,
	36779, 36647, 36515, 36383, 36252, 36121, 35991, 35861,
	35732, 35603, 35475, 35347, 35220, 35093, 34967, 34841,
	34715, 34590, 34465, 34341, 34217, 34094, 33971, 33849,
	33727, 33605, 33484, 33363, 33243, 33123, 33004, 32885,
	32767, 32648, 32531, 32414, 32297, 32180, 32064, 31949,
	31834, 31719, 31605, 31491, 31377, 31264, 31151, 31039,
	30927, 30816, 30705, 30594, 30484, 30374, 30265, 30156,
	30047, 29939, 29831, 29723, 29616, 29509, 29403, 29297,
	29191, 29086, 28981, 28877, 28773, 28669, 28566, 28463,
	28360, 28258, 28156, 28055, 27954, 27853, 27753, 27653,
	27553, 27454, 27355, 27256, 27158, 27060, 26963, 26865,
	26769, 26672, 26576, 26480, 26385, 26290, 26195, 26101
};

inline uint32_t LinearSlideDown(uint8_t slide)
{
	const fixed64_t c192(192);
	const fixed64_t c65535(65535);
	fixed64_t ret = (fixed64_t(slide, 0, -1) / c192).pow2() * c65535;
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
	for (pass = 0, warn = 0, i = 0; i < 256; i++)
	{
		uint32_t j = LinearSlideDown(i);
		printf("%u | %u\n", j, LinearSlideDownTable[i]);
		if (j != LinearSlideDownTable[i])
		{
			if (unsign(LinearSlideDownTable[i] - j) < 3)
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
