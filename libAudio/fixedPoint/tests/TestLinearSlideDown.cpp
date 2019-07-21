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
	65535, 65299, 65064, 64830, 64596, 64363, 64131, 63900,
	63670, 63440, 63212, 62984, 62757, 62531, 62305, 62081,
	61857, 61634, 61412, 61191, 60970, 60751, 60532, 60314,
	60096, 59880, 59664, 59449, 59235, 59021, 58809, 58597,
	58385, 58175, 57965, 57757, 57548, 57341, 57134, 56928,
	56723, 56519, 56315, 56112, 55910, 55709, 55508, 55308,
	55108, 54910, 54712, 54515, 54318, 54123, 53928, 53733,
	53540, 53347, 53154, 52963, 52772, 52582, 52392, 52204,
	52015, 51828, 51641, 51455, 51270, 51085, 50901, 50717,
	50535, 50353, 50171, 49990, 49810, 49631, 49452, 49274,
	49096, 48919, 48743, 48567, 48392, 48218, 48044, 47871,
	47698, 47526, 47355, 47185, 47014, 46845, 46676, 46508,
	46340, 46173, 46007, 45841, 45676, 45511, 45347, 45184,
	45021, 44859, 44697, 44536, 44376, 44216, 44056, 43898,
	43740, 43582, 43425, 43268, 43112, 42957, 42802, 42648,
	42494, 42341, 42189, 42037, 41885, 41734, 41584, 41434,
	41285, 41136, 40988, 40840, 40693, 40546, 40400, 40254,
	40109, 39965, 39821, 39677, 39534, 39392, 39250, 39108,
	38967, 38827, 38687, 38548, 38409, 38270, 38132, 37995,
	37858, 37722, 37586, 37450, 37315, 37181, 37047, 36913,
	36780, 36648, 36516, 36384, 36253, 36122, 35992, 35862,
	35733, 35604, 35476, 35348, 35221, 35094, 34968, 34842,
	34716, 34591, 34466, 34342, 34218, 34095, 33972, 33850,
	33728, 33606, 33485, 33364, 33244, 33124, 33005, 32886,
	32768, 32649, 32532, 32415, 32298, 32181, 32065, 31950,
	31835, 31720, 31606, 31492, 31378, 31265, 31152, 31040,
	30928, 30817, 30706, 30595, 30485, 30375, 30266, 30157,
	30048, 29940, 29832, 29724, 29617, 29510, 29404, 29298,
	29192, 29087, 28982, 28878, 28774, 28670, 28567, 28464,
	28361, 28259, 28157, 28056, 27955, 27854, 27754, 27654,
	27554, 27455, 27356, 27257, 27159, 27061, 26964, 26866,
	26770, 26673, 26577, 26481, 26386, 26291, 26196, 26102
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
			if (unsign(LinearSlideDownTable[i] - j) < 2)
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
