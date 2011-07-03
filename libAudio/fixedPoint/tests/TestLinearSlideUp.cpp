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
const uint32_t LinearSlideUpTable[256] =
{
	65536, 65773, 66010, 66249, 66489, 66729, 66971, 67213,
	67456, 67700, 67945, 68190, 68437, 68685, 68933, 69182,
	69432, 69684, 69936, 70189, 70442, 70697, 70953, 71209,
	71467, 71725, 71985, 72245, 72507, 72769, 73032, 73296,
	73561, 73827, 74094, 74362, 74631, 74901, 75172, 75444,
	75717, 75991, 76265, 76541, 76818, 77096, 77375, 77655,
	77935, 78217, 78500, 78784, 79069, 79355, 79642, 79930,
	80219, 80509, 80800, 81093, 81386, 81680, 81976, 82272,
	82570, 82868, 83168, 83469, 83771, 84074, 84378, 84683,
	84989, 85297, 85605, 85915, 86225, 86537, 86850, 87164,
	87480, 87796, 88113, 88432, 88752, 89073, 89395, 89718,
	90043, 90369, 90695, 91023, 91353, 91683, 92015, 92347,
	92681, 93017, 93353, 93691, 94029, 94370, 94711, 95053,
	95397, 95742, 96088, 96436, 96785, 97135, 97486, 97839,
	98193, 98548, 98904, 99262, 99621, 99981, 100343, 100706,
	101070, 101435, 101802, 102170, 102540, 102911, 103283, 103657,
	104031, 104408, 104785, 105164, 105545, 105926, 106309, 106694,
	107080, 107467, 107856, 108246, 108637, 109030, 109425, 109820,
	110217, 110616, 111016, 111418, 111821, 112225, 112631, 113038,
	113447, 113857, 114269, 114682, 115097, 115514, 115931, 116351,
	116771, 117194, 117618, 118043, 118470, 118898, 119328, 119760,
	120193, 120628, 121064, 121502, 121941, 122382, 122825, 123269,
	123715, 124162, 124611, 125062, 125514, 125968, 126424, 126881,
	127340, 127801, 128263, 128727, 129192, 129660, 130129, 130599,
	131072, 131546, 132021, 132499, 132978, 133459, 133942, 134426,
	134912, 135400, 135890, 136381, 136875, 137370, 137866, 138365,
	138865, 139368, 139872, 140378, 140885, 141395, 141906, 142419,
	142935, 143451, 143970, 144491, 145014, 145538, 146064, 146593,
	147123, 147655, 148189, 148725, 149263, 149803, 150344, 150888,
	151434, 151982, 152531, 153083, 153637, 154192, 154750, 155310,
	155871, 156435, 157001, 157569, 158138, 158710, 159284, 159860,
	160439, 161019, 161601, 162186, 162772, 163361, 163952, 164545
};

inline uint32_t LinearSlideUp(uint8_t slide)
{
	const fixed64_t c192(192);
	const fixed64_t c65536(65536);
	fixed64_t ret = (fixed64_t(slide) / c192).pow2() * c65536;
	return ret.operator int();
}

inline uint32_t unsign(int32_t num)
{
	return (num < 0 ? -(uint32_t)num : num);
}

int main(int argc, char **argv)
{
	uint16_t i, pass, warn;
	fixed64_t a(4), b(1, 2147483648ULL), c(3), d(1), e(192);
	GetConsoleWidth();
	for (pass = 0, warn = 0, i = 0; i < 256; i++)
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

	printf("%f, %f\n", (a / b).operator double(), (b / a).operator double());
	printf("%f, %f\n", (a / c).operator double(), (c / a).operator double());
	printf("%f, %f\n", (c / b).operator double(), (b / c).operator double());
	printf("%f\n", (d / e).operator double());

	printf("Results:\n"
		"\t" COLOUR_SUCCESS "PASS" COLOUR_NORMAL ": " COLOUR_BRACKET "%u" NEWLINE
		"\t" COLOUR_WARNING "WARN" COLOUR_NORMAL ": " COLOUR_BRACKET "%u" NEWLINE
		"\t" COLOUR_FAILURE "FAIL" COLOUR_NORMAL ": " COLOUR_BRACKET "%u" NEWLINE, pass, warn, i - warn - pass);
	return 0;
}
