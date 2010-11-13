#define _USE_MATH_DEFINES
#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <windows.h>
#endif
#include <assert.h>
#include <math.h>

#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "../ImpulseTracker.h"

#define SONG_EMBEDMIDICFG		0x000001
#define SONG_FASTVOLSLIDES		0x000002
#define SONG_ITOLDEFFECTS		0x000004
#define SONG_ITCOMPATMODE		0x000008
#define SONG_LINEARSLIDES		0x000010
#define SONG_PATTERNLOOP		0x000020
#define SONG_STEP				0x000040
#define SONG_PAUSED				0x000080
#define SONG_FADINGSONG			0x000100
#define SONG_ENDREACHED			0x000200
#define SONG_GLOBALFADE			0x000400
#define SONG_CPUVERYHIGH		0x000800
#define SONG_FIRSTTICK			0x001000
#define SONG_MPTFILTERMODE		0x002000
#define SONG_SURROUNDPAN		0x004000
#define SONG_EXFILTERRANGE		0x008000
#define SONG_AMIGALIMITS		0x010000
#define SONG_ITPROJECT			0x020000
#define SONG_ITPEMBEDIH			0x040000

#define CHN_16BIT				0x00000001
#define CHN_LOOP				0x00000002
#define CHN_PINGPONGLOOP		0x00000004
#define CHN_SUSTAINLOOP			0x00000008
#define CHN_PINGPONGSUSTAIN		0x00000010
#define CHN_PANNING				0x00000020
#define CHN_STEREO				0x00000040
#define	CHN_PINGPONGFLAG		0x00000080
#define CHN_MUTE				0x00000100
#define CHN_KEYOFF				0x00000200
#define CHN_NOTEFADE			0x00000400
#define CHN_SURROUND			0x00000800
#define CHN_NOIDO				0x00001000
#define CHN_HQSRC				0x00002000
#define CHN_FILTER				0x00004000
#define CHN_VOLUMERAMP			0x00008000
#define CHN_VIBRATO				0x00010000
#define CHN_TREMOLO				0x00020000
#define	CHN_PANBRELLO			0x00040000
#define CHN_PORTAMENTO			0x00080000
#define CHN_GLISSANDO			0x00100000
#define CHN_VOLENV				0x00200000
#define CHN_PANENV				0x00400000
#define CHN_PITCHENV			0x00800000
#define CHN_FASTVOLRAMP			0x01000000
#define CHN_EXTRALOUD			0x02000000
#define CHN_REVERB				0x04000000
#define CHN_NOREVERB			0x08000000
#define CHN_SOLO				0x10000000
#define CHN_NOFX				0x20000000

#define CMD_NONE				0
#define CMD_ARPEGGIO			1
#define CMD_PORTAMENTOUP		2
#define CMD_PORTAMENTODOWN		3
#define CMD_TONEPORTAMENTO		4
#define CMD_VIBRATO				5
#define CMD_TONEPORTAVOL		6
#define CMD_VIBRATOVOL			7
#define CMD_TREMOLO				8
#define CMD_PANNING8			9
#define CMD_OFFSET				10
#define CMD_VOLUMESLIDE			11
#define CMD_POSITIONJUMP		12
#define CMD_VOLUME				13
#define CMD_PATTERNBREAK		14
#define CMD_RETRIG				15
#define CMD_SPEED				16
#define CMD_TEMPO				17
#define CMD_TREMOR				18
#define CMD_MODCMDEX			19
#define CMD_S3MCMDEX			20
#define CMD_CHANNELVOLUME		21
#define CMD_CHANNELVOLSLIDE		22
#define CMD_GLOBALVOLUME		23
#define CMD_GLOBALVOLSLIDE		24
#define CMD_KEYOFF				25
#define CMD_FINEVIBRATO			26
#define CMD_PANBRELLO			27
#define CMD_XFINEPORTAUPDOWN	28
#define CMD_PANNINGSLIDE		29
#define CMD_SETENVPOSITION		30
#define CMD_MIDI				31
#define CMD_SMOOTHMIDI			32
#define CMD_VELOCITY			33
#define CMD_XPARAM				34

#define VOLCMD_VOLUME			1
#define VOLCMD_PANNING			2
#define VOLCMD_VOLSLIDEUP		3
#define VOLCMD_VOLSLIDEDOWN		4
#define VOLCMD_FINEVOLUP		5
#define VOLCMD_FINEVOLDOWN		6
#define VOLCMD_VIBRATOSPEED		7
#define VOLCMD_VIBRATO			8
#define VOLCMD_PANSLIDELEFT		9
#define VOLCMD_PANSLIDERIGHT	10
#define VOLCMD_TONEPORTAMENTO	11
#define VOLCMD_PORTAUP			12
#define VOLCMD_PORTADOWN		13
#define VOLCMD_VELOCITY			14
#define VOLCMD_OFFSET			15

#define NNA_NOTECUT				0
#define NNA_CONTINUE			1
#define NNA_NOTEOFF				2
#define NNA_NOTEFADE			3

#define DCT_NONE				0
#define DCT_NOTE				1
#define DCT_SAMPLE				2
#define DCT_INSTRUMENT			3
#define DCT_PLUGIN				4

#define DNA_NOTECUT				0
#define DNA_NOTEOFF				1
#define DNA_NOTEFADE			2

#define SYSMIX_ENABLEMMX		0x01
#define SYSMIX_SLOWCPU			0x02
#define SYSMIX_FASTCPU			0x04
#define SYSMIX_MMXEX			0x08
#define SYSMIX_3DNOW			0x10
#define SYSMIX_SSE				0x20

#define CHANNEL_ONLY			0
#define INSTRUMENT_ONLY			1
#define PRIORITISE_INSTRUMENT	2
#define PRIORITISE_CHANNEL		3
#define EVEN_IF_MUTED			false
#define RESPECT_MUTES			true

#define ENV_VOLUME				0x0001
#define ENV_VOLSUSTAIN			0x0002
#define ENV_VOLLOOP				0x0004
#define ENV_PANNING				0x0008
#define ENV_PANSUSTAIN			0x0010
#define ENV_PANLOOP				0x0020
#define ENV_PITCH				0x0040
#define ENV_PITCHSUSTAIN		0x0080
#define ENV_PITCHLOOP			0x0100
#define ENV_SETPANNING			0x0200
#define ENV_FILTER				0x0400
#define ENV_VOLCARRY			0x0800
#define ENV_PANCARRY			0x1000
#define ENV_PITCHCARRY			0x2000
#define ENV_MUTE				0x4000

#define FLTMODE_UNCHANGED		0xFF
#define FLTMODE_LOWPASS			0
#define FLTMODE_HIGHPASS		1
#define FLTMODE_BANDPASS		2

#define FILTER_PRECISION		8192

#define SNDMIX_REVERSESTEREO	0x000001
#define SNDMIX_NOISEREDUCTION	0x000002
#define SNDMIX_AGC				0x000004
#define SNDMIX_NORESAMPLING		0x000008
#define SNDMIX_SPLINESRCMODE	0x000010
#define SNDMIX_MEGABASS			0x000020
#define SNDMIX_SURROUND			0x000040
#define SNDMIX_REVERB			0x000080
#define SNDMIX_EQ				0x000100
#define SNDMIX_SOFTPANNING		0x000200
#define SNDMIX_POLYPHASESRCMODE	0x000400
#define SNDMIX_FIRFILTERSRCMODE	0x000800
#define SNDMIX_HQRESAMPLER		(SNDMIX_SPLINESRCMODE|SNDMIX_POLYPHASESRCMODE|SNDMIX_FIRFILTERSRCMODE)
#define SNDMIX_ULTRAHQSRCMODE	(SNDMIX_POLYPHASESRCMODE|SNDMIX_FIRFILTERSRCMODE)
#define SNDMIX_ENABLEMMX		0x020000
#define SNDMIX_NOBACKWARDJUMPS	0x040000
#define SNDMIX_MAXDEFAULTPAN	0x080000
#define SNDMIX_MUTECHNMODE		0x100000
#define SNDMIX_EMULATE_MIX_BUGS 0x200000

#define SOUNDSETUP_ENABLEMMX	0x08
#define SOUNDSETUP_SOFTPANNING	0x10
#define SOUNDSETUP_STREVERSE	0x20
#define SOUNDSETUP_SECONDARY	0x40
#define SOUNDSETUP_RESTARTMASK	SOUNDSETUP_SECONDARY

#define PROCSUPPORT_CPUID		0x01
#define PROCSUPPORT_MMX			0x02
#define PROCSUPPORT_MMXEX		0x04
#define PROCSUPPORT_3DNOW		0x08
#define PROCSUPPORT_SSE			0x10

#define SRCMODE_NEAREST			0
#define SRCMODE_LINEAR			1
#define SRCMODE_SPLINE			2
#define SRCMODE_POLYPHASE		3
#define SRCMODE_FIRFILTER		4
#define SRCMODE_DEFAULT			5
#define NUM_SRC_MODES			6

#define QUALITY_NOISEREDUCTION	0x01
#define QUALITY_MEGABASS		0x02
#define QUALITY_SURROUND		0x08
#define QUALITY_REVERB			0x20
#define QUALITY_AGC				0x40
#define QUALITY_EQ				0x80

#define tempo_mode_classic		0
#define tempo_mode_alternative	1
#define tempo_mode_modern		2

#define MIXBUFFERSIZE		512
#define SCRATCH_BUFFER_SIZE 64
#define MIXING_ATTENUATION	4
#define VOLUMERAMPPRECISION	12	
#define FADESONGDELAY		100
#define EQ_BUFFERSIZE		(MIXBUFFERSIZE)
#define AGC_PRECISION		10
#define AGC_UNITY			(1 << AGC_PRECISION)

#define OFFSDECAYSHIFT		8
#define OFFSDECAYMASK		0xFF

#define MIXNDX_16BIT		0x01
#define MIXNDX_STEREO		0x02
#define MIXNDX_RAMP			0x04
#define MIXNDX_FILTER		0x08

#define MIXNDX_LINEARSRC	0x10
#define MIXNDX_HQSRC		0x20
#define MIXNDX_KAISERSRC	0x30
#define MIXNDX_FIRFILTERSRC	0x40

#define MIXPLUG_INPUTF_MASTEREFFECT	0x01
#define MIXPLUG_INPUTF_BYPASS		0x02
#define MIXPLUG_INPUTF_WETMIX		0x04
#define MIXPLUG_INPUTF_MIXEXPAND	0x08

#define MIDIOUT_START		0
#define MIDIOUT_STOP		1
#define MIDIOUT_TICK		2
#define MIDIOUT_NOTEON		3
#define MIDIOUT_NOTEOFF		4
#define MIDIOUT_VOLUME		5
#define MIDIOUT_PAN			6
#define MIDIOUT_BANKSEL		7
#define MIDIOUT_PROGRAM		8

#define plugmix_mode_original	0
#define plugmix_mode_117RC1		1
#define plugmix_mode_117RC2		2
#define plugmix_mode_Test		3

#define NO_ATTENUATION		1
#define MIXING_CLIPMIN		-0x07FFFFFF
#define MIXING_CLIPMAX		0x07FFFFFF

typedef DWORD (__CDECL__ *ConvertProc)(void *, int *, DWORD);

BYTE PortaVolCmd[16] =
{
	0x00, 0x01, 0x04, 0x08, 0x10, 0x20, 0x40, 0x60,
	0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

DWORD FineLinearSlideUpTable[16] =
{
	65536, 65595, 65654, 65714,	65773, 65832, 65892, 65951,
	66011, 66071, 66130, 66190, 66250, 66309, 66369, 66429
};

DWORD FineLinearSlideDownTable[16] =
{
	65535, 65477, 65418, 65359, 65300, 65241, 65182, 65123,
	65065, 65006, 64947, 64888, 64830, 64772, 64713, 64645
};

DWORD LinearSlideUpTable[256] = 
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
	160439, 161019, 161601, 162186, 162772, 163361, 163952, 164545, 
};

DWORD LinearSlideDownTable[256] = 
{
	65536, 65299, 65064, 64830, 64596, 64363, 64131, 63900, 
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
	26770, 26673, 26577, 26481, 26386, 26291, 26196, 26102, 
};

signed char retrigTable1[16] =
{
	0, 0, 0, 0, 0, 0, 10, 8, 0, 0, 0, 0, 0, 0, 24, 32
};

signed char retrigTable2[16] =
{
	0, -1, -2, -4, -8, -16, 0, 0, 0, 1, 2, 4, 8, 16, 0, 0
};

WORD FineTuneTable[16] = 
{
	7895, 7941, 7985, 8046, 8107, 8169, 8232, 8280,
	8363, 8413, 8463, 8529, 8581, 8651, 8723, 8757
};

const UINT PreAmpTable[16] =
{
	0x60, 0x60, 0x60, 0x70,
	0x80, 0x88, 0x90, 0x98,
	0xA0, 0xA4, 0xA8, 0xAC,
	0xB0, 0xB4, 0xB8, 0xBC
};

const UINT PreAmpAGCTable[16] =
{
	0x60, 0x60, 0x60, 0x64,
	0x68, 0x70, 0x78, 0x80,
	0x84, 0x88, 0x8C, 0x90,
	0x92, 0x94, 0x96, 0x98
};

short SinusTable[64] =
{
	0,12,25,37,49,60,71,81,90,98,106,112,117,122,125,126,
	127,126,125,122,117,112,106,98,90,81,71,60,49,37,25,12,
	0,-12,-25,-37,-49,-60,-71,-81,-90,-98,-106,-112,-117,-122,-125,-126,
	-127,-126,-125,-122,-117,-112,-106,-98,-90,-81,-71,-60,-49,-37,-25,-12
};

short RampDownTable[64] =
{
	0,-4,-8,-12,-16,-20,-24,-28,-32,-36,-40,-44,-48,-52,-56,-60,
	-64,-68,-72,-76,-80,-84,-88,-92,-96,-100,-104,-108,-112,-116,-120,-124,
	127,123,119,115,111,107,103,99,95,91,87,83,79,75,71,67,
	63,59,55,51,47,43,39,35,31,27,23,19,15,11,7,3
};

short SquareTable[64] =
{
	127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
	127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
	-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,
	-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127
};

short RandomTable[64] =
{
	98,-127,-43,88,102,41,-65,-94,125,20,-71,-86,-70,-32,-16,-96,
	17,72,107,-5,116,-69,-62,-40,10,-61,65,109,-18,-38,-13,-76,
	-23,88,21,-94,8,106,21,-112,6,109,20,-88,-30,9,-127,118,
	42,-34,89,-4,-51,-72,21,-29,112,123,84,-101,-92,98,-54,-95
};

WORD FreqTable[16] = 
{
	1712,1616,1524,1440,1356,1280,
	1208,1140,1076,1016,960,907,
	0,0,0,0
};

signed char VibratoTable[256] =
{
	0,-2,-3,-5,-6,-8,-9,-11,-12,-14,-16,-17,-19,-20,-22,-23,
	-24,-26,-27,-29,-30,-32,-33,-34,-36,-37,-38,-39,-41,-42,
	-43,-44,-45,-46,-47,-48,-49,-50,-51,-52,-53,-54,-55,-56,
	-56,-57,-58,-59,-59,-60,-60,-61,-61,-62,-62,-62,-63,-63,
	-63,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-63,-63,
	-63,-62,-62,-62,-61,-61,-60,-60,-59,-59,-58,-57,-56,-56,
	-55,-54,-53,-52,-51,-50,-49,-48,-47,-46,-45,-44,-43,-42,
	-41,-39,-38,-37,-36,-34,-33,-32,-30,-29,-27,-26,-24,-23,
	-22,-20,-19,-17,-16,-14,-12,-11,-9,-8,-6,-5,-3,-2,0,
	2,3,5,6,8,9,11,12,14,16,17,19,20,22,23,24,26,27,29,30,
	32,33,34,36,37,38,39,41,42,43,44,45,46,47,48,49,50,51,
	52,53,54,55,56,56,57,58,59,59,60,60,61,61,62,62,62,63,
	63,63,64,64,64,64,64,64,64,64,64,64,64,63,63,63,62,62,
	62,61,61,60,60,59,59,58,57,56,56,55,54,53,52,51,50,49,
	48,47,46,45,44,43,42,41,39,38,37,36,34,33,32,30,29,27,
	26,24,23,22,20,19,17,16,14,12,11,9,8,6,5,3,2
};

short FastSinc[1024] =
{
    0, 16384,     0,     0,   -31, 16383,    32,     0,   -63, 16381,    65,     0,   -93, 16378,   100,    -1,
 -124, 16374,   135,    -1,  -153, 16368,   172,    -3,  -183, 16361,   209,    -4,  -211, 16353,   247,    -5,
 -240, 16344,   287,    -7,  -268, 16334,   327,    -9,  -295, 16322,   368,   -12,  -322, 16310,   410,   -14,
 -348, 16296,   453,   -17,  -374, 16281,   497,   -20,  -400, 16265,   541,   -23,  -425, 16248,   587,   -26,
 -450, 16230,   634,   -30,  -474, 16210,   681,   -33,  -497, 16190,   729,   -37,  -521, 16168,   778,   -41,
 -543, 16145,   828,   -46,  -566, 16121,   878,   -50,  -588, 16097,   930,   -55,  -609, 16071,   982,   -60,
 -630, 16044,  1035,   -65,  -651, 16016,  1089,   -70,  -671, 15987,  1144,   -75,  -691, 15957,  1199,   -81,
 -710, 15926,  1255,   -87,  -729, 15894,  1312,   -93,  -748, 15861,  1370,   -99,  -766, 15827,  1428,  -105,
 -784, 15792,  1488,  -112,  -801, 15756,  1547,  -118,  -818, 15719,  1608,  -125,  -834, 15681,  1669,  -132,
 -850, 15642,  1731,  -139,  -866, 15602,  1794,  -146,  -881, 15561,  1857,  -153,  -896, 15520,  1921,  -161,
 -911, 15477,  1986,  -168,  -925, 15434,  2051,  -176,  -939, 15390,  2117,  -184,  -952, 15344,  2184,  -192,
 -965, 15298,  2251,  -200,  -978, 15251,  2319,  -208,  -990, 15204,  2387,  -216, -1002, 15155,  2456,  -225,
-1014, 15106,  2526,  -234, -1025, 15055,  2596,  -242, -1036, 15004,  2666,  -251, -1046, 14952,  2738,  -260,
-1056, 14899,  2810,  -269, -1066, 14846,  2882,  -278, -1075, 14792,  2955,  -287, -1084, 14737,  3028,  -296,
-1093, 14681,  3102,  -306, -1102, 14624,  3177,  -315, -1110, 14567,  3252,  -325, -1118, 14509,  3327,  -334,
-1125, 14450,  3403,  -344, -1132, 14390,  3480,  -354, -1139, 14330,  3556,  -364, -1145, 14269,  3634,  -374,
-1152, 14208,  3712,  -384, -1157, 14145,  3790,  -394, -1163, 14082,  3868,  -404, -1168, 14018,  3947,  -414,
-1173, 13954,  4027,  -424, -1178, 13889,  4107,  -434, -1182, 13823,  4187,  -445, -1186, 13757,  4268,  -455,
-1190, 13690,  4349,  -465, -1193, 13623,  4430,  -476, -1196, 13555,  4512,  -486, -1199, 13486,  4594,  -497,
-1202, 13417,  4676,  -507, -1204, 13347,  4759,  -518, -1206, 13276,  4842,  -528, -1208, 13205,  4926,  -539,
-1210, 13134,  5010,  -550, -1211, 13061,  5094,  -560, -1212, 12989,  5178,  -571, -1212, 12915,  5262,  -581,
-1213, 12842,  5347,  -592, -1213, 12767,  5432,  -603, -1213, 12693,  5518,  -613, -1213, 12617,  5603,  -624,
-1212, 12542,  5689,  -635, -1211, 12466,  5775,  -645, -1210, 12389,  5862,  -656, -1209, 12312,  5948,  -667,
-1208, 12234,  6035,  -677, -1206, 12156,  6122,  -688, -1204, 12078,  6209,  -698, -1202, 11999,  6296,  -709,
-1200, 11920,  6384,  -720, -1197, 11840,  6471,  -730, -1194, 11760,  6559,  -740, -1191, 11679,  6647,  -751,
-1188, 11598,  6735,  -761, -1184, 11517,  6823,  -772, -1181, 11436,  6911,  -782, -1177, 11354,  6999,  -792,
-1173, 11271,  7088,  -802, -1168, 11189,  7176,  -812, -1164, 11106,  7265,  -822, -1159, 11022,  7354,  -832,
-1155, 10939,  7442,  -842, -1150, 10855,  7531,  -852, -1144, 10771,  7620,  -862, -1139, 10686,  7709,  -872,
-1134, 10602,  7798,  -882, -1128, 10516,  7886,  -891, -1122, 10431,  7975,  -901, -1116, 10346,  8064,  -910,
-1110, 10260,  8153,  -919, -1103, 10174,  8242,  -929, -1097, 10088,  8331,  -938, -1090, 10001,  8420,  -947,
-1083,  9915,  8508,  -956, -1076,  9828,  8597,  -965, -1069,  9741,  8686,  -973, -1062,  9654,  8774,  -982,
-1054,  9566,  8863,  -991, -1047,  9479,  8951,  -999, -1039,  9391,  9039, -1007, -1031,  9303,  9127, -1015,
-1024,  9216,  9216, -1024, -1015,  9127,  9303, -1031, -1007,  9039,  9391, -1039,  -999,  8951,  9479, -1047,
 -991,  8863,  9566, -1054,  -982,  8774,  9654, -1062,  -973,  8686,  9741, -1069,  -965,  8597,  9828, -1076,
 -956,  8508,  9915, -1083,  -947,  8420, 10001, -1090,  -938,  8331, 10088, -1097,  -929,  8242, 10174, -1103,
 -919,  8153, 10260, -1110,  -910,  8064, 10346, -1116,  -901,  7975, 10431, -1122,  -891,  7886, 10516, -1128,
 -882,  7798, 10602, -1134,  -872,  7709, 10686, -1139,  -862,  7620, 10771, -1144,  -852,  7531, 10855, -1150,
 -842,  7442, 10939, -1155,  -832,  7354, 11022, -1159,  -822,  7265, 11106, -1164,  -812,  7176, 11189, -1168,
 -802,  7088, 11271, -1173,  -792,  6999, 11354, -1177,  -782,  6911, 11436, -1181,  -772,  6823, 11517, -1184,
 -761,  6735, 11598, -1188,  -751,  6647, 11679, -1191,  -740,  6559, 11760, -1194,  -730,  6471, 11840, -1197,
 -720,  6384, 11920, -1200,  -709,  6296, 11999, -1202,  -698,  6209, 12078, -1204,  -688,  6122, 12156, -1206,
 -677,  6035, 12234, -1208,  -667,  5948, 12312, -1209,  -656,  5862, 12389, -1210,  -645,  5775, 12466, -1211,
 -635,  5689, 12542, -1212,  -624,  5603, 12617, -1213,  -613,  5518, 12693, -1213,  -603,  5432, 12767, -1213,
 -592,  5347, 12842, -1213,  -581,  5262, 12915, -1212,  -571,  5178, 12989, -1212,  -560,  5094, 13061, -1211,
 -550,  5010, 13134, -1210,  -539,  4926, 13205, -1208,  -528,  4842, 13276, -1206,  -518,  4759, 13347, -1204,
 -507,  4676, 13417, -1202,  -497,  4594, 13486, -1199,  -486,  4512, 13555, -1196,  -476,  4430, 13623, -1193,
 -465,  4349, 13690, -1190,  -455,  4268, 13757, -1186,  -445,  4187, 13823, -1182,  -434,  4107, 13889, -1178,
 -424,  4027, 13954, -1173,  -414,  3947, 14018, -1168,  -404,  3868, 14082, -1163,  -394,  3790, 14145, -1157,
 -384,  3712, 14208, -1152,  -374,  3634, 14269, -1145,  -364,  3556, 14330, -1139,  -354,  3480, 14390, -1132,
 -344,  3403, 14450, -1125,  -334,  3327, 14509, -1118,  -325,  3252, 14567, -1110,  -315,  3177, 14624, -1102,
 -306,  3102, 14681, -1093,  -296,  3028, 14737, -1084,  -287,  2955, 14792, -1075,  -278,  2882, 14846, -1066,
 -269,  2810, 14899, -1056,  -260,  2738, 14952, -1046,  -251,  2666, 15004, -1036,  -242,  2596, 15055, -1025,
 -234,  2526, 15106, -1014,  -225,  2456, 15155, -1002,  -216,  2387, 15204,  -990,  -208,  2319, 15251,  -978,
 -200,  2251, 15298,  -965,  -192,  2184, 15344,  -952,  -184,  2117, 15390,  -939,  -176,  2051, 15434,  -925,
 -168,  1986, 15477,  -911,  -161,  1921, 15520,  -896,  -153,  1857, 15561,  -881,  -146,  1794, 15602,  -866,
 -139,  1731, 15642,  -850,  -132,  1669, 15681,  -834,  -125,  1608, 15719,  -818,  -118,  1547, 15756,  -801,
 -112,  1488, 15792,  -784,  -105,  1428, 15827,  -766,   -99,  1370, 15861,  -748,   -93,  1312, 15894,  -729,
  -87,  1255, 15926,  -710,   -81,  1199, 15957,  -691,   -75,  1144, 15987,  -671,   -70,  1089, 16016,  -651,
  -65,  1035, 16044,  -630,   -60,   982, 16071,  -609,   -55,   930, 16097,  -588,   -50,   878, 16121,  -566,
  -46,   828, 16145,  -543,   -41,   778, 16168,  -521,   -37,   729, 16190,  -497,   -33,   681, 16210,  -474,
  -30,   634, 16230,  -450,   -26,   587, 16248,  -425,   -23,   541, 16265,  -400,   -20,   497, 16281,  -374,
  -17,   453, 16296,  -348,   -14,   410, 16310,  -322,   -12,   368, 16322,  -295,    -9,   327, 16334,  -268,
   -7,   287, 16344,  -240,    -5,   247, 16353,  -211,    -4,   209, 16361,  -183,    -3,   172, 16368,  -153,
   -1,   135, 16374,  -124,    -1,   100, 16378,   -93,     0,    65, 16381,   -63,     0,    32, 16383,   -31,
};

#define SNDMIX_REFLECTIONS_DELAY_MASK	0x1FFF
#define SNDMIX_PREDIFFUSION_DELAY_MASK	0x7F
#define SNDMIX_REVERB_DELAY_MASK		0xFFFF

#define RVBDLY_MASK						2047

#define DCR_AMOUNT						9

typedef struct _SWRVBREFLECTION
{
	ULONG Delay, DelayDest;
	short Gains[4];
} SWRVBREFLECTION;

typedef struct _SWRVBREFDELAY
{
	ULONG DelayPos, PreDifPos, RefOutPos;
	long MasterGain;
	short Coeffs[2];
	short History[2];
	short PreDifCoeffs[2];
	short ReflectionsGain[2];
	SWRVBREFLECTION Reflections[8];
	short RefDelayBuffer[(SNDMIX_REFLECTIONS_DELAY_MASK + 1) * 2];
	short PreDifBuffer[(SNDMIX_PREDIFFUSION_DELAY_MASK + 1) * 2];
	short RefOut[(SNDMIX_REVERB_DELAY_MASK + 1) * 2];
} SWRVBREFDELAY;

typedef struct _SWLATEREVERB
{
	ULONG ReverbDelay;
	ULONG DelayPos;
	short DifCoeffs[4];
	short DecayDC[4];
	short DecayLP[4];
	short LPHistory[4];
	short Dif2InGains[4];
	short RvbOutGains[4];
	long MasterGain;
	long DummyAlign;
	short Diffusion1[(RVBDLY_MASK + 1) * 2];
	short Diffusion2[(RVBDLY_MASK + 1) * 2];
	short Delay1[(RVBDLY_MASK + 1) * 2];
	short Delay2[(RVBDLY_MASK + 1) * 2];
} SWLATEREVERB;

#define SINC_PHASES	4096
#define NUM_REVERBTYPES 29
#define DEFAULT_XBASS_RANGE 14
#define DEFAULT_XBASS_DEPTH 6
#define RVBMINREFDELAY 96
#define RVBMAXREFDELAY 7500
#define RVBMINRVBDELAY 128
#define RVBMAXRVBDELAY 3800

typedef struct _REFLECTIONPRESET
{
	long DelayFactor;
	short GainLL, GainRR, GainLR, GainRL;
} REFLECTIONPRESET;

const REFLECTIONPRESET ReflectionsPreset[ENVIRONMENT_NUMREFLECTIONS] =
{
	{0, 9830, 6554, 0, 0},
	{10, 6554, 13107, 0, 0},
	{24, -9830, 13107, 0, 0},
	{36, 13107, -6554, 0, 0},
	{54, 16384, 16384, -1638, -1638},
	{61, -13107, 8192, -328, -328},
	{73, -11468, -11468, -3277, 3277},
	{87, 13107, -9830, 4916, -4916}
};

#define SNDMIX_REVERB_PRESET_DEFAULT \
	{-10000, 0, 1.00F, 0.50F, -10000, 0.020F, -10000, 0.040F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_GENERIC \
	{-1000, -100, 1.49F, 0.83F, -2602, 0.007F, 200, 0.011F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_PADDEDCELL \
	{-1000, -6000, 0.17F, 0.10F, -1204, 0.001F, 207, 0.002F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_ROOM \
	{-1000, -454, 0.40F, 0.83F, -1646, 0.002F, 53, 0.003F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_BATHROOM \
	{-1000, -1200, 1.49F, 0.54F, -370, 0.007F, 1030, 0.011F, 100.0F, 60.0F}
#define SNDMIX_REVERB_PRESET_LIVINGROOM \
	{-1000, -6000, 0.50F, 0.10F, -1376, 0.003F, -1104, 0.004F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_STONEROOM \
	{-1000, -300, 2.31F, 0.64F, -711, 0.012F, 83, 0.017F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_AUDITORIUM \
	{-1000, -476, 4.32F, 0.59F, -789, 0.020F, -289, 0.030F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_CONCERTHALL \
	{-1000, -500, 3.92F, 0.70F, -1230, 0.020F, -2, 0.029F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_CAVE \
	{-1000, 0, 2.91F, 1.30F, -602, 0.015F, -302, 0.022F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_ARENA \
	{-1000, -698, 7.24F, 0.33F, -1166, 0.020F, 16, 0.030F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_HANGAR \
	{-1000, -1000, 10.05F, 0.23F, -602, 0.020F, 198, 0.030F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_CARPETEDHALLWAY \
	{-1000, -4000, 0.30F, 0.10F, -1831, 0.002F, -1630, 0.030F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_HALLWAY \
	{-1000, -300, 1.49F, 0.59F, -1219, 0.007F, 441, 0.011F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_STONECORRIDOR \
	{-1000, -237, 2.70F, 0.79F, -1214, 0.013F, 395, 0.020F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_ALLEY \
	{-1000, -270, 1.49F, 0.86F, -1204, 0.007F, -4, 0.011F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_FOREST \
	{-1000, -3300, 1.49F, 0.54F, -2560, 0.162F, -613, 0.088F, 79.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_CITY \
	{-1000, -800, 1.49F, 0.67F, -2273, 0.007F, -2217, 0.011F, 50.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_MOUNTAINS \
	{-1000, -2500, 1.49F, 0.21F, -2780, 0.300F, -2014, 0.100F, 27.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_QUARRY \
	{-1000, -1000, 1.49F, 0.83F, -10000, 0.061F, 500, 0.025F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_PLAIN \
	{-1000, -2000, 1.49F, 0.50F, -2466, 0.179F, -2514, 0.100F, 21.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_PARKINGLOT \
	{-1000, 0, 1.65F, 1.50F, -1363, 0.008F, -1153, 0.012F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_SEWERPIPE \
	{-1000, -1000, 2.81F, 0.14F, 429, 0.014F, 648, 0.021F, 80.0F, 60.0F}
#define SNDMIX_REVERB_PRESET_UNDERWATER \
	{-1000, -4000, 1.49F, 0.10F, -449, 0.007F, 1700, 0.011F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_SMALLROOM \
	{-1000, -600, 1.10F, 0.83F, -400, 0.005F, 500, 0.010F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_MEDIUMROOM \
	{-1000, -600, 1.30F, 0.83F, -1000, 0.010F, -200, 0.020F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_LARGEROOM \
	{-1000, -600, 1.50F, 0.83F, -1600, 0.020F, -1000, 0.040F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_MEDIUMHALL \
	{-1000, -600, 1.80F, 0.70F, -1300, 0.015F, -800, 0.030F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_LARGEHALL \
	{-1000, -600, 1.80F, 0.70F, -2000, 0.030F, -1400, 0.060F, 100.0F, 100.0F}
#define SNDMIX_REVERB_PRESET_PLATE \
	{-1000, -200, 1.30F, 0.90F, 0, 0.002F, 0, 0.010F, 100.0F, 75.0F}

static SNDMIX_RVBPRESET RvbPresets[NUM_REVERBTYPES] =
{
	{SNDMIX_REVERB_PRESET_PLATE, "GM Plate"},
	{SNDMIX_REVERB_PRESET_SMALLROOM, "GM Small Room"},
	{SNDMIX_REVERB_PRESET_MEDIUMROOM, "GM Medium Room"},
	{SNDMIX_REVERB_PRESET_LARGEROOM, "GM Large Room"},
	{SNDMIX_REVERB_PRESET_MEDIUMHALL, "GM Medium Hall"},
	{SNDMIX_REVERB_PRESET_LARGEHALL, "GM Large Hall"},
	{SNDMIX_REVERB_PRESET_GENERIC, "Generic"},
	{SNDMIX_REVERB_PRESET_PADDEDCELL, "Padded Cell"},
	{SNDMIX_REVERB_PRESET_ROOM, "Room"},
	{SNDMIX_REVERB_PRESET_BATHROOM, "Bathroom"},
	{SNDMIX_REVERB_PRESET_LIVINGROOM, "Living Room"},
	{SNDMIX_REVERB_PRESET_STONEROOM, "Stone Room"},
	{SNDMIX_REVERB_PRESET_AUDITORIUM, "Auditorium"},
	{SNDMIX_REVERB_PRESET_CONCERTHALL, "Concert Hall"},
	{SNDMIX_REVERB_PRESET_CAVE, "Cave"},
	{SNDMIX_REVERB_PRESET_ARENA, "Arena"},
	{SNDMIX_REVERB_PRESET_HANGAR, "Hangar"},
	{SNDMIX_REVERB_PRESET_CARPETEDHALLWAY, "Carpeted Hallway"},
	{SNDMIX_REVERB_PRESET_HALLWAY, "Hallway"},
	{SNDMIX_REVERB_PRESET_STONECORRIDOR, "Stone Corridor"},
	{SNDMIX_REVERB_PRESET_ALLEY, "Alley"},
	{SNDMIX_REVERB_PRESET_FOREST, "Forest"},
	{SNDMIX_REVERB_PRESET_CITY, "City"},
	{SNDMIX_REVERB_PRESET_MOUNTAINS, "Mountains"},
	{SNDMIX_REVERB_PRESET_QUARRY, "Quarry"},
	{SNDMIX_REVERB_PRESET_PLAIN, "Plain"},
	{SNDMIX_REVERB_PRESET_PARKINGLOT, "Parking Lot"},
	{SNDMIX_REVERB_PRESET_SEWERPIPE, "Sewer Pipe"},
	{SNDMIX_REVERB_PRESET_UNDERWATER, "Underwater"},
};

short KaiserSinc[SINC_PHASES * 8];
short DownSample13x[SINC_PHASES * 8];
short DownSample2x[SINC_PHASES * 8];

static SWRVBREFDELAY RefDelay;
static SWLATEREVERB LateReverb;

static bool RvbDownSample2x = false;
static bool LastInPresent = false;
static bool LastOutPresent = false;
static int LastRvbIn_xl = 0;
static int LastRvbIn_xr = 0;
static int LastRvbIn_yl = 0;
static int LastRvbIn_yr = 0;
static int LastRvbOut_xl = 0;
static int LastRvbOut_xr = 0;
static __int64 DCRRvb_Y1 = 0;
static __int64 DCRRvb_X1 = 0;
long DolbyDepth = 0;

UINT ReverbSamples = 0;
UINT ReverbDecaySamples = 0;
UINT ReverbSend = 0;
long RvbROffsVol = 0;
long RvbLOffsVol = 0;
long DryROffsVol = 0;
long DryLOffsVol = 0;
bool InitPlugins = false;
bool InitTables = false;

int MixSoundBuffer[MIXBUFFERSIZE * 4];
int MixReverbBuffer[MIXBUFFERSIZE * 2];
int MixRearBuffer[MIXBUFFERSIZE * 2];
float MixFloatBuffer[MIXBUFFERSIZE * 2];

#pragma warning(disable : 4799 4731)
#include "fir_mix.h"
#include "std_mix.h"
#include "mmx_mix.h"
#include "rvb_mix.h"
#pragma warning(default : 4799 4731)

typedef void (__CDECL__ *MixInterface)(Channel *, int *, int *);

MixInterface MixFunctionTable[80] = 
{
	//No SRC
	Mono8BitMix, Mono16BitMix, Stereo8BitMix, Stereo16BitMix,
	Mono8BitRampMix, Mono16BitRampMix, Stereo8BitRampMix, Stereo16BitRampMix,
	// Filter No SRC
	FilterMono8BitMix, FilterMono16BitMix, FilterStereo8BitMix, FilterStereo16BitMix,
	FilterMono8BitRampMix, FilterMono16BitRampMix, FilterStereo8BitRampMix, FilterStereo16BitRampMix,
	// Linear SRC
	Mono8BitLinearMix, Mono16BitLinearMix, Stereo8BitLinearMix, Stereo16BitLinearMix,
	Mono8BitLinearRampMix, Mono16BitLinearRampMix, Stereo8BitLinearRampMix, Stereo16BitLinearRampMix,
	// Filter Linear SRC
	FilterMono8BitLinearMix, FilterMono16BitLinearMix, FilterStereo8BitLinearMix, FilterStereo16BitLinearMix,
	FilterMono8BitLinearRampMix, FilterMono16BitLinearRampMix, FilterStereo8BitLinearMix, FilterStereo16BitLinearMix,
	// HQ SRC
	Mono8BitHQMix, Mono16BitHQMix, Stereo8BitHQMix, Stereo16BitHQMix,
	Mono8BitHQRampMix, Mono16BitHQRampMix, Stereo8BitHQRampMix, Stereo16BitHQRampMix,
	// Filter HQ SRC
	FilterMono8BitLinearMix, FilterMono16BitLinearMix, FilterStereo8BitLinearMix, FilterStereo16BitLinearMix,
	FilterMono8BitLinearRampMix, FilterMono16BitLinearRampMix, FilterStereo8BitLinearMix, FilterStereo16BitLinearMix,
	// Kaiser SRC
	Mono8BitKaiserMix, Mono16BitKaiserMix, Stereo8BitKaiserMix, Stereo16BitKaiserMix,
	Mono8BitKaiserRampMix, Mono16BitKaiserRampMix, Stereo8BitKaiserRampMix, Stereo16BitKaiserRampMix,
	// Filter Kaiser SRC
	FilterMono8BitKaiserMix, FilterMono16BitKaiserMix, FilterStereo8BitKaiserMix, FilterStereo16BitKaiserMix,
	FilterMono8BitKaiserRampMix, FilterMono16BitKaiserRampMix, FilterStereo8BitKaiserRampMix, FilterStereo16BitKaiserRampMix,
	// FIRFilter SRC
	Mono8BitFIRFilterMix, Mono16BitFIRFilterMix, Stereo8BitFIRFilterMix, Stereo16BitFIRFilterMix,
	Mono8BitFIRFilterRampMix, Mono16BitFIRFilterRampMix, Stereo8BitFIRFilterRampMix, Stereo16BitFIRFilterRampMix,
	// Filter FIRFilter SRC
	FilterMono8BitFIRFilterMix, FilterMono16BitFIRFilterMix, FilterStereo8BitFIRFilterMix, FilterStereo16BitFIRFilterMix,
	FilterMono8BitFIRFilterRampMix, FilterMono16BitFIRFilterRampMix, FilterStereo8BitFIRFilterRampMix, FilterStereo16BitFIRFilterRampMix
};

MixInterface FastMixFunctionTable[32] = 
{
	// No SRC
	FastMono8BitMix, FastMono16BitMix, Stereo8BitMix, Stereo16BitMix,
	FastMono8BitRampMix, FastMono16BitRampMix, Stereo8BitRampMix, Stereo16BitRampMix,
	// Filter No SRC
	FilterMono8BitMix, FilterMono16BitMix, FilterStereo8BitMix, FilterStereo16BitMix,
	FilterMono8BitRampMix, FilterMono16BitRampMix, FilterStereo8BitRampMix, FilterStereo16BitRampMix,
	// Linear SRC
	FastMono8BitLinearMix, FastMono16BitLinearMix, Stereo8BitLinearMix, Stereo16BitLinearMix,
	FastMono8BitLinearRampMix, FastMono16BitLinearRampMix, Stereo8BitLinearRampMix, Stereo16BitLinearRampMix,
	// Filter Linear SRC
	FilterMono8BitLinearMix, FilterMono16BitLinearMix, FilterStereo8BitLinearMix, FilterStereo16BitLinearMix,
	FilterMono8BitLinearRampMix, FilterMono16BitLinearRampMix, FilterStereo8BitLinearMix, FilterStereo16BitLinearMix
};

MixInterface MMXFunctionTable[80] =
{
	// No SRC
	MMX_Mono8BitMix, MMX_Mono16BitMix, Stereo8BitMix, Stereo16BitMix,
	Mono8BitRampMix, Mono16BitRampMix, Stereo8BitRampMix, Stereo16BitRampMix,
	// Filter No SRC
	MMX_FilterMono8BitMix, MMX_FilterMono16BitMix, FilterStereo8BitMix, FilterStereo16BitMix,
	MMX_FilterMono8BitRampMix, MMX_FilterMono16BitRampMix, FilterStereo8BitRampMix, FilterStereo16BitRampMix,
	// Linear SRC
	MMX_Mono8BitLinearMix, MMX_Mono16BitLinearMix, Stereo8BitLinearMix, Stereo16BitLinearMix,
	Mono8BitLinearRampMix, Mono16BitLinearRampMix, Stereo8BitLinearRampMix, Stereo16BitLinearRampMix,
	// Filter Linear SRC
	MMX_FilterMono8BitLinearMix, MMX_FilterMono16BitLinearMix, FilterStereo8BitLinearMix, FilterStereo16BitLinearMix,
	MMX_FilterMono8BitLinearRampMix, MMX_FilterMono16BitLinearRampMix, FilterStereo8BitLinearMix, FilterStereo16BitLinearMix,
	// HQ SRC
	MMX_Mono8BitHQMix, MMX_Mono16BitHQMix, Stereo8BitHQMix, Stereo16BitHQMix,
	Mono8BitHQRampMix, Mono16BitHQRampMix, Stereo8BitHQRampMix, Stereo16BitHQRampMix,
	// Filter HQ SRC
	MMX_FilterMono8BitLinearMix, MMX_FilterMono16BitLinearMix, FilterStereo8BitLinearMix,
	MMX_FilterMono8BitLinearRampMix, MMX_FilterMono16BitLinearRampMix, FilterStereo8BitLinearMix, FilterStereo16BitLinearMix,
	// Kaiser SRC
MMX_Mono8BitKaiserMix, MMX_Mono16BitKaiserMix, Stereo8BitKaiserMix, Stereo16BitKaiserMix,
	MMX_Mono8BitKaiserRampMix, MMX_Mono16BitKaiserRampMix, Stereo8BitKaiserRampMix, Stereo16BitKaiserRampMix,
	// Filter Kaiser SRC
	FilterMono8BitKaiserMix, FilterMono16BitKaiserMix, FilterStereo8BitKaiserMix, FilterStereo16BitKaiserMix,
	FilterMono8BitKaiserRampMix, FilterMono16BitKaiserRampMix, FilterStereo8BitKaiserRampMix, FilterStereo16BitKaiserRampMix,
	// FIRFilter SRC
	Mono8BitFIRFilterMix, Mono16BitFIRFilterMix, Stereo8BitFIRFilterMix, Stereo16BitFIRFilterMix,
	Mono8BitFIRFilterRampMix, Mono16BitFIRFilterRampMix, Stereo8BitFIRFilterRampMix, Stereo16BitFIRFilterRampMix,
	// Filter FIRFilter SRC
	FilterMono8BitFIRFilterMix, FilterMono16BitFIRFilterMix, FilterStereo8BitFIRFilterMix, FilterStereo16BitFIRFilterMix,
	FilterMono8BitFIRFilterRampMix, FilterMono16BitFIRFilterRampMix, FilterStereo8BitFIRFilterRampMix, FilterStereo16BitFIRFilterRampMix
};

BYTE autovibit2xm[8] = {0, 3, 1, 4, 2, 0, 0, 0};
BYTE autovibxm2it[8] = {0, 2, 4, 1, 3, 0, 0, 0};

void __CDECL__ X86_InitMixBuffer(int *Buffer, UINT Samples)
{
	__asm
	{
		mov ecx, Samples
		mov esi, Buffer
		xor eax, eax
		mov edx, ecx
		shr ecx, 2
		and edx, 3
		jz unroll_4x
loop_1x:
		add esi, 4
		dec edx
		mov dword ptr [esi - 4], eax
		jnz loop_1x
unroll_4x:
		or ecx, ecx
		jnz loop_4x
		jmp done
loop_4x:
		add esi, 16
		dec ecx
		mov dword ptr [esi - 16], eax
		mov dword ptr [esi - 16], eax
		mov dword ptr [esi - 8], eax
		mov dword ptr [esi - 4], eax
		jnz loop_4x
done:
	}
}

void __CDECL__ X86_MonoFromStereo(int *MixBuff, UINT Samples)
{
	__asm
	{
		mov ecx, Samples
		mov esi, MixBuff
		mov edi, esi
st_loop:
		mov eax, dword ptr [esi]
		mov edx, dword ptr [esi + 4]
		add edi, 4
		add esi, 8
		add eax, edx
		sar eax, 1
		dec ecx
		mov dword ptr [edi - 4], eax
		jnz st_loop
	}
}

void __CDECL__ X86_StereoFill(int *Buffer, UINT Samples, long *ROffs, long *LOffs)
{
	__asm
	{
		mov edi, Buffer
		mov ecx, Samples
		mov eax, ROffs
		mov edx, LOffs
		mov eax, [eax]
		mov edx, [edx]
		or ecx, ecx
		jz fill_loop
		mov ebx, eax
		or ebx, edx
		jz fill_loop
offs_loop:
		mov ebx, eax
		mov esi, edx
		neg ebx
		neg esi
		sar ebx, 31
		sar esi, 31
		and ebx, OFFSDECAYMASK
		and esi, OFFSDECAYMASK
		add ebx, eax
		add esi, edx
		sar ebx, OFFSDECAYSHIFT
		sar esi, OFFSDECAYSHIFT
		sub eax, ebx
		sub edx, esi
		mov ebx, eax
		or ebx, edx
		jz fill_loop
		add edi, 8
		dec ecx
		mov [edi - 8], eax
		mov [edi - 4], edx
		jnz offs_loop
fill_loop:
		mov ebx, ecx
		and ebx, 3
		jz fill_4x
fill_1x:
		mov [edi], eax
		mov [edi + 4], edx
		add edi, 8
		dec ebx
		jnz fill_1x
fill_4x:
		shr ecx, 2
		or ecx, ecx
		jz done
fill_4x_loop:
		mov [edi], eax
		mov [edi + 4], edx
		mov [edi + 8], eax
		mov [edi + 12], edx
		add edi, 32
		dec ecx
		mov [edi - 16], eax
		mov [edi - 12], edx
		mov [edi - 8], eax
		mov [edi - 4], edx
		jnz fill_4x_loop
done:
		mov esi, ROffs
		mov edi, LOffs
		mov [esi], eax
		mov [edi], edx
	}
}

DWORD __CDECL__ X86_Convert32To8(void *_out, int *_in, DWORD SampleCount)
{
	DWORD result;

	__asm
	{
		mov ebx, _out
		mov edx, _in
		mov edi, SampleCount
cliploop:
		mov eax, dword ptr [edx]
		inc ebx
		add eax, (1 << 19)
		add edx, 4
		cmp eax, (-0x07FFFFFF)
		jl cliplow
		cmp eax, (0x07FFFFFF)
		jg cliphigh
cliprecover:
		sar eax, 20
		xor eax, 0x80
		dec edi
		mov byte ptr [ebx - 1], al
		jnz cliploop
		jmp done
cliplow:
		mov eax, (-0x07FFFFFF)
		jmp cliprecover
cliphigh:
		mov eax, (0x07FFFFFF)
		jmp cliprecover
done:
		mov eax, SampleCount
		mov result, eax
	}
	return result;
}

DWORD __CDECL__ X86_Convert32To16(void *_out, int *_in, DWORD SampleCount)
{
	DWORD result;

	__asm
	{
		mov ebx, _out
		mov edx, _in
		mov edi, SampleCount
cliploop:
		mov eax, dword ptr [edx]
		add ebx, 2
		add eax, (1 << 11)
		add edx, 4
		cmp eax, (-0x07FFFFFF)
		jl cliplow
		cmp eax, (0x07FFFFFF)
		jg cliphigh
cliprecover:
		sar eax, 12
		dec edi
		mov word ptr [ebx - 2], ax
		jnz cliploop
		jmp done
cliplow:
		mov eax, (-0x07FFFFFF)
		jmp cliprecover
cliphigh:
		mov eax, (0x07FFFFFF)
		jmp cliprecover
done:
		mov eax, SampleCount
		add eax, eax
		mov result, eax
	}
	return result;
}

void __CDECL__ X86_EndChannelOffs(Channel *chn, int *Buffer, UINT Samples)
{
	__asm
	{
		mov esi, chn
		mov edi, Buffer
		mov ecx, Samples
		mov eax, dword ptr [esi + Channel.ROfs]
		mov edx, dword ptr [esi + Channel.LOfs]
		or ecx, ecx
		jz brkloop
offsloop:
		mov ebx,eax
		mov esi, edx
		neg ebx
		neg esi
		sar ebx, 31
		sar esi, 31
		and ebx, OFFSDECAYMASK
		and esi, OFFSDECAYMASK
		and ebx, eax
		and esi, edx
		sar ebx, OFFSDECAYSHIFT
		sar esi, OFFSDECAYSHIFT
		sub eax, ebx
		sub edx, esi
		mov ebx, eax
		add dword ptr [edi + 0], eax
		add dword ptr [edi + 4], edx
		or ebx, edx
		jz brkloop
		add edi, 8
		dec ecx
		jnz offsloop
brkloop:
		mov esi, chn
		mov dword ptr [esi + Channel.ROfs], eax
		mov dword ptr [esi + Channel.LOfs], edx
	}
}

int muldiv(long a, long b, long c)
{
	int sign, result;
	__asm
	{
		mov eax, a
		mov ebx, b
		or eax, eax
		mov edx, eax
		jge a_neg
		neg eax
a_neg:
		xor edx, ebx
		or ebx, ebx
		mov ecx, c
		jge b_neg
		neg ebx
b_neg:
		xor edx, ecx
		or ecx, ecx
		mov sign, edx
		jge c_neg
		neg ecx
c_neg:
		mul ebx
		cmp edx, ecx
		jae diverr // unsigned jge
		div ecx
		jmp ok
diverr:
		mov eax, 0x7FFFFFFF
ok:
		mov edx, sign
		or edx, edx
		jge r_neg
		neg eax
r_neg:
		mov result, eax
	}
	return result;
}

inline float Sign(float x)
{
	return (x >= 0 ? 1.0F : -1.0F);
}

void ShelfEQ(long Scale, long *A1, long *B0, long *B1, long c, long s, float GainDC, float GainFT, float GainPI)
{
	float a1, b0, b1, gainFT, gainDC, gainPI, Alpha, Beta0, Beta1, Rho, wT, Quad;
	__asm
	{
		fild c
		fldpi
		fmulp st(1), st(0)
		fild s
		fdivp st(1), st(0)
		fstp wT
		fld GainDC
		fld GainFT
		fld GainPI
		fmul st(0), st(0)
		fstp gainPI
		fmul st(0), st(0)
		fstp gainFT
		fmul st(0), st(0)
		fstp gainDC
	}
	Quad = gainPI * gainDC * (gainFT * 2);
	Alpha = 0;
	if (Quad != 0)
	{
		float Lambda = (gainPI - gainDC) / Quad;
		Alpha = (float)(Lambda - Sign(Lambda) * sqrt(Lambda * Lambda - 1.0F));
	}
	Beta0 = 0.5F * ((GainDC + GainPI) + (GainDC - GainPI) * Alpha);
	Beta1 = 0.5F * ((GainDC - GainPI) + (GainDC + GainPI) * Alpha);
	Rho = (float)(sin((wT * 0.5F) - (M_PI / 4.0F)) / sin((wT * 0.5F) + (M_PI / 4.0F)));
	Quad = 1.0F / (1.0F + Rho * Alpha);
	b0 = (Beta0 + Rho * Beta1) * Quad;
	b1 = (Beta1 + Rho * Beta0) * Quad;
	a1 = -((Rho + Alpha) * Quad);
	__asm
	{
		fild Scale
		fld a1
		mov eax, A1
		fmul st(0), st(1)
		fistp dword ptr [eax]
		fld b0
		mov eax, B0
		fmul st(0), st(1)
		fistp dword ptr [eax]
		fld b1
		mov eax, B1
		fmul st(0), st(1)
		fistp dword ptr [eax]
		fstp Rho
	}
}

class IPlayConfig
{
public:
	IPlayConfig()
	{
		SetPluginMixLevels(plugmix_mode_117RC2);
		Set_VSTiVolume(1.0F);
	}

	bool Get_GlobalVolumeAppliesToMaster()
	{
		return GlobalVolumeAppliesToMaster;
	}

	void Set_GlobalVolumeAppliesToMaster(bool applies)
	{
		GlobalVolumeAppliesToMaster = applies;
	}

	float Get_IntToFloat()
	{
		return IntToFloat;
	}

	float Get_FloatToInt()
	{
		return FloatToInt;
	}

	void Set_IntToFloat(float IntToFloat)
	{
		this->IntToFloat = IntToFloat;
	}

	void Set_FloatToInt(float FloatToInt)
	{
		this->FloatToInt = FloatToInt;
	}

	void Set_VSTiAttenuation(float VSTiAtten)
	{
		VSTiAttenuation = VSTiAtten;
		VSTiGainFactor = VSTiAttenuation * VSTiVolume;
	}

	void Set_VSTiVolume(float VSTiVol)
	{
		VSTiVolume = VSTiVol;
		VSTiGainFactor = VSTiAttenuation * VSTiVolume;
	}

	void SetPluginMixLevels(int mixLevelType)
	{
		switch (mixLevelType)
		{
			case plugmix_mode_original:
			{
				Set_VSTiAttenuation(NO_ATTENUATION);
				Set_IntToFloat(1.0F / (float)(1 << 28));
				Set_FloatToInt((float)(1 << 28));
				Set_GlobalVolumeAppliesToMaster(false);
				break;
			}
			case plugmix_mode_117RC1:
			{
				Set_VSTiAttenuation(32.0F);
				Set_IntToFloat(1.0F / (float)(0x7FFFFFFF));
				Set_FloatToInt((float)(0x7FFFFFFF));
				Set_GlobalVolumeAppliesToMaster(false);
				break;
			}
			default:
			{
				Set_VSTiAttenuation(2.0F);
				Set_IntToFloat(1.0F / (float)MIXING_CLIPMAX);
				Set_FloatToInt((float)MIXING_CLIPMAX);
				Set_GlobalVolumeAppliesToMaster(true);
			}
		}
	}

private:
	float IntToFloat;
	float FloatToInt;
	float VSTiAttenuation;
	float VSTiVolume;
	float VSTiGainFactor;
	bool GlobalVolumeAppliesToMaster;
};

void ISoundFile::ResetMidiCfg()
{
	memset(&MidiCfg, 0x00, sizeof(MidiCfg));
	strcpy(&MidiCfg.MidiGlb[MIDIOUT_START * 32], "FF");
	strcpy(&MidiCfg.MidiGlb[MIDIOUT_STOP * 32], "FC");
	strcpy(&MidiCfg.MidiGlb[MIDIOUT_NOTEON * 32], "9c n v");
	strcpy(&MidiCfg.MidiGlb[MIDIOUT_NOTEOFF * 32], "9c n 0");
	strcpy(&MidiCfg.MidiGlb[MIDIOUT_PROGRAM * 32], "Cc p");
	strcpy(MidiCfg.MidiSFXExt, "F0F000z");
	for (int i = 0; i < 16; i++)
		sprintf(&MidiCfg.MidiZXXExt[i * 32], "F0F001%02X", i * 8);
}

DWORD ISoundFile::OutChannels = 0;
DWORD ISoundFile::SoundSetup = 0;
DWORD ISoundFile::SysInfo = 0;
DWORD ISoundFile::BitsPerSample = 0;
UINT ISoundFile::MaxMixChannels = 32;
UINT ISoundFile::VolumeRampSamples = 42;
UINT ISoundFile::AGC = AGC_UNITY;
UINT ISoundFile::StereoSeparation = 128;
UINT ISoundFile::ReverbDepth = 8;
UINT ISoundFile::XBassDepth = DEFAULT_XBASS_DEPTH;
UINT ISoundFile::XBassRange = DEFAULT_XBASS_RANGE;
UINT ISoundFile::ProLogicDepth = 12;
UINT ISoundFile::ProLogicDelay = 20;
UINT ISoundFile::ReverbType = 0;
DWORD ISoundFile::Quality = 0;//QUALITY_MEGABASS;// | QUALITY_SURROUND;

void ISoundFile::SetCurrentPos(UINT Pos)
{
	UINT i, Pattern, Row;
	for (i = 0; i < MAX_CHANNELS; i++)
	{
		Chns[i].Note = Chns[i].NewNote = Chns[i].NewInst = 0;
		Chns[i].Instrument = NULL;
		Chns[i].Header = NULL;
		Chns[i].PortamentoDest = 0;
		Chns[i].Command = 0;
		Chns[i].PaternLoopCount = 0;
		Chns[i].PaternLoop = 0;
		Chns[i].FadeOutVol = 0;
		Chns[i].Flags |= (CHN_KEYOFF | CHN_NOTEFADE);
		Chns[i].TremorCount = 0;
	}
	if (Pos == 0)
	{
		for (i = 0; i < MAX_CHANNELS; i++)
		{
			Chns[i].Period = 0;
			Chns[i].Pos = Chns[i].Length = 0;
			Chns[i].LoopStart = Chns[i].LoopEnd = 0;
			Chns[i].ROfs = Chns[i].LOfs = 0;
			Chns[i].Sample = NULL;
			Chns[i].Instrument = NULL;
			Chns[i].Header = NULL;
			Chns[i].CutOff = 0x7F;
			Chns[i].Resonance = 0;
			Chns[i].FilterMode = 0;
			Chns[i].LeftVol = Chns[i].RightVol = 0;
			Chns[i].NewLeftVol = Chns[i].NewRightVol = 0;
			Chns[i].LeftRamp = Chns[i].RightRamp = 0;
			Chns[i].Volume = 256;
			if (i < MAX_BASECHANNELS)
			{
				Chns[i].Flags = ChnSettings[i].Flags;
				Chns[i].Pan = ChnSettings[i].Pan;
				Chns[i].GlobalVol = ChnSettings[i].Volume;
			}
			else
			{
				Chns[i].Flags = 0;
				Chns[i].Pan = 128;
				Chns[i].GlobalVol = 64;
			}
		}
		GlobalVolume = p_IF->p_Head->globalvol;
		MusicSpeed = p_IF->p_Head->speed;
		MusicTempo = p_IF->p_Head->tempo;
	}
	SongFlags &= ~(SONG_PATTERNLOOP | SONG_CPUVERYHIGH | SONG_FADINGSONG | SONG_ENDREACHED | SONG_GLOBALFADE);
	for (Pattern = 0; Pattern < p_IF->p_Head->ordnum; Pattern++)
	{
		UINT ord = Order[Pattern];
		if (ord == 0xFE)
			continue;
		if (ord == 0xFF)
			break;
		if (ord < p_IF->p_Head->patnum)
		{
			if (Pos < (UINT)Patterns[ord].nRows)
				break;
			Pos -= Patterns[ord].nRows;
		}
	}
	if (Pattern >= p_IF->p_Head->ordnum || Order[Pattern] >= p_IF->p_Head->patnum ||
		Pos >= Patterns[Order[Pattern]].nRows)
	{
		Pos = 0;
		Pattern = 0;
	}
	Row = Pos;
	if (Row != 0 && Order[Pattern] < p_IF->p_Head->patnum)
	{
		Command *cmd = Patterns[Order[Pattern]].p_Commands;
		if (cmd != NULL && Row < Patterns[Order[Pattern]].nRows)
		{
			bool OK = false;
			while (OK == false && Row > 0)
			{
				UINT n = Row * Channels;
				for (UINT k = 0; k < Channels; k++, n++)
				{
					if (cmd[n].note != 0)
					{
						OK = true;
						break;
					}
				}
				if (OK == false)
					Row--;
			}
		}
	}
	NextPattern = Pattern;
	NextRow = Row;
	TickCount = MusicSpeed;
	BufferCount = 0;
	PatternDelay = 0;
	FrameDelay = 0;
}

ISoundFile::ISoundFile(IT_Intern *p_ITFile)
{
	InitSysInfo();
	if ((SysInfo & SYSMIX_ENABLEMMX) != 0)
		SoundSetup |= SOUNDSETUP_ENABLEMMX;
	UpdateAudioParameters(true);
	p_IF = p_ITFile;
	BufferCount = 0;
	MusicSpeed = p_IF->p_Head->speed;
	if (MusicSpeed == 0)
		MusicSpeed = 6;
	MusicTempo = p_IF->p_Head->tempo;
	if (MusicTempo < 32)
		MusicTempo = 125;
	Channels = p_IF->nChannels;
	Instruments = p_IF->p_Head->smpnum;
	OutChannels = p_IF->p_FI->Channels;
	BitsPerSample = p_IF->p_FI->BitsPerSample;
	SongFlags = 0;
	if (p_IF->p_Head->flags & 0x08) SongFlags |= SONG_LINEARSLIDES;
	if (p_IF->p_Head->flags & 0x10) SongFlags |= SONG_ITOLDEFFECTS;
	if (p_IF->p_Head->flags & 0x20) SongFlags |= SONG_ITCOMPATMODE;
	if (p_IF->p_Head->flags & 0x80) SongFlags |= SONG_EMBEDMIDICFG;
	if (p_IF->p_Head->flags & 0x1000) SongFlags |= SONG_EXFILTERRANGE;
	MixChannels = 0;
	FreqFactor = 128;
	MasterVolume = 128;
	MinPeriod = 8;
	MaxPeriod = 0xF000;
	RepeatCount = 0;
	SeqOverride = 0;
	PatternTransitionOccurred = false;
	RowsPerBeat = 4;
	TempoMode = tempo_mode_classic;
	memset(Chns, 0x00, sizeof(Chns));
	memset(ChnMix, 0x00, sizeof(ChnMix));
	Ins = p_IF->p_Samp;
	memset(ChnSettings, 0x00, sizeof(ChnSettings));
	memset(Headers, 0x00, sizeof(Headers));
	Order = p_IF->p_PaternOrder;
	Patterns = p_IF->p_Paterns;
	for (int i = 0; i < p_IF->p_Head->insnum; i++)
		Headers[i] = &p_IF->p_Ins[i];
	PlugMixMode = plugmix_mode_117RC2;
	Config = new IPlayConfig();
	GlobalVolume = 128;
	OldGlbVolSlide = 0;
	PatternDelay = 0;
	FrameDelay = 0;
	NextRow = Row = 0;
	TotalSampleCount = TotalCount = 0;
	Pattern = CurrentPattern = NextPattern = 0;
	RestartPos = 0;
	SongPreAmp = 100;
	MaxOrderPosition = 0;
	GlobalFadeSamples = GlobalFadeMaxSamples = 0;
	MixStat = 0;
	ResetMidiCfg();
	for (UINT i = 0; i < Channels; i++)
	{
		ChnSettings[i].Pan = ((p_IF->p_Head->chnpan[i] & 0x7F) <= 64 ? (p_IF->p_Head->chnpan[i] & 0x7F) << 2 : 128);
		ChnSettings[i].Volume = p_IF->p_Head->chnvol[i];
		if (ChnSettings[i].Volume > 64)
			ChnSettings[i].Volume = 64;
		ChnSettings[i].Flags = 0;
		ChnSettings[i].Flags |= ((p_IF->p_Head->chnpan[i] & 0x80) != 0 ? CHN_MUTE : 0);
		ChnSettings[i].Flags |= ((p_IF->p_Head->chnpan[i] & 0x7F) == 100 ? CHN_SURROUND : 0);
		//ChnSettings[i].Name[0] = 0;
		Chns[i].Pan = ChnSettings[i].Pan;
		Chns[i].GlobalVol = ChnSettings[i].Volume;
		Chns[i].Flags = ChnSettings[i].Flags;
		Chns[i].Volume = 256;
		Chns[i].CutOff = 0x7F;
	}
	for (UINT i = Channels; i < MAX_BASECHANNELS; i++)
	{
		if (i < 64)
			ChnSettings[i].Pan = 128;
		else
			ChnSettings[i].Pan = 256;
		ChnSettings[i].Volume = 64;
		ChnSettings[i].Flags = 0;
		//ChnSettings[i].Name[0] = 0;
		Chns[i].Pan = ChnSettings[i].Pan;
		Chns[i].GlobalVol = ChnSettings[i].Volume;
		Chns[i].Flags = ChnSettings[i].Flags;
		Chns[i].Volume = 256;
		Chns[i].CutOff = 0x7F;
	}
	for (UINT i = 0; i < p_IF->p_Head->smpnum; i++)
	{
		if (p_IF->p_Samples[i].PCM != NULL)
		{
			if (Ins[i].loopend > Ins[i].length)
				Ins[i].loopend = Ins[i].length;
			if (Ins[i].loopbegin + 3 >= Ins[i].loopend)
				Ins[i].loopbegin = Ins[i].loopend = 0;
			if (Ins[i].susloopend > Ins[i].length)
				Ins[i].susloopend = Ins[i].length;
			if (Ins[i].susloopbegin + 3 >= Ins[i].susloopend)
				Ins[i].susloopbegin = Ins[i].susloopend = 0;
		}
		else
			Ins[i].length = Ins[i].loopbegin = Ins[i].loopend =
			Ins[i].susloopbegin = Ins[i].susloopend = 0;
		if (Ins[i].loopend == 0)
			Ins[i].flags &= ~CHN_LOOP;
		if (Ins[i].susloopend == 0)
			Ins[i].flags &= ~CHN_SUSTAINLOOP;
		if (Ins[i].gvl > 64)
			Ins[i].gvl = 64;
	}
	while (Instruments > 0 && Headers[Instruments] == NULL)
		Instruments--;
	HighResRampingGlobalVolume = GlobalVolume << VOLUMERAMPPRECISION;
	GlobalVolumeDest = GlobalVolume;
	SamplesToGlobalVolRampDest = 0;
	BufferCount = 0;
	BufferDiff = 0;
	TickCount = MusicSpeed;
	switch (TempoMode)
	{
		case tempo_mode_alternative:
		{
			SamplesPerTick = 44100 / MusicTempo;
			break;
		}
		case tempo_mode_modern:
		{
			SamplesPerTick = 44100 * (60 / MusicTempo / (MusicSpeed * RowsPerBeat));
			break;
		}
		default:
		{
			/*44100 * 5 * 128 = 28224000*/
			SamplesPerTick = (28224000) / (MusicTempo << 8);
		}
	}
	if (RestartPos >= MAX_ORDERS || Order[RestartPos] >= MAX_PATTERNS)
		RestartPos = 0;
	Config->SetPluginMixLevels(PlugMixMode);
	InitPlayer(true);
	SetCurrentPos(0);
}

bool ISoundFile::SetResamplingMode(UINT Mode)
{
	DWORD d = SoundSetup & ~(SNDMIX_NORESAMPLING | SNDMIX_SPLINESRCMODE | SNDMIX_POLYPHASESRCMODE | SNDMIX_FIRFILTERSRCMODE);
	switch (Mode)
	{
		case SRCMODE_NEAREST:
		{
			d |= SNDMIX_NORESAMPLING;
			break;
		}
		case SRCMODE_SPLINE:
		{
			d |= SNDMIX_SPLINESRCMODE;
			break;
		}
		case SRCMODE_POLYPHASE:
		{
			d |= SNDMIX_POLYPHASESRCMODE;
			break;
		}
		case SRCMODE_FIRFILTER:
		{
			d |= SNDMIX_FIRFILTERSRCMODE;
			break;
		}
	}
	SoundSetup = d;
	return true;
}

void ISoundFile::UpdateAudioParameters(bool Reset)
{
	DWORD SS = SoundSetup;
	SoundSetup = 0;
	if (BitsPerSample != 8 && BitsPerSample != 32)
		BitsPerSample = 16;
	if ((SS & SOUNDSETUP_STREVERSE) != 0)
		SoundSetup |= SNDMIX_REVERSESTEREO;
	else
		SoundSetup &= ~SNDMIX_REVERSESTEREO;
	if ((SS & SOUNDSETUP_SOFTPANNING) != 0)
		SoundSetup |= SNDMIX_SOFTPANNING;
	else
		SoundSetup &= ~SNDMIX_SOFTPANNING;

	if ((SysInfo & SYSMIX_MMXEX) != 0)
		SetResamplingMode(SRCMODE_POLYPHASE);
	else if ((SysInfo & SYSMIX_ENABLEMMX) != 0)
		SetResamplingMode(SRCMODE_SPLINE);
	else
		SetResamplingMode(SRCMODE_LINEAR);
	SetDspEffects((Quality & QUALITY_SURROUND) != 0, (Quality & QUALITY_REVERB) != 0, (Quality & QUALITY_MEGABASS) != 0,
		(Quality & QUALITY_NOISEREDUCTION) != 0, (Quality & QUALITY_EQ) != 0);
	if (Reset == true)
		InitPlayer(true);
}

void ISoundFile::SetDspEffects(bool Surround, bool Reverb, bool MegaBass, bool NR, bool EQ)
{
	DWORD d = SoundSetup & ~(SNDMIX_SURROUND | SNDMIX_REVERB | SNDMIX_MEGABASS | SNDMIX_NOISEREDUCTION | SNDMIX_EQ);
	if (Surround == true)
		d |= SNDMIX_SURROUND;
	if (Reverb == true && (SysInfo & SYSMIX_ENABLEMMX) != 0)
		d |= SNDMIX_REVERB;
	if (MegaBass == true)
		d |= SNDMIX_MEGABASS;
	if (NR == true)
		d |= SNDMIX_NOISEREDUCTION;
	if (EQ == true)
		d |= SNDMIX_EQ;
	SoundSetup = d;
	InitPlayer(false);
}

DWORD QueryProcessorExtensions()
{
	static DWORD ProcExts = 0;
	static bool MMXChecked = false;

	if (MMXChecked == false)
	{
		__asm
		{
			pushfd
			pop eax
			mov ecx, eax
			xor eax, 0x00200000
			push eax
			popfd
			pushfd
			pop eax
			xor eax, ecx
			jz Done
			mov ProcExts, PROCSUPPORT_CPUID
			mov eax, 1
			push ebx
			cpuid
			pop ebx
			test edx, 0x00800000
			jz Done
			or ProcExts, PROCSUPPORT_MMX
			test edx, 0x02000000
			jz no_sse
			or ProcExts, (PROCSUPPORT_MMXEX | PROCSUPPORT_SSE)
			jmp Done
no_sse:
			mov eax, 0x80000000
			cpuid
			cmp eax, 0x80000000
			jbe Done
			mov eax, 0x80000001
			cpuid
			test edx, 0x80000000
			jz Done
			or ProcExts, PROCSUPPORT_3DNOW
			test edx, (1 << 2)
			jz Done
			or ProcExts, PROCSUPPORT_MMXEX
		}
Done:
		MMXChecked = true;
	}
	return ProcExts;
}

DWORD ISoundFile::InitSysInfo()
{
	OSVERSIONINFO vi;
	DWORD d = 0, ProcSupport;

	memset(&vi, 0x00, sizeof(vi));
	vi.dwOSVersionInfoSize = sizeof(vi);
	GetVersionEx(&vi);
	ProcSupport = QueryProcessorExtensions();
	switch (vi.dwPlatformId)
	{
		case VER_PLATFORM_WIN32s:
		{
			ProcSupport &= PROCSUPPORT_CPUID;
			break;
		}
		case VER_PLATFORM_WIN32_WINDOWS:
		{
			if (vi.dwMajorVersion < 4 || (vi.dwMajorVersion == 4 && vi.dwMinorVersion == 0))
				ProcSupport &= (PROCSUPPORT_CPUID | PROCSUPPORT_MMX | PROCSUPPORT_3DNOW | PROCSUPPORT_MMXEX);
			break;
		}
		case VER_PLATFORM_WIN32_NT:
		{
			if (vi.dwMajorVersion < 5)
				ProcSupport &= (PROCSUPPORT_CPUID | PROCSUPPORT_MMX | PROCSUPPORT_3DNOW | PROCSUPPORT_MMXEX);
			break;
		}
	}
	if ((ProcSupport & PROCSUPPORT_MMX) != 0)
		d |= (SYSMIX_ENABLEMMX | SYSMIX_FASTCPU);
	if ((ProcSupport & PROCSUPPORT_MMXEX) != 0)
		d |= SYSMIX_MMXEX;
	if ((ProcSupport & PROCSUPPORT_3DNOW) != 0)
		d |= SYSMIX_3DNOW;
	if ((ProcSupport & PROCSUPPORT_SSE) != 0)
		d |= SYSMIX_SSE;
	if ((ProcSupport & PROCSUPPORT_CPUID) == 0)
		d |= SYSMIX_SLOWCPU;
	SysInfo = d;
	return d;
}

bool ISoundFile::InitPlayer(bool Reset)
{
	if (InitTables == false)
	{
		SndMixInitializeTables();
		InitTables = true;
	}
	if (MaxMixChannels > MAX_CHANNELS)
		MaxMixChannels = MAX_CHANNELS;

	VolumeRampSamples = 42;
	DryROffsVol = DryLOffsVol = 0;
	RvbROffsVol = RvbLOffsVol = 0;
	InitializeDSP(Reset);
	InitPlugins = (Reset == true ? false : true);
	return true;
}

void ISoundFile::SndMixInitializeTables()
{
	WindowedFIR::InitTable();
	getsinc(KaiserSinc, 9.6377, WFIRCutoff);
	getsinc(DownSample13x, 8.5, 0.5);
	getsinc(DownSample2x, 2.7625, 0.425);
}

double ISoundFile::Zero(double y)
{
	double s = 1, ds = 1, d = 0;

	do
	{
		d = d + 2;
		ds = ds * (y * y) / (d * d);
		s = s + ds;
	}
	while (ds > 1e-7 * s);
	return s;
}

void ISoundFile::getsinc(short *Sinc, double Beta, double LowPassFactor)
{
	double ZeroBeta = Zero(Beta);
	double LPAt = 4.0 * atan(1.0) * LowPassFactor;
	for (int i = 0; i < 8 * SINC_PHASES; i++)
	{
		double FSinc;
		int x = 7 - (i & 7), n;
		x = (x * SINC_PHASES) + (i >> 3);
		if (x == 4 * SINC_PHASES)
			FSinc = 1.0;
		else
		{
			double y = (x - (4 * SINC_PHASES)) * (1.0 / SINC_PHASES);
			FSinc = sin(y * LPAt) * Zero(Beta * sqrt(1 - y * y * (1.0 / 16.0))) / (ZeroBeta * y * LPAt);
		}
		n = (int)(FSinc * LowPassFactor * (16384 * 256));
		*Sinc = (n + 0x80) >> 8;
		Sinc++;
	}
}

long ISoundFile::OnePoleLowPassCoef(long Scale, float g, float c, float s)
{
	float cosw;
	float ScaleOver1mg;
	long result;
	if (g > 0.999999F)
		return 0;
	__asm
	{
		fild Scale
		fld1
		fld g
		fld st(0)
		fmulp st(1), st(0)
		fst g
		fsubp st(1), st(0)
		fdivp st(1), st(0)
		fstp ScaleOver1mg
		fld c
		fld s
		fdivp st(1), st(0)
		fldpi
		fadd st(0), st(0)
		fmulp st(1), st(0)
		fcos
		fstp cosw
		fld g
		fadd st(0), st(0)
		fld1
		fld cosw
		fsubp st(1), st(0)
		fmulp st(1), st(0)
		fld g
		fmul st(0), st(0)
		fld1
		fld cosw
		fmul st(0), st(0)
		fsubp st(1), st(0)
		fmulp st(1), st(0)
		fsubp st(1), st(0)
		fsqrt
		fld g
		faddp st(1), st(0)
		fld1
		fsubrp st(1), st(0)
		fld ScaleOver1mg
		fmulp st(1), st(0)
		fistp result
	}
	return result;
}

long ISoundFile::BToLinear(long Scale, long dB)
{
	const float factor = 3.321928094887362304F / 2000.0F;
	long result;
	if (dB == 0)
		return Scale;
	if (dB <= -10000)
		return 0;
	__asm
	{
		fild dB;
		fld factor;
		fmulp st(1), st(0)
		fist result
		fisub result
		f2xm1
		fild result
		fild Scale
		fscale
		fstp st(1)
		fmul st(1), st(0)
		faddp st(1), st(0)
		fistp result
	}
	return result;
}

float ISoundFile::BToLinear(long dB)
{
	const float factor = 3.321928094887362304F / 2000.0F;
	long result;
	float res;
	if (dB == 0)
		return 1;
	if (dB <= -10000)
		return 0;
	__asm
	{
		fild dB
		fld factor
		fmulp st(1), st(0)
		fist result
		fisub result
		f2xm1
		fild result
		fld1
		fscale
		fstp st(1)
		fmul st(1), st(0)
		faddp st(1), st(0)
		fstp res
	}
	return res;
}

inline void ISoundFile::I3dl2_to_Generic(SNDMIX_REVERB_PROPERTIES *Reverb, ENVIRONMENTREVERB *Rvb, float OutputFreq, long MinRefDelay,
	long MaxRefDelay, long MinRvbDelay, long MaxRvbDelay, long TankLength)
{
	float DelayFactor, DelayFactorHF, DecayTimeHF, RefDelay;
	long Density, TailDiffusion, MaxLevel, ReverbDelay, ReflectionsDelay;
	long ReverbDecayTime;
	Rvb->ReverbLevel = Reverb->Reverb;
	Rvb->ReflectionsLevel = Reverb->Reflections;
	Rvb->RoomHF = Reverb->RoomHF;
	MaxLevel = (Rvb->ReverbLevel > Rvb->ReflectionsLevel ? Rvb->ReverbLevel : Rvb->ReflectionsLevel);
	if (MaxLevel < -600)
	{
		MaxLevel += 600;
		Rvb->ReverbLevel -= MaxLevel;
		Rvb->ReflectionsLevel -= MaxLevel;
	}
	Density = 8192 + (long)(79.31F * Reverb->Density);
	Rvb->PreDiffusion = Density;
	TailDiffusion = (long)((0.15F + Reverb->Diffusion * 0.0036F) * 32767.0F);
	if (TailDiffusion > 0x7F00)
		TailDiffusion = 0x7F00;
	Rvb->TankDiffusion = TailDiffusion;
	RefDelay = Reverb->ReflectionsDelay;
	if (RefDelay > 0.1F)
		RefDelay = 0.1F;
	ReverbDelay = (long)(Reverb->ReverbDelay * OutputFreq);
	ReflectionsDelay = (long)(RefDelay * OutputFreq);
	ReverbDecayTime = (long)(Reverb->DecayTime * OutputFreq);
	if (ReflectionsDelay < MinRefDelay)
	{
		ReverbDelay -= MinRefDelay - ReflectionsDelay;
		ReflectionsDelay = MinRefDelay;
	}
	if (ReflectionsDelay > MaxRefDelay)
	{
		ReverbDelay += ReflectionsDelay - MaxRefDelay;
		ReflectionsDelay = MaxRefDelay;
	}
	if (ReverbDelay < MinRvbDelay)
	{
		ReverbDecayTime -= MinRvbDelay - ReverbDelay;
		ReverbDelay = MinRvbDelay;
	}
	if (ReverbDelay > MaxRvbDelay)
	{
		ReverbDecayTime += ReverbDelay - MaxRvbDelay;
		ReverbDelay = MaxRvbDelay;
	}
	Rvb->ReverbDelay = ReverbDelay;
	Rvb->ReverbDecaySamples = ReverbDecayTime;
	for (UINT i = 0; i < ENVIRONMENT_NUMREFLECTIONS; i++)
	{
		ENVIRONMENTREFLECTION *Ref = &Rvb->Reflections[i];
		Ref->Delay = ReflectionsDelay + (ReflectionsPreset[i].DelayFactor * ReverbDelay + 50) / 100;
		Ref->GainLL = ReflectionsPreset[i].GainLL;
		Ref->GainRL = ReflectionsPreset[i].GainRL;
		Ref->GainLR = ReflectionsPreset[i].GainLR;
		Ref->GainRR = ReflectionsPreset[i].GainRR;
	}
	if (TankLength < 10)
		TankLength = 10;
	DelayFactor = (ReverbDecayTime <= TankLength ? 1.0F : (float)TankLength / (float)ReverbDecayTime);
	Rvb->ReverbDecay = (long)(pow(0.001F, DelayFactor) * 32768.0F);
	DecayTimeHF = (float)ReverbDecayTime * Reverb->DecayHFRatio;
	DelayFactorHF = (DecayTimeHF <= (float)TankLength ? 1.0F : (float)TankLength / DecayTimeHF);
	Rvb->ReverbDamping = pow(0.001F, DelayFactorHF);
}

void ISoundFile::InitializeReverb(bool Reset)
{
	static SNDMIX_REVERB_PROPERTIES *CurrentPreset = NULL;
	SNDMIX_REVERB_PROPERTIES *RvbPreset = &RvbPresets[ReverbType].Preset;

	if (RvbPreset != CurrentPreset || Reset == true)
	{
		ENVIRONMENTREVERB rvb;
		long RoomLP, ReflectionsGain = 0, ReverbGain = 0, ReverbDecay;
		ULONG TailDiffusion;
		float ReverbDamping;
		long DampingLowPass;

		RvbDownSample2x = false;
		CurrentPreset = RvbPreset;
		I3dl2_to_Generic(RvbPreset, &rvb, 44100, RVBMINREFDELAY, RVBMAXREFDELAY,
			RVBMINRVBDELAY, RVBMAXRVBDELAY, (RVBDIF1L_LEN + RVBDIF1R_LEN +
			RVBDIF2L_LEN + RVBDIF2R_LEN + RVBDLY1L_LEN + RVBDLY1R_LEN +
			RVBDLY2L_LEN + RVBDLY2R_LEN) / 2);
		ReverbDecaySamples = (RvbDownSample2x == true ? rvb.ReverbDecaySamples * 2 : rvb.ReverbDecaySamples);
		RoomLP = OnePoleLowPassCoef(32768, BToLinear(rvb.RoomHF), 5000, 44100);
		RefDelay.Coeffs[0] = (short)RoomLP;
		RefDelay.Coeffs[1] = (short)RoomLP;

		RefDelay.PreDifCoeffs[0] = (short)(rvb.PreDiffusion * 2);
		RefDelay.PreDifCoeffs[1] = (short)(rvb.PreDiffusion * 2);

		for (UINT i = 0; i < 8; i++)
		{
			SWRVBREFLECTION *Ref = &RefDelay.Reflections[i];
			Ref->Delay = Ref->DelayDest = rvb.Reflections[i].Delay;
			Ref->Gains[0] = rvb.Reflections[i].GainLL;
			Ref->Gains[1] = rvb.Reflections[i].GainRL;
			Ref->Gains[2] = rvb.Reflections[i].GainLR;
			Ref->Gains[3] = rvb.Reflections[i].GainRR;
		}
		LateReverb.ReverbDelay = rvb.ReverbDelay;
		if (rvb.ReflectionsLevel > -9000)
			ReflectionsGain = BToLinear(32768, rvb.ReflectionsLevel);
		RefDelay.MasterGain = ReflectionsGain;
		if (rvb.ReverbLevel > -9000)
			ReverbGain = BToLinear(32768, rvb.ReflectionsLevel);
		LateReverb.MasterGain = ReverbGain;
		TailDiffusion = rvb.TankDiffusion;
		if (TailDiffusion > 0x7F00)
			TailDiffusion = 0x7F00;
		LateReverb.DifCoeffs[0] = (short)TailDiffusion;
		LateReverb.DifCoeffs[1] = (short)TailDiffusion;
		LateReverb.DifCoeffs[2] = (short)TailDiffusion;
		LateReverb.DifCoeffs[3] = (short)TailDiffusion;
		LateReverb.Dif2InGains[0] = LateReverb.Dif2InGains[3] = 0x7000;
		LateReverb.Dif2InGains[1] = LateReverb.Dif2InGains[2] = 0x1000;
		ReverbDecay = rvb.ReverbDecay;
		if (ReverbDecay < 0)
			ReverbDecay = 0;
		if (ReverbDecay > 0x7FF0)
			ReverbDecay = 0x7FF0;
		LateReverb.DecayDC[0] = LateReverb.DecayDC[3] = (short)ReverbDecay;
		LateReverb.DecayDC[1] = LateReverb.DecayDC[2] = 0;
		ReverbDamping = rvb.ReverbDamping * rvb.ReverbDamping;
		DampingLowPass = OnePoleLowPassCoef(32768, ReverbDamping, 5000, 44100);
		if (DampingLowPass < 0x100)
			DampingLowPass = 0x100;
		if (DampingLowPass > 0x7F00)
			DampingLowPass = 0x7F00;
		LateReverb.DecayLP[0] = LateReverb.DecayLP[3] = (short)DampingLowPass;
		LateReverb.DecayLP[1] = LateReverb.DecayLP[2] = 0;
	}
	if (Reset == true)
	{
		ReverbSamples = 0;
		ReverbShutdown();
	}
	if (ReverbDecaySamples < 44100 * 5)
		ReverbDecaySamples = 44100 * 5;
}

void ISoundFile::InitializeDSP(bool Reset)
{
	if (ReverbType >= NUM_REVERBTYPES)
		ReverbType = 0;
	if (ProLogicDelay == 0)
		ProLogicDelay = 20;
	if (Reset == true)
		LeftNR = RightNR = 0;
	SurroundPos = SurroundSize = 0;
	if ((SoundSetup & SNDMIX_SURROUND) != 0)
	{
		memset(SurroundBuffer, 0x00, sizeof(SurroundBuffer));
		SurroundSize = (44100 * ProLogicDelay) / 1000;
		if (SurroundSize > SURROUNDBUFFERSIZE)
			SurroundSize = SURROUNDBUFFERSIZE;
		DolbyDepth = ProLogicDepth;
		if (DolbyDepth < 1)
			DolbyDepth = 1;
		if (DolbyDepth > 16)
			DolbyDepth = 16;
		ShelfEQ(1024, &DolbyHP_A1, &DolbyHP_B0, &DolbyHP_B1, 200, 44100, 0, 0.5F, 1);
		ShelfEQ(1024, &DolbyLP_A1, &DolbyLP_B0, &DolbyLP_B1, 7000, 44100, 1, 0.75F, 0);
		DolbyHP_X1 = DolbyHP_Y1 = DolbyLP_Y1 = 0;
		DolbyHP_B0 = (DolbyHP_B0 * DolbyDepth) >> 5;
		DolbyHP_B1 = (DolbyHP_B1 * DolbyDepth) >> 5;
		DolbyLP_B0 *= 2;
		DolbyLP_B1 *= 2;
	}
	InitializeReverb(Reset);
	if ((SoundSetup & SNDMIX_MEGABASS) != 0)
	{
		long a1 = 0, b1 = 0, b0 = 1024;
		int XBassCutOff = 50 + (XBassRange + 2) * 20;
		int XBassGain = XBassDepth;
		if (XBassGain < 2)
			XBassGain = 2;
		if (XBassGain > 8)
			XBassGain = 8;
		if (XBassCutOff < 60)
			XBassCutOff = 60;
		if (XBassCutOff > 600)
			XBassCutOff = 600;
		ShelfEQ(1024, &a1, &b0, &b1, XBassCutOff, 44100, 1.0F + (1.0F / 16.0F) * (0x0300 >> XBassGain), 1.0F, 0.0000001F);
		if (XBassGain > 5)
		{
			b0 >>= XBassGain - 5;
			b1 >>= XBassGain - 5;
		}
		XBassFlt_A1 = a1;
		XBassFlt_B0 = b0;
		XBassFlt_B1 = b1;
	}
	if (Reset == true)
		XBassFlt_X1 = XBassFlt_Y1 = DCRFlt_X1l = DCRFlt_X1r = DCRFlt_Y1l = DCRFlt_Y1r = 0;
}

DWORD ISoundFile::CutOffToFrequency(UINT CutOff, int flt_modifier)
{
	float fc;
	long freq;
	assert(CutOff < 128);
	if (SongFlags & SONG_EXFILTERRANGE)
		fc = 110.0F * powf(2.0F, 0.25F + ((float)(CutOff * (flt_modifier + 256))) / (20.0F * 512.0F));
	else
		fc = 100.0F * powf(2.0F, 0.25F + ((float)(CutOff * (flt_modifier + 256))) / (24.0F * 512.0F));
	freq = (long)fc;
	if (freq < 120)
		return 120;
	if (freq > 20000)
		return 20000;
	if (freq * 2 > 44100)
		freq = 44100 >> 1;
	return freq;
}

void ISoundFile::SetupChannelFilter(Channel *chn, bool Reset, int flt_modifier)
{
	float fs = 44100.0F;
	float fg, fb0, fb1, fc, dmpfac, d, e;

	fc = (float)CutOffToFrequency((chn->CutOff + chn->CutSwing) & 0x7F, flt_modifier);
	dmpfac = pow(10.0F, -((24.0F / 128.0F) * (float)((chn->Resonance + chn->ResSwing) & 0x7F)) / 20.0F);
	fc *= (float)(2.0 * M_PI / fs);
	d = (1.0F - (2.0F * dmpfac)) * fc;
	if (d > 2.0F) d = 2.0F;
	d = (2.0F * dmpfac - d) / fc;
	e = pow(1.0F / fc, 2.0F);

	fg = 1 + d + e;
	fb0 = (d + e + e) / fg;
	fb1 = -e / fg;
	fg = 1 / fg;

	if (chn->FilterMode == FLTMODE_HIGHPASS)
	{
		chn->Filter_A0 = (int)((1.0F - fg) * FILTER_PRECISION);
		chn->Filter_B0 = (int)(fb0 * FILTER_PRECISION);
		chn->Filter_B1 = (int)(fb1 * FILTER_PRECISION);
		chn->Filter_HP = -1;
	}
	else
	{
		chn->Filter_A0 = (int)(fg * FILTER_PRECISION);
		chn->Filter_B0 = (int)(fb0 * FILTER_PRECISION);
		chn->Filter_B1 = (int)(fb1 * FILTER_PRECISION);
		chn->Filter_HP = 0;
	}
	if (Reset == true)
	{
		chn->Filter_Y1 = chn->Filter_Y2 = 0;
		chn->Filter_Y3 = chn->Filter_Y4 = 0;
	}
	chn->Flags |= CHN_FILTER;
}

bool ISoundFile::MuteChannel(UINT nChn, bool Mute)
{
	DWORD d = (Mute == true ? CHN_MUTE : 0), i;
	if (nChn >= Channels)
		return false;
	if (d != (ChnSettings[nChn].Flags & CHN_MUTE))
	{
		if (d != 0)
			ChnSettings[nChn].Flags |= CHN_MUTE;
		else
			ChnSettings[nChn].Flags &= ~CHN_MUTE;
	}
	if (d != 0)
		Chns[nChn].Flags |= CHN_MUTE;
	else
		Chns[nChn].Flags &= ~CHN_MUTE;
	for (i = Channels; i < MAX_CHANNELS; i++)
	{
		if (Chns[i].MasterChn == nChn + 1)
		{
			if (d != 0)
				Chns[i].Flags |= CHN_MUTE;
			else
				Chns[i].Flags &= ~CHN_MUTE;
		}
	}
	return true;
}

bool ISoundFile::IsChannelMuted(UINT nChn)
{
	if (nChn >= Channels)
		return true;
	return ((ChnSettings[nChn].Flags & CHN_MUTE) != 0 ? true : false);
}

void ISoundFile::HandlePatternTransitionEvents()
{
	if (PatternTransitionOccurred == true)
	{
		if (SeqOverride > 0 && SeqOverride <= MAX_ORDERS)
		{
			if ((SongFlags & SONG_PATTERNLOOP) != 0)
				Pattern = Order[SeqOverride - 1];
			NextPattern = SeqOverride - 1;
			SeqOverride = 0;
		}
		PatternTransitionOccurred = false;
	}
}

UINT ISoundFile::GetPeriodFromNote(UINT note, int FineTune, UINT C4Speed)
{
	if (note == 0 || note > 0xF0)
		return 0;
	note--;
	if ((SongFlags & SONG_LINEARSLIDES) != 0)
		return (FreqTable[note % 12] << 5) >> (note / 12);
	else
	{
		if (C4Speed == 0)
			C4Speed = 8363;
		return muldiv(8363, (FreqTable[note % 12] << 5), C4Speed << (note / 12));
	}
}

UINT ISoundFile::GetNoteFromPeriod(UINT period)
{
	if (period == 0)
		return 0;
	for (UINT i = 1; i < 120; i++)
	{
		long n = GetPeriodFromNote(i, 0, 0);
		if (n > 0 && n <= (long)period)
			return i;
	}
	return 120;
}

UINT ISoundFile::GetFreqFromPeriod(UINT period, UINT C4Speed, int PeriodFrac)
{
	if (period == 0)
		return 0;
	if ((SongFlags & SONG_LINEARSLIDES) != 0)
	{
		if (C4Speed == 0)
			C4Speed = 8363;
		return muldiv(C4Speed, 1712L << 8, (period << 8) + PeriodFrac);
	}
	else
		return muldiv(8363, 1712L << 8, (period << 8) + PeriodFrac);
}

void ISoundFile::KeyOff(UINT nChn)
{
	Channel *chn = &Chns[nChn];
	bool KeyOn = ((chn->Flags & CHN_KEYOFF) != 0 ? false : true);
	chn->Flags |= CHN_KEYOFF;
	if (chn->Header != NULL && (chn->Flags & CHN_VOLENV) == 0)
		chn->Flags |= CHN_NOTEFADE;
	if (chn->Length == 0)
		return;
	if ((chn->Flags & CHN_SUSTAINLOOP) != 0 && chn->Instrument != NULL && KeyOn == true)
	{
		ITSampleStruct *smp = chn->Instrument;
		if ((smp->flags & CHN_LOOP) != 0)
		{
			if ((smp->flags & CHN_PINGPONGLOOP) != 0)
				chn->Flags |= CHN_PINGPONGLOOP;
			else
				chn->Flags &= ~(CHN_PINGPONGLOOP | CHN_PINGPONGFLAG);
			chn->Flags |= CHN_LOOP;
			chn->Length = smp->length;
			chn->LoopStart = smp->loopbegin;
			chn->LoopEnd = smp->loopend;
			if (chn->Length > chn->LoopEnd)
				chn->Length = chn->LoopEnd;
		}
		else
		{
			chn->Flags &= ~(CHN_LOOP | CHN_PINGPONGLOOP | CHN_PINGPONGFLAG);
			chn->Flags = smp->length;
		}
	}
	if (chn->Header)
	{
		ITInstrument *env = chn->Header;
		if ((env->volenv.flags & 2) != 0 && env->fadeout != 0)
			chn->Flags |= CHN_NOTEFADE;
	}
}

UINT ISoundFile::GetNNAChannel(UINT nChn)
{
	Channel *chn = &Chns[nChn];
	Channel *chn_end = &Chns[Channels];
	UINT result = 0;
	DWORD vol = 64 * 65536;
	DWORD envpos = 0xFFFFFF;
	for (UINT i = Channels; i < MAX_CHANNELS; i++, chn_end++)
	{
		if (chn_end->Length == 0)
			return i;
	}
	if (chn->FadeOutVol == 0)
		return 0;
	chn_end = &Chns[Channels];
	for (UINT i = Channels; i < MAX_CHANNELS; i++, chn_end++)
	{
		DWORD v = chn_end->Volume;
		if (chn_end->FadeOutVol == 0)
			return i;
		if ((chn_end->Flags & CHN_NOTEFADE) != 0)
			v = v * chn_end->FadeOutVol;
		else
			v <<= 16;
		if (chn_end->Flags && CHN_LOOP) v >>= 1;
		if (v < vol || (v == vol && chn_end->VolEnvPosition > envpos))
		{
			envpos = chn_end->VolEnvPosition;
			vol = v;
			result = i;
		}
	}
	return result;
}

void ISoundFile::CheckNNA(UINT nChn, BYTE instr, BYTE note, bool ForceCut)
{
	Channel *chn = &Chns[nChn], *p;
	ITInstrument *Header;
	BYTE *Sample;

	if (note > 0x80 || note < 1)
		return;
	if (instr >= Instruments)
		instr = 0;
	Sample = chn->Sample;
	Header = chn->Header;
	if (instr != 0 && note != 0)
	{
		Header = Headers[instr];
		if (Header != NULL)
		{
			UINT n = 0;
			if (note <= 0x80)
			{
				n = Header->keyboard[(note - 1) * 2 + 1];
				note = Header->keyboard[(note - 1) * 2] + 1;
				if (n != 0 && n < MAX_SAMPLES) Sample = p_IF->p_Samples[n].PCM;
			}
		}
		else
			Sample = NULL;
	}
	p = chn;
	if (chn->Flags & CHN_MUTE)
		return;

	for (UINT i = nChn; i < MAX_CHANNELS; p++, i++)
	{
		if (i >= Channels || p == chn)
		{
			if ((p->MasterChn == nChn + 1 || p == chn) && p->Header != NULL)
			{
				bool OK = false;
				switch (p->Header->dct)
				{
					case DCT_NOTE:
					{
						if (note != 0 && p->Note == note && Header == p->Header)
							OK = true;
						break;
					}
					case DCT_SAMPLE:
					{
						if (Sample != NULL && Sample == p->Sample)
							OK = true;
						break;
					}
					case DCT_INSTRUMENT:
					{
						if (Header == p->Header)
							OK = true;
						break;
					}
					case DCT_PLUGIN:
					{
						if ((Header->mch - 128) != 0 && Header->mch == p->Header->mch)
							OK = true;
						break;
					}
				}
				if (OK == true)
				{
					switch (p->Header->dca)
					{
						case DNA_NOTECUT:
						{
							KeyOff(i);
							p->Volume = 0;
							break;
						}
						case DNA_NOTEOFF:
						{
							KeyOff(i);
							break;
						}
						case DNA_NOTEFADE:
						{
							p->Flags |= CHN_NOTEFADE;
							break;
						}
					}
					if (p->Volume == 0)
					{
						p->FadeOutVol = 0;
						p->Flags |= (CHN_NOTEFADE | CHN_FASTVOLRAMP);
					}
				}
			}
		}
	}
	if (chn->Volume != 0 && chn->Length != 0)
	{
		UINT n = GetNNAChannel(nChn);
		if (n != 0)
		{
			Channel *p = &Chns[n];
			*p = *chn;
			p->Flags &= ~(CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO | CHN_MUTE | CHN_PORTAMENTO);
			p->Flags |= (chn->Flags & CHN_MUTE) | (chn->Flags & CHN_NOFX);

			p->MasterChn = nChn + 1;
			p->Command = 0;
			switch (chn->NNA)
			{
				case NNA_NOTEOFF:
				{
					KeyOff(n);
					break;
				}
				case NNA_NOTECUT:
				{
					p->FadeOutVol = 0;
				}
				case NNA_NOTEFADE:
				{
					p->Flags |= CHN_NOTEFADE;
					break;
				}
			}
			if (p->Volume == 0)
			{
				p->FadeOutVol = 0;
				p->Flags |= (CHN_NOTEFADE | CHN_FASTVOLRAMP);
			}
			chn->Length = chn->Pos = chn->PosLo = 0;
			chn->ROfs = chn->LOfs = 0;
		}
	}
}

inline BYTE ISoundFile::SampleToChannelFlags(const ITSampleStruct * const smp)
{
	BYTE ret = 0;
	if ((smp->flags & 0x10) != 0)
		ret |= CHN_LOOP;
	if ((smp->flags & 0x20) != 0)
		ret |= CHN_SUSTAINLOOP;
	if ((smp->flags & 0x40) != 0)
		ret |= CHN_PINGPONGLOOP;
	if ((smp->flags & 0x80) != 0)
		ret |= CHN_PINGPONGSUSTAIN;
	if ((smp->dfp & 0x80) != 0)
		ret |= CHN_PANNING;
	if ((smp->flags & 0x02) != 0)
		ret |= CHN_16BIT;
	if ((smp->flags & 0x04) != 0)
		ret |= CHN_STEREO;
	return ret;
}

inline BYTE ISoundFile::SampleToMixVibType(const ITSampleStruct * const smp)
{
	return autovibit2xm[smp->vit & 7];
}

inline BYTE ISoundFile::SampleToMixVibDepth(const ITSampleStruct * const smp)
{
	return smp->vid & 0x7F;
}

inline BYTE ISoundFile::SampleToMixVibSweep(const ITSampleStruct * const smp)
{
	return (smp->vir + 3) / 4;
}

void ISoundFile::InstrumentChange(Channel *chn, UINT instr_, bool Porta, bool UpdVol, bool ResetEnv)
{
	bool InstrumentChanged = false;
	ITInstrument *env;
	ITSampleStruct *smp;
	UINT note, instr = instr_ - 1;

	if (instr >= MAX_INSTRUMENTS)
		return;
	env = Headers[instr];
	smp = &Ins[instr];
	note = chn->NewNote;
	if (env != NULL && note != 0 && note <= 128)
	{
		UINT n;
		if (env->keyboard[(note - 1) * 2] >= 0xFE)
			return;
		n = env->keyboard[(note - 1) * 2 + 1];
		smp = ((n != 0 && n <= p_IF->p_Head->smpnum) ? &Ins[n - 1] : NULL);
	}
	else if (Instruments != 0)
	{
		if (note >= 0xFE)
			return;
		smp = NULL;
	}
	if (UpdVol == true)
	{
		chn->Volume = (smp != NULL ? smp->vol << 2 : 0);
		if (chn->Volume > 256)
			chn->Volume = 256;
	}
	if (env != chn->Header)
	{
		InstrumentChanged = true;
		chn->Header = env;
	}
	chn->NewInst = 0;

	if (env != NULL && (env->mch - 128 != 0 || smp != NULL))
		chn->NNA = env->nna;

	if (smp != NULL)
	{
		if (env != NULL)
		{
			chn->InsVol = (smp->gvl * env->gbv) >> 6;
			if (smp->dfp < 0x80)
			{
				chn->Pan = (smp->dfp & 0x7F) << 2;
				if (chn->Pan > 256)
					chn->Pan = 256;
			}
		}
		else
			chn->InsVol = smp->gvl;
		if ((smp->flags & CHN_PANNING) != 0)
		{
			chn->Pan = (smp->dfp & 0x7F) << 2;
			if (chn->Pan > 256)
				chn->Pan = 256;
		}
	}
	if (ResetEnv == true)
	{
		if (Porta == false || (SongFlags & SONG_ITCOMPATMODE) != 0 || chn->Length != 0 ||
			((chn->Flags & CHN_NOTEFADE) != 0 && chn->FadeOutVol == 0))
		{
			chn->Flags |= CHN_FASTVOLRAMP;
			if (InstrumentChanged == false && env != NULL && (chn->Flags & (CHN_KEYOFF | CHN_NOTEFADE)) == 0)
			{
				if ((env->volenv.flags & 8) == 0)
					chn->VolEnvPosition = 0;
				if ((env->panenv.flags & 8) == 0)
					chn->PanEnvPosition = 0;
				if ((env->pitchenv.flags & 8) == 0)
					chn->PitchEnvPosition = 0;
			}
			else
			{
				chn->VolEnvPosition = 0;
				chn->PanEnvPosition = 0;
				chn->PitchEnvPosition = 0;
			}
			chn->AutoVibDepth = 0;
			chn->AutoVibPos = 0;
		}
		else if (env != NULL && (env->volenv.flags & 1) == 0)
		{
			chn->VolEnvPosition = 0;
			chn->AutoVibDepth = 0;
			chn->AutoVibPos = 0;
		}
	}
	if (smp == NULL)
	{
		chn->Instrument = NULL;
		chn->InsVol = 0;
		return;
	}
	if (Porta == true && smp == chn->Instrument)
		return;
	else
	{
		chn->Flags &= ~(CHN_KEYOFF | CHN_NOTEFADE | CHN_VOLENV | CHN_PANENV | CHN_PITCHENV);
		chn->Flags = (chn->Flags & 0xFFFFFF00) | SampleToChannelFlags(smp);
		if (env != NULL)
		{
			if ((env->volenv.flags & 1) != 0)
				chn->Flags |= CHN_VOLENV;
			if ((env->panenv.flags & 1) != 0)
				chn->Flags |= CHN_PANENV;
			if ((env->pitchenv.flags & 1) != 0)
				chn->Flags |= CHN_PITCHENV;
			if ((env->pitchenv.flags & 1) != 0 && (env->pitchenv.flags & 0x80) != 0)
			{
				if (chn->CutOff == 0)
					chn->CutOff = 0x7F;
			}
			if (env->ifc & 0x80)
				chn->CutOff = env->ifc & 0x7F;
			if (env->ifr & 0x80)
				chn->Resonance = env->ifr & 0x7F;
		}
		chn->VolSwing = chn->PanSwing = 0;
		chn->ResSwing = chn->CutSwing = 0;
	}
	chn->Instrument = smp;
	chn->Length = smp->length;
	chn->LoopStart = smp->loopbegin;
	chn->LoopEnd = smp->loopend;
	chn->C4Speed = smp->C5Speed;
	chn->Sample = p_IF->p_Samples[instr].PCM;
	chn->FineTune = 0;
	if ((chn->Flags & CHN_SUSTAINLOOP) != 0)
	{
		chn->LoopStart = smp->susloopbegin;
		chn->LoopEnd = smp->susloopend;
		chn->Flags |= CHN_LOOP;
		if ((chn->Flags & CHN_PINGPONGSUSTAIN) != 0)
			chn->Flags |= CHN_PINGPONGLOOP;
	}
	if ((chn->Flags & CHN_LOOP) != 0 && chn->LoopEnd < chn->Length)
		chn->Length = chn->LoopEnd;
}

void ISoundFile::ProcessMidiOut(UINT nChn, Channel *chn)
{
	Command *cmd;
	if (chn->Flags & CHN_MUTE)
		return;
	if (Instruments == 0 || Pattern >= p_IF->p_Head->patnum || Row >= p_IF->p_Paterns[Pattern].nRows || p_IF->p_Paterns[Pattern].p_Commands == NULL)
		return;
	cmd = p_IF->p_Paterns[Pattern].p_Commands + Row * Channels + nChn;
	if (cmd->note != 0)
	{
		ITInstrument *env = chn->Header;
		if (cmd->instr != 0 && cmd->instr < p_IF->p_Head->smpnum)
			env = Headers[cmd->instr];
	}
}

int ISoundFile::PatternLoop(Channel *chn, UINT param)
{
	if (param != 0)
	{
		if (chn->PaternLoopCount != 0)
		{
			chn->PaternLoopCount--;
			if (chn->PaternLoopCount == 0)
				return -1;
		}
		else
		{
			Channel *p = Chns;
			for (UINT i = 0; i < Channels; i++, p++)
			{
				if (p != chn)
				{
					if (p->PaternLoopCount != 0)
						return -1;
				}
			}
			chn->PaternLoopCount = param;
		}
		return chn->PaternLoop;
	}
	else
		chn->PaternLoop = Row;
	return -1;
}

void ISoundFile::NoteChange(UINT nChn, int note, bool Porta, bool ResetEnv, bool Manual)
{
	Channel * const chn = &Chns[nChn];
	ITSampleStruct *smp = chn->Instrument;
	ITInstrument *env = chn->Header;
	UINT period;

	if (note < 1)
		return;
	if (env != NULL && note <= 0x80)
	{
		UINT n = env->keyboard[(note - 1) * 2 + 1];
		if (n != 0 && n < MAX_SAMPLES) smp = &Ins[n];
		note = env->keyboard[(note - 1) * 2];
	}
	if (note >= 0x80)
	{
		KeyOff(nChn);
		if (note == 0xFE)
		{
			chn->Flags |= (CHN_NOTEFADE | CHN_FASTVOLRAMP);
			chn->FadeOutVol = 0;
		}
		return;
	}
	if (note < 1) note = 1;
	if (note > 132) note = 132;
	chn->Note = note;

	chn->NewInst = 0;
	period = GetPeriodFromNote(note, chn->FineTune, chn->C4Speed);
	if (smp == NULL)
		return;
	if (period != 0)
	{
		if (Porta == false || chn->Period == 0)
			chn->Period = period;
		chn->PortamentoDest = period;
		if (Porta == false || chn->Length == 0)
		{
			chn->Instrument = smp;
			chn->Sample = p_IF->p_Samples[(smp - p_IF->p_Samp)].PCM;
			chn->Length = smp->length;
			chn->LoopEnd = smp->length;
			chn->LoopStart = 0;
			chn->Flags = (chn->Flags & 0xFFFFFF00) | (smp->flags & 0xFF);
			if (chn->Flags & CHN_SUSTAINLOOP)
			{
				chn->LoopStart = smp->susloopbegin;
				chn->LoopEnd = smp->susloopend;
				chn->Flags &= ~CHN_PINGPONGLOOP;
				chn->Flags |= CHN_LOOP;
				if (chn->Flags & CHN_PINGPONGSUSTAIN)
					chn->Flags |= CHN_PINGPONGLOOP;
				if (chn->Length > chn->LoopEnd)
					chn->Length = chn->LoopEnd;
			}
			else if (chn->Flags & CHN_LOOP)
			{
				chn->LoopStart = smp->loopbegin;
				chn->LoopEnd = smp->loopend;
				if (chn->Length > chn->LoopEnd)
					chn->Length = chn->LoopEnd;
			}
			chn->Pos = 0;
			chn->PosLo = 0;
			if (chn->VibratoType < 4)
				chn->VibratoPos = (SongFlags & SONG_ITOLDEFFECTS ? 0x10 : 0);
			if (chn->TremoloType < 4)
				chn->TremoloPos = 0;
		}
		if (chn->Pos >= chn->Length)
			chn->Pos = chn->Length;
	}
	else
		Porta = false;
	if (Porta == false || ((chn->Flags && CHN_NOTEFADE) != 0 && chn->FadeOutVol != 0) ||
		((SongFlags & SONG_ITCOMPATMODE) != 0 && chn->RowInstr != 0))
	{
		if (chn->Flags & CHN_NOTEFADE && chn->FadeOutVol == 0)
		{
			chn->VolEnvPosition = 0;
			chn->PanEnvPosition = 0;
			chn->PitchEnvPosition = 0;
			chn->AutoVibDepth = 0;
			chn->AutoVibPos = 0;
			chn->Flags &= ~CHN_NOTEFADE;
			chn->FadeOutVol = 0xFFFF;
		}
		if (Porta == false || (SongFlags & SONG_ITCOMPATMODE) == 0 || chn->RowInstr != 0)
		{
			chn->Flags &= ~CHN_NOTEFADE;
			chn->FadeOutVol = 0xFFFF;
		}
	}
	chn->Flags &= ~(CHN_EXTRALOUD | CHN_KEYOFF);
	if (Porta == false)
	{
		bool flt;
		chn->Flags &= ~CHN_FILTER;
		chn->Flags |= CHN_FASTVOLRAMP;
		chn->RetrigCount = 0;
		chn->TremorCount = 0;
		if (ResetEnv == true)
		{
			chn->VolSwing = chn->PanSwing = 0;
			chn->ResSwing = chn->CutSwing = 0;
			if (env != NULL)
			{
				if ((env->volenv.flags & 8) != 0)
					chn->VolEnvPosition = 0;
				if ((env->panenv.flags & 8) != 0)
					chn->PanEnvPosition = 0;
				if ((env->pitchenv.flags & 8) != 0)
					chn->PitchEnvPosition = 0;
				if (env->rv != 0)
				{
					int d = (env->rv * ((rand() & 0xFF) - 0x7F)) / 128;
					chn->VolSwing = ((d * chn->Volume + 1) / 128);
				}
				if (env->rp != 0)
					chn->PanSwing = (env->rp * ((rand() & 0xFF) - 0x7F)) / 128;
			}
			chn->AutoVibDepth = 0;
			chn->AutoVibPos = 0;
		}
		chn->LeftVol = chn->RightVol = 0;
		flt = ((SongFlags & SONG_MPTFILTERMODE) != 0 ? false : true);
		if (env != NULL)
		{
			if ((env->ifr & 0x80) != 0)
			{
				chn->Resonance = env->ifr & 0x7F;
				flt = true;
			}
			if ((env->ifc & 0x80) != 0)
			{
				chn->CutOff = env->ifc & 0x7F;
				flt = true;
			}
		}
		else
		{
			chn->VolSwing = chn->PanSwing = 0;
			chn->CutSwing = chn->ResSwing = 0;
		}
		if (chn->CutOff < 0x7F && flt == true)
			SetupChannelFilter(chn, true);
	}
	if (Manual == true)
		chn->Flags &= ~CHN_MUTE;
}

void ISoundFile::GlobalVolSlide(UINT param)
{
	long GlbSlide = 0;
	if (param != 0)
		OldGlbVolSlide = param;
	else
		param = OldGlbVolSlide;
	if ((param & 0x0F) == 0x0F && (param & 0xF0) != 0)
	{
		if ((SongFlags & SONG_FIRSTTICK) != 0)
			GlbSlide = (param >> 4) * 2;
	}
	else if ((param & 0xF0) == 0xF0 && (param & 0x0F) != 0)
	{
		if ((SongFlags & SONG_FIRSTTICK) != 0)
			GlbSlide = -(int)(param & 0x0F) * 2;
	}
	else if ((SongFlags & SONG_FIRSTTICK) == 0)
	{
		if ((param & 0xF0) != 0)
			GlbSlide = ((param & 0xF0) >> 4) * 2;
		else
			GlbSlide = -(int)(param & 0x0F) * 2;
	}
	if (GlbSlide != 0)
	{
		GlbSlide += GlobalVolume;
		if (GlbSlide < 0)
			GlbSlide = 0;
		if (GlbSlide > 256)
			GlbSlide = 256;
		GlobalVolume = GlbSlide;
	}
}

DWORD ISoundFile::IsSongFinished(UINT StartOrder, UINT StartRow)
{
	UINT Ord;
	for (Ord = StartOrder; Ord < MAX_ORDERS; Ord++)
	{
		UINT Pat = Order[Ord];
		if (Pat != 0xFE)
		{
			Command *cmd;
			if (Pat >= p_IF->p_Head->patnum)
				break;
			cmd = Patterns[Pat].p_Commands;
			if (cmd != NULL)
			{
				UINT len = Patterns[Pat].nRows * Channels;
				UINT pos = (Ord == StartOrder ? StartRow : 0);
				pos *= Channels;
				while (pos < len)
				{
					UINT cmnd;
					if (cmd[pos].note != 0 || cmd[pos].volcmd != 0)
						return 0;
					cmnd = cmd[pos].command;
					if (cmnd == CMD_MODCMDEX)
					{
						UINT cmdex = cmd[pos].param & 0xF0;
						if (cmdex == 0 || cmdex == 0x60 || cmdex == 0xE0 || cmdex == 0xF0)
							cmnd = 0;
					}
					if (cmnd != 0 && cmnd != CMD_SPEED && cmnd != CMD_TEMPO)
						return 0;
					pos++;
				}
			}
		}
	}
	return (Ord < MAX_ORDERS) ? Ord : MAX_ORDERS - 1;
}

bool ISoundFile::GlobalFadeSong(UINT msec)
{
	if ((SongFlags & SONG_GLOBALFADE) != 0)
		return false;
	GlobalFadeMaxSamples = muldiv(msec, 44100, 1000);
	GlobalFadeSamples = GlobalFadeMaxSamples;
	SongFlags |= SONG_GLOBALFADE;
	return true;
}

void ISoundFile::SetSpeed(UINT param)
{
	if (param == 0 || param >= 0x80)
	{
		if (RepeatCount == 0 && IsSongFinished(CurrentPattern, Row + 1) != 0)
			GlobalFadeSong(1000);
	}
	if (param != 0 && param <= 256)
		MusicSpeed = param;
}

void ISoundFile::SetTempo(UINT param)
{
	if (param >= 0x20 && TickCount == 0)
		MusicTempo = param;
	else if (param < 0x20 && TickCount != 0)
	{
		if ((param & 0xF0) == 0x10)
		{
			MusicTempo += param & 0x0F;
			if (MusicTempo > 512)
				MusicTempo = 512;
		}
		else
		{
			MusicTempo -= param & 0x0F;
			if (MusicTempo < 32)
				MusicTempo = 32;
		}
	}
}

void ISoundFile::FineVolumeUp(Channel *chn, UINT param)
{
	if (param != 0)
		chn->OldFineVolUpDown = param;
	else
		param = chn->OldFineVolUpDown;
	if ((SongFlags & SONG_FIRSTTICK) != 0)
	{
		chn->Volume += param * 4;
		if (chn->Volume > 256)
			chn->Volume = 256;
	}
}

void ISoundFile::FineVolumeDown(Channel *chn, UINT param)
{
	if (param != 0)
		chn->OldFineVolUpDown = param;
	else
		param = chn->OldFineVolUpDown;
	if ((SongFlags & SONG_FIRSTTICK) != 0)
	{
		chn->Volume -= param * 4;
		if (chn->Volume < 0)
			chn->Volume = 0;
	}
}

void ISoundFile::VolumeSlide(Channel *chn, UINT param)
{
	long NewVolume = chn->Volume;
	if (param != 0)
		chn->OldVolumeSlide = param;
	else
		param = chn->OldVolumeSlide;
	if ((param & 0x0F) == 0x0F)
	{
		if ((param & 0xF0) != 0)
		{
			FineVolumeUp(chn, (param >> 4));
			return;
		}
		else if ((SongFlags & SONG_FIRSTTICK) != 0 && (SongFlags & SONG_FASTVOLSLIDES) == 0)
			NewVolume -= 0x0F * 4;
	}
	else if ((param & 0xF0) == 0xF0)
	{
		if ((param & 0x0F) != 0)
		{
			FineVolumeDown(chn, (param & 0x0F));
			return;
		}
		else if ((SongFlags & SONG_FIRSTTICK) != 0 && (SongFlags & SONG_FASTVOLSLIDES) == 0)
			NewVolume += 0x0F * 4;
	}
	if ((SongFlags & SONG_FIRSTTICK) == 0 || (SongFlags & SONG_FASTVOLSLIDES) != 0)
	{
		if ((param & 0x0F) != 0)
			NewVolume -= (param & 0x0F) * 4;
		else
			NewVolume += (param & 0xF0) >> 2;
	}
	if (NewVolume < 0)
		NewVolume = 0;
	if (NewVolume > 256)
		NewVolume = 256;
	chn->Volume = NewVolume;
}

void ISoundFile::Vibrato(Channel *chn, UINT param)
{
	if ((param & 0x0F) != 0)
		chn->VibratoDepth = (param & 0x0F) * 4;
	if ((param & 0xF0) != 0)
		chn->VibratoSpeed = (param >> 4) & 0x0F;
	chn->Flags |= CHN_VIBRATO;
}

void ISoundFile::FineVibrato(Channel *chn, UINT param)
{
	if ((param & 0x0F) != 0)
		chn->VibratoDepth = param & 0x0F;
	if ((param & 0xF0) != 0)
		chn->VibratoSpeed = (param >> 4) & 0x0F;
	chn->Flags |= CHN_VIBRATO;
}

void ISoundFile::PanningSlide(Channel *chn, UINT param)
{
	long PanSlide = 0;
	if (param != 0)
		chn->OldPanSlide = param;
	else
		param = chn->OldPanSlide;
	if ((param & 0x0F) == 0x0F && (param & 0xF0) != 0 && (SongFlags & SONG_FIRSTTICK) != 0)
	{
		param = (param & 0xF0) >> 2;
		PanSlide = -(long)param;
	}
	else if ((param & 0xF0) == 0xF0 && (param & 0x0F) != 0 && (SongFlags & SONG_FIRSTTICK) != 0)
		PanSlide = (param & 0x0F) << 2;
	else if ((SongFlags & SONG_FIRSTTICK) == 0)
	{
		if ((param & 0x0F) != 0)
			PanSlide = (param & 0x0F) << 2;
		else
			PanSlide = -(long)((param & 0xF0) >> 2);
	}
	if (PanSlide != 0)
	{
		PanSlide += chn->Pan;
		if (PanSlide < 0)
			PanSlide = 0;
		if (PanSlide > 256)
			PanSlide = 256;
		chn->Pan = PanSlide;
	}
}

void ISoundFile::DoFreqSlide(Channel *chn, long FreqSlide)
{
	if (chn->Period == 0)
		return;
	if ((SongFlags & SONG_LINEARSLIDES) != 0)
	{
		long OldPeriod = chn->Period;
		if (FreqSlide < 0)
		{
			UINT n = (-FreqSlide) >> 2;
			if (n != 0)
			{
				if (n > 255)
					n = 255;
				//(a * b + c / 2) / c
				chn->Period = (chn->Period * LinearSlideDownTable[n] + 32768) / 65536;
				if (chn->Period == OldPeriod)
					chn->Period = OldPeriod - 1;
			}
		}
		else
		{
			UINT n = (FreqSlide) >> 2;
			if (n != 0)
			{
				if (n > 255) n = 255;
				chn->Period = (chn->Period * LinearSlideUpTable[n] + 32768) / 65536;
				if (chn->Period == OldPeriod)
					chn->Period = OldPeriod + 1;
			}
		}
	}
	else
		chn->Period += FreqSlide;
	if (chn->Period < 1)
	{
		chn->Period = 1;
		chn->Flags |= CHN_NOTEFADE;
		chn->FadeOutVol = 0;
	}
}

void ISoundFile::Tremolo(Channel *chn, UINT param)
{
	if ((param & 0x0F) != 0)
		chn->TremoloDepth = (param & 0x0F) << 2;
	if ((param & 0xF0) != 0)
		chn->TremoloSpeed = (param >> 4) & 0x0F;
	chn->Flags |= CHN_TREMOLO;
}

void ISoundFile::ChannelVolSlide(Channel *chn, UINT param)
{
	long ChnSlide = 0;
	if (param != 0)
		chn->OldChnVolSlide = param;
	else
		param = chn->OldChnVolSlide;
	if ((param & 0x0F) == 0x0F && (param & 0xF0) != 0)
	{
		if ((SongFlags & SONG_FIRSTTICK) != 0)
			ChnSlide = param >> 4;
	}
	else if ((SongFlags & SONG_FIRSTTICK) == 0)
	{
		if ((param & 0x0F) != 0)
			ChnSlide = -(int)(param & 0x0F);
		else
			ChnSlide = (param & 0xF0) >> 4;
	}
	if (ChnSlide != 0)
	{
		ChnSlide += chn->GlobalVol;
		if (ChnSlide < 0)
			ChnSlide = 0;
		if (ChnSlide > 64)
			ChnSlide = 64;
		chn->GlobalVol = ChnSlide;
	}
}

void ISoundFile::Panbrello(Channel *chn, UINT param)
{
	if ((param & 0x0F) != 0)
		chn->PanbrelloDepth = param & 0x0F;
	if ((param & 0xF0) != 0)
		chn->PanbrelloSpeed = (param >> 4) & 0x0F;
	chn->Flags |= CHN_PANBRELLO;
}

void ISoundFile::ExtraFinePortamentoUp(Channel *chn, UINT param)
{
	if ((SongFlags & SONG_FIRSTTICK) != 0)
	{
		if (chn->Period != 0 && param != 0)
		{
			if ((SongFlags & SONG_LINEARSLIDES) != 0)
				chn->Period = (chn->Period * FineLinearSlideDownTable[param & 0x0F] + 32768) / 65536;
			else
				chn->Period -= (int)param;
			if (chn->Period < 1)
				chn->Period = 1;
		}
	}
}

void ISoundFile::ExtraFinePortamentoDown(Channel *chn, UINT param)
{
	if ((SongFlags & SONG_FIRSTTICK) != 0)
	{
		if (chn->Period != 0 && param != 0)
		{
			if ((SongFlags & SONG_LINEARSLIDES) != 0)
				chn->Period = (chn->Period * FineLinearSlideUpTable[param & 0x0F] + 32768) / 65536;
			else
				chn->Period += param;
			if (chn->Period < 65535)
				chn->Period = 65535;
		}
	}
}

void ISoundFile::FinePortamentoUp(Channel *chn, UINT param)
{
	if ((SongFlags & SONG_FIRSTTICK) != 0)
	{
		if (chn->Period != 0 && param != 0)
		{
			if ((SongFlags & SONG_LINEARSLIDES) != 0)
				chn->Period = (chn->Period * LinearSlideDownTable[param & 0x0F] + 32768) / 65536;
			else
				chn->Period -= (int)(param * 4);
			if (chn->Period < 1)
				chn->Period = 1;
		}
	}
}

void ISoundFile::FinePortamentoDown(Channel *chn, UINT param)
{
	if ((SongFlags & SONG_FIRSTTICK) != 0)
	{
		if (chn->Period != 0 && param != 0)
		{
			if ((SongFlags & SONG_LINEARSLIDES) != 0)
				chn->Period = (chn->Period * LinearSlideUpTable[param & 0x0F] + 32768) / 65536;
			else
				chn->Period += (param * 4);
			if (chn->Period > 65535)
				chn->Period = 65535;
		}
	}
}

void ISoundFile::PortamentoUp(Channel *chn, UINT param)
{
	if (param != 0)
		chn->OldPortaUpDown = param;
	else
		param = chn->OldPortaUpDown;
	if ((param & 0xF0) >= 0xE0)
	{
		if ((param & 0x0F) != 0)
		{
			if ((param & 0xF0) == 0xF0)
				FinePortamentoUp(chn, param & 0x0F);
			else if ((param & 0xF0) == 0xE0)
				ExtraFinePortamentoUp(chn, param & 0x0F);
		}
		return;
	}
	if ((SongFlags & SONG_FIRSTTICK) == 0 || MusicSpeed == 1)
		DoFreqSlide(chn, -(int)(param * 4));
}

void ISoundFile::PortamentoDown(Channel *chn, UINT param)
{
	if (param != 0)
		chn->OldPortaUpDown = param;
	else
		param = chn->OldPortaUpDown;
	if ((param & 0xF0) >= 0xE0)
	{
		if ((param & 0x0F) != 0)
		{
			if ((param & 0xF0) == 0xF0)
				FinePortamentoDown(chn, param & 0x0F);
			else if ((param & 0xF0) == 0xE0)
				ExtraFinePortamentoDown(chn, param & 0x0F);
		}
		return;
	}
	if ((SongFlags & SONG_FIRSTTICK) == 0 || MusicSpeed == 1)
		DoFreqSlide(chn, param << 2);
}

void ISoundFile::TonePortamento(Channel *chn, UINT param)
{
	if (param != 0)
		chn->PortamentoSlide = param * 4;
	chn->Flags |= CHN_PORTAMENTO;
	if (chn->Period != 0 && chn->PortamentoDest != 0 && ((MusicSpeed == 1 || (SongFlags & SONG_FIRSTTICK) == 0)))
	{
		if (chn->Period < chn->PortamentoDest)
		{
			long delta = chn->PortamentoSlide;
			if ((SongFlags & SONG_LINEARSLIDES) != 0)
			{
				UINT n = chn->PortamentoSlide >> 2;
				if (n > 255)
					n = 255;
				delta = ((chn->Period * LinearSlideUpTable[n] + 32768) / 65536) - chn->Period;
				if (delta < 1)
					delta = 1;
			}
			chn->Period += delta;
			if (chn->Period > chn->PortamentoDest)
				chn->Period = chn->PortamentoDest;
		}
		else if (chn->Period > chn->PortamentoDest)
		{
			long delta = -chn->PortamentoSlide;
			if ((SongFlags & SONG_LINEARSLIDES) != 0)
			{
				UINT n = chn->PortamentoSlide >> 2;
				if (n > 255)
					n = 255;
				delta = ((chn->Period * LinearSlideDownTable[n] + 32768) / 65536) - chn->Period;
				if (delta > -1)
					delta = -1;
			}
			chn->Period += delta;
			if (chn->Period < chn->PortamentoDest)
				chn->Period = chn->PortamentoDest;
		}
	}
}

void ISoundFile::SampleOffset(UINT nChn, UINT param, bool Porta)
{
	Channel *chn = &Chns[nChn];
	Command *cmd = NULL;
	if ((int)Row < Patterns[Pattern].nRows - 1)
		cmd = Patterns[Pattern].p_Commands + ((Row + 1) * Channels) + nChn;
	if (cmd != NULL && cmd->command == CMD_XPARAM)
	{
		UINT tmp = cmd->param;
		cmd = NULL;
		if ((int)Row < Patterns[Pattern].nRows - 2)
			cmd = Patterns[Pattern].p_Commands + ((Row + 2) * Channels) + nChn;
		if (cmd != NULL && cmd->command == CMD_XPARAM)
			param = (param << 16) + (tmp << 8) + cmd->param;
		else
			param = (param << 8) + tmp;
	}
	else
	{
		if (param != 0)
			chn->OldOffset = param;
		else
			param = chn->OldOffset;
		param <<= 8;
		param |= chn->OldHiOffset << 16;
	}
	if (chn->RowNote != 0 && chn->RowNote < 0x80)
	{
		if (Porta == true)
			chn->Pos = param;
		else
			chn->Pos += param;
		if (chn->Pos >= chn->Length)
		{
			chn->Pos = chn->LoopStart;

			if ((SongFlags & SONG_ITOLDEFFECTS) != 0 && chn->Length > 4)
				chn->Pos = chn->Length - 2;
		}
	}
	return;
}

void ISoundFile::RetrigNote(UINT nChn, UINT param, UINT offset)
{
	Channel *chn = &Chns[nChn];
	UINT RetrigSpeed = param & 0x0F;
	UINT RetrigCount = chn->RetrigCount;
	bool DoRetrig = false;

	if (RetrigSpeed == 0)
		RetrigSpeed = 1;
	if (RetrigCount != 0 && (RetrigCount % RetrigSpeed) == 0)
		DoRetrig = true;
	RetrigCount++;
	if (DoRetrig == true)
	{
		UINT dv = (param >> 4) & 0x0F;
		UINT Note = chn->NewNote;
		UINT OldPeriod = chn->Period;
		bool ResetEnv = false;
		if (dv != 0)
		{
			int vol = chn->Volume;
			if (retrigTable1[dv] != 0)
				vol = (vol * retrigTable1[dv]) >> 4;
			else
				vol += retrigTable2[dv] << 2;
			if (vol < 0)
				vol = 0;
			if (vol > 256)
				vol = 256;
			chn->Volume = vol;
			chn->Flags |= CHN_FASTVOLRAMP;
		}
		if (Note != 0 && Note <= 120 && chn->Length != 0)
			CheckNNA(nChn, 0, Note, true);
		NoteChange(nChn, Note, false, ResetEnv);
		if (Instruments != 0)
			ProcessMidiOut(nChn, chn);
		if (chn->RowNote == 0 && OldPeriod != 0)
			chn->Period = OldPeriod;
		if (offset != 0)
		{
			if (chn->Instrument != NULL)
				chn->Length = chn->Instrument->length;
			SampleOffset(nChn, offset, false);
		}
	}
	chn->RetrigCount = RetrigCount;
}

void ISoundFile::NoteCut(UINT nChn, UINT Tick)
{
	if (TickCount == Tick)
	{
		Channel *chn = &Chns[nChn];
		chn->Volume = 0;
		chn->Flags |= CHN_FASTVOLRAMP;
	}
}

void ISoundFile::ExtendedChannelEffect(Channel *chn, UINT param)
{
	if (TickCount != 0)
		return;
	switch (param & 0x0F)
	{
		case 0x00:
		{
			chn->Flags &= ~CHN_SURROUND;
			break;
		}
		case 0x01:
		{
			chn->Flags |= CHN_SURROUND;
			chn->Pan = 128;
			break;
		}
		case 0x08:
		{
			chn->Flags &= ~CHN_REVERB;
			chn->Flags |= CHN_NOREVERB;
			break;
		}
		case 0x09:
		{
			chn->Flags &= ~CHN_NOREVERB;
			chn->Flags |= CHN_REVERB;
			break;
		}
		case 0x0A:
		{
			SongFlags &= ~SONG_SURROUNDPAN;
			break;
		}
		case 0x0B:
		{
			SongFlags |= SONG_SURROUNDPAN;
			break;
		}
		case 0x0C:
		{
			SongFlags &= ~SONG_MPTFILTERMODE;
			break;
		}
		case 0x0D:
		{
			SongFlags |= SONG_MPTFILTERMODE;
			break;
		}
		case 0x0E:
		{
			SongFlags &= ~CHN_PINGPONGFLAG;
			break;
		}
		case 0x0F:
		{
			if ((chn->Flags & CHN_LOOP) == 0 && chn->Pos == 0 && chn->Length != 0)
			{
				chn->Pos = chn->Length - 1;
				chn->PosLo = 65535;
			}
			chn->Flags |= CHN_PINGPONGFLAG;
			break;
		}
	}
}

void ISoundFile::ExtendedS3MCommands(UINT nChn, UINT param)
{
	Channel *chn = &Chns[nChn];
	UINT command = param & 0xF0;
	param &= 0x0F;
	switch (command)
	{
		case 0x10:
		{
			chn->Flags &= ~CHN_GLISSANDO;
			if (param != 0)
				chn->Flags |= CHN_GLISSANDO;
			break;
		}
		case 0x20:
		{
			if (TickCount != 0)
				break;
			chn->C4Speed = FineTuneTable[param & 0x0F];
			chn->FineTune = (int)((UCHAR)(param << 4));
			if  (chn->Period != 0)
				chn->Period = GetPeriodFromNote(chn->Note, chn->FineTune, chn->C4Speed);
			break;
		}
		case 0x30:
		{
			chn->VibratoType = param & 0x07;
			break;
		}
		case 0x40:
		{
			chn->TremoloType = param & 0x07;
			break;
		}
		case 0x50:
		{
			chn->PanbrelloType = param & 0x07;
			break;
		}
		case 0x60:
		{
			FrameDelay = param;
			break;
		}
		case 0x70:
		{
			if (TickCount != 0)
				break;
			switch (param)
			{
				case 0:
				case 1:
				case 2:
				{
					Channel *bck = &Chns[Channels];
					for (UINT i = Channels; i < MAX_CHANNELS; i++, bck++)
					{
						if (bck->MasterChn == nChn + 1)
						{
							if (param == 1)
								KeyOff(i);
							else if (param == 2)
								bck->Flags |= CHN_NOTEFADE;
							else
							{
								bck->Flags |= CHN_NOTEFADE;
								bck->FadeOutVol = 0;
							}
						}
					}
					break;
				}
				case 3:
				{
					chn->NNA = NNA_NOTECUT;
					break;
				}
				case 4:
				{
					chn->NNA = NNA_CONTINUE;
					break;
				}
				case 5:
				{
					chn->NNA = NNA_NOTEOFF;
					break;
				}
				case 6:
				{
					chn->NNA = NNA_NOTEFADE;
					break;
				}
				case 7:
				{
					chn->Flags &= ~CHN_VOLENV;
					break;
				}
				case 8:
				{
					chn->Flags |= CHN_VOLENV;
					break;
				}
				case 9:
				{
					chn->Flags &= ~CHN_PANENV;
					break;
				}
				case 10:
				{
					chn->Flags |= CHN_PANENV;
					break;
				}
				case 11:
				{
					chn->Flags &= ~CHN_PITCHENV;
					break;
				}
				case 12:
				{
					chn->Flags |= CHN_PITCHENV;
					break;
				}
			}
			break;
		}
		case 0x80:
		{
			if (TickCount == 0)
			{
				chn->Pan = (param << 4) + 8;
				chn->Flags |= CHN_FASTVOLRAMP;
			}
			break;
		}
		case 0x90:
		{
			ExtendedChannelEffect(chn, param & 0x0F);
			break;
		}
		case 0xA0:
		{
			if (TickCount == 0)
			{
				chn->OldHiOffset = param;
				if (chn->RowNote != 0 && chn->RowNote < 0x80)
				{
					DWORD pos = param << 16;
					if (pos < chn->Length)
						chn->Pos = pos;
				}
			}
			break;
		}
		case 0xC0:
		{
			NoteCut(nChn, param);
			break;
		}
		case 0xF0:
		{
			chn->ActiveMacro = param;
			break;
		}
	}
}

void ISoundFile::ProcessSmoothMidiMacro(UINT nChn, char *MidiMacro, UINT param)
{
	Channel *chn = &Chns[nChn];
	DWORD Macro = (*((DWORD *)MidiMacro)) & 0x7F5F7F5F;
	int InternalCode = -256;
	char Data;
	DWORD Param;

	if (Macro != 0x30463046)
		return;

	MidiMacro += 4;
	if (MidiMacro[0] >= '0' && MidiMacro[0] <= '9')
		InternalCode = (MidiMacro[0] - '0') << 4;
	else if (MidiMacro[0] >= 'A' && MidiMacro[0] <= 'F')
		InternalCode = (MidiMacro[0] - 'A' + 0x0A) << 4;
	if (MidiMacro[1] >= '0' && MidiMacro[1] <= '9')
		InternalCode += MidiMacro[1] - '0';
	else if (MidiMacro[1] >= 'A' && MidiMacro[1] <= 'F')
		InternalCode += MidiMacro[1] - 'A' + 0x0A;
	if (InternalCode < 0)
		return;
	Data = MidiMacro[2];
	Param = 0;

	if (toupper(Data) == 'Z')
		Param = param;
	else
	{
		char Data2 = MidiMacro[3];
		if (Data >= '0' && Data <= '9')
			Param += (Data - '0') << 4;
		else if (Data >= 'A' && Data <= 'F')
			Param += (Data - 'A' + 0x0A) << 4;
		if (Data2 >= '0' && Data2 <= '9')
			Param += Data2 - '0';
		else if (Data2 >= 'A' && Data2 <= 'F')
			Param += Data2 - 'A' + 0x0A;
	}

	switch (InternalCode)
	{
		case 0x00:
		{
			int OldCutOff = chn->CutOff;
			if (Param < 0x80)
			{
				if ((SongFlags & SONG_FIRSTTICK) != 0)
				{
					chn->PlugInitialParamValue = chn->CutOff;
					chn->PlugParamValueStep = (float)((int)Param - chn->PlugInitialParamValue) / (float)MusicSpeed;
				}
				chn->CutOff = (BYTE)(chn->PlugInitialParamValue + (TickCount + 1) * chn->PlugParamValueStep + 0.5);
			}
			OldCutOff -= chn->CutOff;
			if (OldCutOff < 0)
				OldCutOff = -OldCutOff;
			if (chn->Volume > 0 || OldCutOff < 0x10 || (chn->Flags & CHN_FILTER) == 0 || (chn->LeftVol | chn->RightVol) == 0)
				SetupChannelFilter(chn, ((chn->Flags & CHN_FILTER) != 0 ? false : true));
			break;
		}
		case 0x01:
		{
			if (Param < 0x80)
			{
				if ((SongFlags & SONG_FIRSTTICK) != 0)
				{
					chn->PlugInitialParamValue = chn->Resonance;
					chn->PlugParamValueStep = (float)((int)Param - chn->PlugInitialParamValue) / (float)MusicSpeed;
				}
				chn->Resonance = (BYTE)(chn->PlugInitialParamValue + (TickCount + 1) * chn->PlugParamValueStep + 0.5);
				SetupChannelFilter(chn, ((chn->Flags & CHN_FILTER) != 0 ? false : true));
			}
			break;
		}
		case 0x02:
		{
			if (Param < 0x20)
			{
				chn->FilterMode = (BYTE)(Param >> 4);
				SetupChannelFilter(chn, ((chn->Flags & CHN_FILTER) != 0 ? false : true));
			}
			break;
		}
	}
}

void ISoundFile::ProcessMidiMacro(UINT nChn, char *MidiMacro, UINT param)
{
	Channel *chn = &Chns[nChn];
	DWORD Macro = (*((DWORD *)MidiMacro)) & 0x7F5F7F5F;
	int InternalCode;

	if (Macro != 0x30463046)
	{
		UINT pos = 0, Nib = 0, Bytes = 0;
		DWORD MidiCode = 0, ByteCode = 0;
		while (pos + 6 <= 32)
		{
			char Data = toupper(MidiMacro[pos]);
			pos++;
			if (Data == 0)
				break;
			if (Data >= '0' && Data <= '9')
			{
				ByteCode = (ByteCode << 4) | (Data - '0');
				Nib++;
			}
			else if (Data >= 'A' && Data <= 'F')
			{
				ByteCode = (ByteCode << 4) |(Data - 'A' + 10);
				Nib++;
			}
			else if (Data == 'Z')
			{
				ByteCode = param & 0x7F;
				Nib = 2;
			}
			else if (Data == 'X')
			{
				ByteCode = param & 0x70;
				Nib = 2;
			}
			else if (Data == 'Y')
			{
				ByteCode = (param & 0x0F) << 3;
				Nib = 2;
			}

			if (Nib >= 2)
			{
				Nib = 0;
				MidiCode |= ByteCode << (Bytes * 8);
				ByteCode = 0;
				Bytes++;

				if (Bytes >= 3)
				{
					Bytes = 0;
					MidiCode = 0;
				}
			}
		}
		return;
	}
	MidiMacro += 5;
	InternalCode = -256;
	if (MidiMacro[0] >= '0' && MidiMacro[0] <= '9')
		InternalCode = (MidiMacro[0] - '0') << 4;
	else if (MidiMacro[0] >= 'A' && MidiMacro[0] <= 'F')
		InternalCode = (MidiMacro[0] - 'A' + 0x0A) << 4;
	if (MidiMacro[1] >= '0' && MidiMacro[1] <= '9')
		InternalCode = MidiMacro[10] - '0';
	else if (MidiMacro[1] >= 'A' && MidiMacro[1] <= 'F')
		InternalCode = MidiMacro[1] - 'A' + 0x0A;
	if (InternalCode >= 0)
	{
		char Data = toupper(MidiMacro[2]);
		DWORD Param = 0;
		if (Data == 'Z')
			Param = param;
		else
		{
			char Data2 = MidiMacro[3];
			if (Data >= '0' && Data <= '9')
				Param += (Data - '0') << 4;
			else if (Data >= 'A' && Data <= 'F')
				Param += (Data - 'A' + 0x0A) << 4;
			if (Data2 >= '0' && Data2 <= '9')
				Param += Data2 - '0';
			else if (Data2 >= 'A' && Data2 <= 'F')
				Param += Data2 - 'A' + 0x0A;
		}

		switch (InternalCode)
		{
			case 0x00:
			{
				int OldCutOff = chn->CutOff;
				if (Param < 0x80)
					chn->CutOff = (BYTE)Param;
				OldCutOff -= chn->CutOff;
				if (OldCutOff < 0)
					OldCutOff = -OldCutOff;
				if (chn->Volume > 0 || OldCutOff < 0x10 || (chn->Flags & CHN_FILTER) == 0 || (chn->LeftVol | chn->RightVol) == 0)
					SetupChannelFilter(chn, ((chn->Flags & CHN_FILTER) != 0 ? false : true));
				break;
			}
			case 0x01:
			{
				if (Param < 0x80)
					chn->Resonance = (BYTE)Param;
				SetupChannelFilter(chn, ((chn->Flags & CHN_FILTER) != 0 ? false : true));
				break;
			}
			case 0x02:
			{
				chn->FilterMode = (BYTE)(Param >> 4);
				SetupChannelFilter(chn, ((chn->Flags & CHN_FILTER) != 0 ? false : true));
				break;
			}
		}
	}
}

bool ISoundFile::ProcessEffects()
{
	Channel *chn = Chns;
	Command *Cmmd;
	int BreakRow = -1, PosJump = -1, PatLoopRow = -1;

	for (DWORD i = 0; i < Channels; i++, chn++)
	{
		BYTE instr = chn->RowInstr;
		BYTE volcmd = chn->RowVolCmd;
		BYTE vol = chn->RowVolume;
		BYTE cmd = chn->RowCommand;
		UINT param = chn->RowParam;
		bool Porta = (cmd != CMD_TONEPORTAMENTO && cmd != CMD_TONEPORTAVOL && volcmd != VOLCMD_TONEPORTAMENTO ? false : true);
		UINT StartTick = 0;

		chn->Flags &= ~CHN_FASTVOLRAMP;
		// Special Effects?
		if (cmd == CMD_MODCMDEX || cmd == CMD_S3MCMDEX)
		{
			if (param == 0)
				param = chn->OldCmdEx;
			else
				chn->OldCmdEx = param;
			// Note Delay?
			if ((param & 0xF0) == 0xD0)
				StartTick = param & 0x0F;
			else if (TickCount == 0)
			{
				// Pattern Loop?
				if (((param & 0xF0) == 0x60 && cmd == CMD_MODCMDEX) || ((param & 0xF0) == 0xB0 && cmd == CMD_S3MCMDEX))
				{
					int loop = PatternLoop(chn, param & 0x0F);
					if (loop >= 0)
						PatLoopRow = loop;
				}
				// Pattern Delay?
				else if ((param & 0xF0) == 0xE0)
					PatternDelay = param & 0x0F;
			}
		}

		// Note/Instrument/Volume changed?
		if (TickCount == StartTick) // (which can be delayed by a note delay effect)
		{
			BYTE note = chn->RowNote;
			if (instr != 0)
				chn->NewInst = instr;
			if (note == 0 && instr != 0)
			{
				if (Instruments != 0)
				{
					if (chn->Instrument != NULL)
						chn->Volume = chn->Instrument->vol << 2;
				}
				else if (instr < MAX_SAMPLES)
					chn->Volume = Ins[instr - 1].vol << 2;
			}
			if (instr >= MAX_INSTRUMENTS) instr = 0;
			if (note >= 0xFE) instr = 0;
			if (note != 0 && note <= 128)
				chn->NewNote = note;
			if (note != 0 && note < 128 && Porta == false)
				CheckNNA(Channels, instr, note, false);
			if (instr != 0)
			{
				ITSampleStruct *smp = chn->Instrument;
				InstrumentChange(chn, instr, Porta, true);
				chn->NewInst = 0;
				if (smp != chn->Instrument && note != 0 && note < 0x80)
					Porta = false;
			}
			if (note != 0)
			{
				if (instr == 0 && chn->NewInst != 0 && note < 0x80)
				{
					InstrumentChange(chn, chn->NewInst, Porta, false, true);
					chn->NewInst = 0;
				}
				NoteChange(i, note, Porta, true);
			}
			if (volcmd == VOLCMD_VOLUME)
			{
				if (vol > 64) vol = 64;
				chn->Volume = vol << 2;
				chn->Flags |= CHN_FASTVOLRAMP;
			}
			else if (volcmd == VOLCMD_PANNING)
			{
				if (vol > 64) vol = 64;
				chn->Pan = vol << 2;
				chn->Flags |= CHN_FASTVOLRAMP;
			}
			if (Instruments != 0)
				ProcessMidiOut(i, chn);
		}
		if (volcmd > VOLCMD_PANNING && TickCount >= StartTick)
		{
			if (volcmd == VOLCMD_TONEPORTAMENTO)
				TonePortamento(chn, PortaVolCmd[vol & 0x0F]);
			else
			{
				if (vol != 0)
					chn->OldVolParam = vol;
				else
					vol = chn->OldVolParam;
				switch (volcmd)
				{
					case VOLCMD_VOLSLIDEUP:
					{
						VolumeSlide(chn, vol << 4);
						break;
					}
					case VOLCMD_VOLSLIDEDOWN:
					{
						VolumeSlide(chn, vol);
						break;
					}
					case VOLCMD_FINEVOLUP:
					{
						if (TickCount == StartTick)
							VolumeSlide(chn, (vol << 4) | 0x0F);
						break;
					}
					case VOLCMD_FINEVOLDOWN:
					{
						if (TickCount == StartTick)
							VolumeSlide(chn, vol | 0xF0);
						break;
					}
					case VOLCMD_VIBRATOSPEED:
					{
						Vibrato(chn, vol << 4);
						break;
					}
					case VOLCMD_VIBRATO:
					{
						Vibrato(chn, vol);
						break;
					}
					case VOLCMD_PANSLIDELEFT:
					{
						PanningSlide(chn, vol);
						break;
					}
					case VOLCMD_PANSLIDERIGHT:
					{
						PanningSlide(chn, vol << 4);
						break;
					}
					case VOLCMD_PORTAUP:
					{
						PortamentoUp(chn, vol << 2);
						break;
					}
					case VOLCMD_PORTADOWN:
					{
						PortamentoDown(chn, vol << 2);
						break;
					}
					case VOLCMD_VELOCITY:
					{
						chn->Volume = vol * 28;
						chn->Flags |= CHN_FASTVOLRAMP;
						if  (TickCount == StartTick)
							SampleOffset(i, 48 - (vol << 3), Porta);
						break;
					}
					case VOLCMD_OFFSET:
					{
						if (TickCount == StartTick)
							SampleOffset(i, vol << 3, Porta);
						break;
					}
				}
			}
		}
		if (cmd != 0)
		{
			switch (cmd)
			{
				case CMD_XPARAM:
					break;
				case CMD_VOLUME:
				{
					if (TickCount == 0)
					{
						chn->Volume = (param < 64 ? param * 4 : 256);
						chn->Flags |= CHN_FASTVOLRAMP;
					}
					break;
				}
				case CMD_PORTAMENTOUP:
				{
					PortamentoUp(chn, param);
					break;
				}
				case CMD_PORTAMENTODOWN:
				{
					PortamentoDown(chn, param);
					break;
				}
				case CMD_VOLUMESLIDE:
				{
					if (param != 0)
						VolumeSlide(chn, param);
					break;
				}
				case CMD_TONEPORTAMENTO:
				{
					TonePortamento(chn, param);
					break;
				}
				case CMD_TONEPORTAVOL:
				{
					if (param != 0)
						VolumeSlide(chn, param);
					TonePortamento(chn, 0);
				}
				case CMD_VIBRATO:
				{
					Vibrato(chn, param);
					break;
				}
				case CMD_VIBRATOVOL:
				{
					if (param != 0)
						VolumeSlide(chn, param);
					Vibrato(chn, 0);
					break;
				}
				case CMD_SPEED:
				{
					if (TickCount == 0)
						SetSpeed(param);
					break;
				}
				case CMD_TEMPO:
				{
					Cmmd = NULL;
					if ((USHORT)Row < Patterns[Pattern].nRows - 1)
						Cmmd = Patterns[Pattern].p_Commands + ((Row + 1) * Channels) + i;
					if (Cmmd != NULL && Cmmd->command == CMD_XPARAM)
						param = (param << 8) + Cmmd->param;
					if (param != 0)
						chn->OldTempo = param;
					else
						param = chn->OldTempo;
					if (param > 512)
						param = 512;
					SetTempo(param);
					break;
				}
				case CMD_OFFSET:
				{
					if (TickCount != 0)
						break;
					SampleOffset(i, param, Porta);
					break;
				}
				case CMD_ARPEGGIO:
				{
					if (TickCount != 0 || chn->Period == 0 || chn->Note == 0)
						break;
					if (param == 0)
						break;
					chn->Command = cmd;
					chn->Arpeggio = param;
					break;
				}
				case CMD_RETRIG:
				{
					if (param != 0)
						chn->RetrigParam = param;
					else
						param = chn->RetrigParam;
					if (volcmd == VOLCMD_OFFSET)
						RetrigNote(i, param, vol << 3);
					else if (volcmd == VOLCMD_VELOCITY)
						RetrigNote(i, param, 48 - (vol << 3));
					else
						RetrigNote(i, param);
					break;
				}
				case CMD_TREMOR:
				{
					if (TickCount != 0)
						break;
					chn->Command = cmd;
					if (param != 0)
						chn->TremorParam = param;
					break;
				}
				case CMD_GLOBALVOLUME:
				{
					if (TickCount != 0)
						break;
					if (param > 128)
						param = 128;
					GlobalVolume = param << 1;
					break;
				}
				case CMD_GLOBALVOLSLIDE:
				{
					GlobalVolSlide(param);
					break;
				}
				case CMD_PANNING8:
				{
					if (TickCount != 0)
						break;
					if ((SongFlags & SONG_SURROUNDPAN) == 0)
						chn->Flags &= ~CHN_SURROUND;
					chn->Pan = param;
					chn->Flags |= CHN_FASTVOLRAMP;
					break;
				}
				case CMD_PANNINGSLIDE:
				{
					PanningSlide(chn, param);
					break;
				}
				case CMD_TREMOLO:
				{
					Tremolo(chn, param);
					break;
				}
				case CMD_FINEVIBRATO:
				{
					FineVibrato(chn, param);
					break;
				}
				case CMD_S3MCMDEX:
				{
					ExtendedS3MCommands(i, param);
					break;
				}
				case CMD_KEYOFF:
				{
					if (TickCount == 0)
						KeyOff(i);
					break;
				}
				case CMD_XFINEPORTAUPDOWN:
				{
					if ((param & 0xF0) == 0x10)
						ExtraFinePortamentoUp(chn, param & 0x0F);
					else if ((param & 0xF0) == 0x20)
						ExtraFinePortamentoDown(chn, param & 0x0F);
					break;
				}
				case CMD_CHANNELVOLUME:
				{
					if (TickCount != 0)
						break;
					if (param <= 64)
					{
						chn->GlobalVol = param;
						chn->Flags |= CHN_FASTVOLRAMP;
					}
					break;
				}
				case CMD_CHANNELVOLSLIDE:
				{
					ChannelVolSlide(chn, param);
					break;
				}
				case CMD_PANBRELLO:
				{
					Panbrello(chn, param);
					break;
				}
				case CMD_SETENVPOSITION:
				{
					if (TickCount != 0)
						break;
					chn->VolEnvPosition = param;
					chn->PanEnvPosition = param;
					chn->PitchEnvPosition = param;
					if (chn->Header != NULL)
					{
						ITInstrument *env = chn->Header;
						int PanEnv = env->panenv.num;
						if ((chn->Flags & CHN_PANENV) != 0 && PanEnv != 0 && param > (UINT)((env->panenv.data[(PanEnv - 1) * 3 + 2] << 8) | (env->panenv.data[(PanEnv - 1) * 3 + 1])))
							chn->Flags &= ~CHN_PANENV;
					}
					break;
				}
				case CMD_POSITIONJUMP:
				{
					PosJump = param;
					break;
				}
				case CMD_PATTERNBREAK:
				{
					Cmmd = NULL;
					if ((USHORT)Row < Patterns[Pattern].nRows - 1)
						Cmmd = Patterns[Pattern].p_Commands + ((Row + 1) * Channels) + i;
					if (Cmmd != NULL && Cmmd->command == CMD_XPARAM)
						BreakRow = (param << 8) + Cmmd->param;
					else
						BreakRow = param;
					break;
				}
				case CMD_MIDI:
				{
					if (TickCount != 0)
						break;
					if (param < 0x80)
						ProcessMidiMacro(i, &MidiCfg.MidiSFXExt[chn->ActiveMacro << 5], param);
					else
						ProcessMidiMacro(i, &MidiCfg.MidiZXXExt[(param & 0x7F) << 5], 0);
					break;
				}
				case CMD_SMOOTHMIDI:
				{
					if (param < 0x80)
						ProcessSmoothMidiMacro(i, &MidiCfg.MidiSFXExt[chn->ActiveMacro << 5], param);
					else
						ProcessSmoothMidiMacro(i, &MidiCfg.MidiZXXExt[(param & 0x7F) << 5], 0);
					break;
				}
				case CMD_VELOCITY:
					break;
			}
		}
	}
	if (TickCount == 0)
	{
		if (PatLoopRow >= 0)
		{
			NextPattern = CurrentPattern;
			NextRow = PatLoopRow;
			if (PatternDelay != 0)
				NextRow++;
		}
		else if (BreakRow >= 0 || PosJump >= 0)
		{
			bool NoLoop = false;
			if (PosJump < 0)
				PosJump = CurrentPattern + 1;
			if (BreakRow < 0)
				BreakRow = 0;
			if (PosJump < (int)CurrentPattern || (PosJump == CurrentPattern && BreakRow <= (int)Row))
			{
				if (RepeatCount != 0)
				{
					if (RepeatCount > 0)
						RepeatCount--;
				}
				else if ((SoundSetup & SNDMIX_NOBACKWARDJUMPS) != 0)
					NoLoop = true;
			}
			if (PosJump > MAX_ORDERS)
				PosJump = 0;
			if (NoLoop == false && (PosJump != CurrentPattern || BreakRow != Row))
			{
				if (PosJump != CurrentPattern)
				{
					for (UINT i = 0; i < Channels; i++)
						Chns[i].PaternLoopCount = 0;
				}
				NextPattern = PosJump;
				NextRow = BreakRow;
				PatternTransitionOccurred = true;
			}
		}
	}
	return true;
}

bool ISoundFile::ProcessRow()
{
	TickCount++;
	if (TickCount >= MusicSpeed * (PatternDelay + 1) + FrameDelay)
	{
		Command *cmd;
		Channel *chn = Chns;
		HandlePatternTransitionEvents();
		PatternDelay = 0;
		FrameDelay = 0;
		TickCount = 0;
		Row = NextRow;
		if (CurrentPattern != NextPattern)
			CurrentPattern = NextPattern;
		if ((SongFlags & SONG_PATTERNLOOP) == 0)
		{
			Pattern = (CurrentPattern < MAX_ORDERS ? Order[CurrentPattern] : 0xFF);
			if (Pattern < p_IF->p_Head->patnum && Patterns[Pattern].p_Commands == NULL)
				Pattern = 0xFE;
			while (Pattern >= MAX_PATTERNS)
			{
				if (Pattern == 0xFF || CurrentPattern >= MAX_ORDERS)
				{
					if (RepeatCount == 0)
						return false;
					if (RestartPos == 0)
					{
						MusicSpeed = p_IF->p_Head->speed;
						MusicTempo = p_IF->p_Head->tempo;
						GlobalVolume = p_IF->p_Head->globalvol;
						for (UINT i = 0; i < MAX_CHANNELS; i++)
						{
							Chns[i].Flags |= (CHN_NOTEFADE | CHN_KEYOFF);
							Chns[i].FadeOutVol = 0;
							if (i < Channels)
							{
								Chns[i].GlobalVol = ChnSettings[i].Volume;
								Chns[i].Volume = ChnSettings[i].Volume;
								Chns[i].Pan = ChnSettings[i].Pan;
								Chns[i].PanSwing = Chns[i].VolSwing = 0;
								Chns[i].CutSwing = Chns[i].ResSwing = 0;
								Chns[i].OldVolParam = Chns[i].OldHiOffset = 0;
								Chns[i].OldOffset = 0;
								Chns[i].PortamentoDest = 0;
								if (Chns[i].Length != 0)
								{
									Chns[i].Flags = ChnSettings[i].Flags;
									Chns[i].LoopStart = 0;
									Chns[i].LoopEnd = 0;
									Chns[i].Header = NULL;
									Chns[i].Sample = NULL;
									Chns[i].Instrument = NULL;
								}
							}
						}
					}
					if (RepeatCount > 0)
						RepeatCount--;
					CurrentPattern = RestartPos;
					Row = 0;
					if (Order[CurrentPattern] >= p_IF->p_Head->patnum || Patterns[Order[CurrentPattern]].p_Commands == NULL)
						return false;
				}
				else
					CurrentPattern++;
				Pattern = (CurrentPattern < MAX_ORDERS ? Order[CurrentPattern] : 0xFF);
				if (Pattern < p_IF->p_Head->patnum && Patterns[Pattern].p_Commands == NULL)
					Pattern = 0xFE;
			}
			NextPattern = CurrentPattern;
			if (MaxOrderPosition != 0 && CurrentPattern >= MaxOrderPosition)
				return false;
		}

		if (Pattern >= p_IF->p_Head->patnum || Patterns[Pattern].p_Commands == NULL)
			return false;
		if (Row >= Patterns[Pattern].nRows)
			Row = 0;
		NextRow = Row + 1;
		if (NextRow >= Patterns[Pattern].nRows)
		{
			if ((SongFlags & SONG_PATTERNLOOP) == 0)
				NextPattern = CurrentPattern + 1;
			NextRow = 0;
			PatternTransitionOccurred = true;
		}
		cmd = Patterns[Pattern].p_Commands + (Row * Channels);
		for (DWORD i = 0; i < Channels; chn++, i++, cmd++)
		{
			chn->RowNote = cmd->note;
			chn->RowInstr = cmd->instr;
			chn->RowVolCmd = cmd->volcmd;
			chn->RowVolume = cmd->vol;
			chn->RowCommand = cmd->command;
			chn->RowParam = cmd->param;

			chn->LeftVol = chn->NewLeftVol;
			chn->RightVol = chn->NewRightVol;
			chn->Flags &= ~(CHN_PORTAMENTO | CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO);
			chn->Command = 0;
		}
	}

	if (MusicSpeed == 0)
		MusicSpeed = 1;
	SongFlags |= SONG_FIRSTTICK;
	if (TickCount != 0)
	{
		SongFlags &= ~SONG_FIRSTTICK;
		if (TickCount < MusicSpeed * (PatternDelay + 1))
		{
			if ((TickCount % MusicSpeed) != 0)
				SongFlags |= SONG_FIRSTTICK;
		}
	}
	return ProcessEffects();
}

bool ISoundFile::ReadNote()
{
	Channel *chn;
	DWORD MasterVol;
	if ((SongFlags & SONG_PAUSED) != 0)
	{
		TickCount = 0;
		if (MusicSpeed == 0)
			MusicSpeed = 6;
		if (MusicTempo == 0)
			MusicTempo = 125;
	}
	else if (ProcessRow() == false)
		return false;

	TotalCount++;
	if (MusicTempo == 0)
		return false;

	switch (TempoMode)
	{
		case tempo_mode_alternative:
		{
			BufferCount = 44100 / MusicTempo;
			break;
		}
		case tempo_mode_modern:
		{
			double accurateBufferCount = 44100.0  * (60.0 / (double)MusicTempo / ((double)MusicSpeed * (double)RowsPerBeat));
			BufferCount = (UINT)accurateBufferCount;
			BufferDiff += accurateBufferCount - BufferCount;
			if (BufferDiff >= 1)
			{
				BufferCount++;
				BufferDiff--;
			}
			else if (BufferDiff <= -1)
			{
				BufferCount--;
				BufferDiff++;
			}
			assert(abs(BufferDiff) < 1);
			break;
		}
		case tempo_mode_classic:
		default:
		{
			/*44100 * 5 * 128 = 28224000*/
			BufferCount = (28224000) / (MusicTempo << 8);
			break;
		}
	}
	SamplesPerTick = BufferCount;
	if ((SongFlags & SONG_PAUSED) != 0)
		BufferCount = 44100 / 64;

	{
		UINT Attenuation;
		int Chn32 = Channels - 1;
		chn = Chns;
		int RealMasterVol = MasterVolume;
		if (Chn32 < 1)
			Chn32 = 1;
		if (Chn32 > 31)
			Chn32 = 32;
		if (RealMasterVol > 0x80)
			RealMasterVol = 0x80 + ((RealMasterVol - 0x80) * (Chn32 + 4)) / 16;
		MasterVol = (RealMasterVol * SongPreAmp) >> 6;
		if ((SongFlags & SONG_GLOBALFADE) != 0 && GlobalFadeMaxSamples != 0)
			MasterVol = muldiv(MasterVol, GlobalFadeSamples, GlobalFadeMaxSamples);
		Attenuation = ((SoundSetup & SNDMIX_AGC) ? PreAmpAGCTable[Chn32 >> 1] : PreAmpTable[Chn32 >> 1]);
		if (Attenuation < 1)
			Attenuation = 1;
		MasterVol = (MasterVol << 7) / Attenuation;
	}
	MixChannels = 0;
	chn = Chns;
	for (UINT i = 0; i < MAX_CHANNELS; i++, chn++)
	{
skipchn:
		if ((chn->Flags & CHN_NOTEFADE) != 0 && (chn->FadeOutVol | chn->RightVol | chn->LeftVol) == 0)
		{
			chn->Length = 0;
			chn->ROfs = chn->LOfs = 0;
		}
		if ((chn->Flags & CHN_MUTE) != 0 || (i >= Channels && chn->Length == 0))
		{
			i++;
			chn++;
			if (i >= Channels)
			{
				while (i < MAX_CHANNELS && chn->Length != 0)
				{
					i++;
					chn++;
				}
			}
			if (i < MAX_CHANNELS)
				goto skipchn;
			goto done;
		}
		chn->Inc = 0;
		chn->RealVolume = 0;
		chn->RealPan = chn->Pan + chn->PanSwing;
		if (chn->RealPan < 0)
			chn->RealPan = 0;
		if (chn->RealPan > 256)
			chn->RealPan = 256;
		if (chn->Period != 0 && chn->Length != 0)
		{
			UINT inc, freq;
			int vol = chn->Volume + chn->VolSwing, period, PeriodFrac;
			if (vol < 0)
				vol = 0;
			if (vol > 256)
				vol = 256;
			if ((chn->Flags & CHN_TREMOLO) != 0)
			{
				UINT trempos = chn->TremoloPos & 0x3F;
				if (vol > 0)
				{
					switch (chn->TremoloType & 0x03)
					{
						case 1:
						{
							vol += (RampDownTable[trempos] * chn->TremoloDepth) >> 5;
							break;
						}
						case 2:
						{
							vol += (SquareTable[trempos] * chn->TremoloDepth) >> 5;
							break;
						}
						case 3:
						{
							vol += (RandomTable[trempos] * chn->TremoloDepth) >> 5;
							break;
						}
						default:
							vol += (SinusTable[trempos] * chn->TremoloDepth) >> 5;
					}
				}
				if (TickCount != 0 || (SongFlags & SONG_ITOLDEFFECTS) == 0)
					chn->TremoloPos = (trempos + chn->TremoloSpeed) & 0x3F;
			}
			if (chn->Command == CMD_TREMOR)
			{
				UINT n = (chn->TremorParam >> 4) + (chn->TremorParam & 0x0F);
				UINT ontime = chn->TremorParam >> 4;
				UINT tremcount = chn->TremorCount;
				if ((SongFlags & SONG_ITOLDEFFECTS) != 0)
				{
					n += 2;
					ontime++;
				}
				if (tremcount >= n)
					tremcount = 0;
				if (tremcount >= ontime)
					vol = 0;
				chn->TremorCount = tremcount + 1;
				chn->Flags |= CHN_FASTVOLRAMP;
			}
			if (vol < 0)
				vol = 0;
			if (vol > 256)
				vol = 256;
			vol <<= 6;
			if (chn->Header != NULL)
			{
				ITInstrument *env = chn->Header;
				if ((chn->Flags & CHN_VOLENV) != 0 && env->volenv.num != 0)
				{
					int envpos = chn->VolEnvPosition, x2, x1, envvol;
					UINT pt = env->volenv.num - 1;
					for (int j = 0; j < env->volenv.num - 1; j++)
					{
						if (envpos <= ((env->volenv.data[j * 3 + 2] << 8) | env->volenv.data[j * 3 + 1]))
						{
							pt = j;
							break;
						}
					}
					x2 = ((env->volenv.data[pt * 3 + 2] << 8) | env->volenv.data[pt * 3 + 1]);
					if (envpos >= x2)
					{
						envvol = env->volenv.data[pt * 3] << 2;
						x1 = x2;
					}
					else if (pt != 0)
					{
						envvol = env->volenv.data[(pt - 1) * 3] << 2;
						x1 = ((env->volenv.data[(pt - 1) * 3 + 2] << 8) | env->volenv.data[(pt - 1) * 3 + 1]);
					}
					else
						envvol = x1 = 0;
					if (envpos > x2)
						envpos = x2;
					if (x2 > x1 && envpos > x1)
						envvol += ((envpos - x1) * ((env->volenv.data[pt * 3] << 2) - envvol)) / (x2 - x1);
					if (envvol < 0)
						envvol = 0;
					if (envvol > 256)
						envvol = 256;
					vol = (vol * envvol) >> 8;
				}
				if ((chn->Flags & CHN_PANENV) != 0 && env->panenv.num != 0)
				{
					int envpos = chn->PanEnvPosition, x1, x2, envpan, y2;
					UINT pt = env->panenv.num - 1;
					int pan;
					for (int j = 0; j < env->panenv.num - 1; j++)
					{
						if (envpos <= ((env->panenv.data[j * 3 + 2] << 8) | (env->volenv.data[j * 3 + 1])))
						{
							pt = j;
							break;
						}
					}
					x2 = ((env->panenv.data[pt * 3 + 2] << 8) | (env->volenv.data[pt * 3 + 1]));
					y2 = env->panenv.data[pt * 3];
					if (envpos >= x2)
					{
						envpan = y2;
						x1 = x2;
					}
					else if (pt != 0)
					{
						envpan = env->panenv.data[(pt - 1) * 3];
						x1 = ((env->panenv.data[(pt - 1) * 3 + 2] << 8) | (env->volenv.data[(pt - 1) * 3 + 1]));
					}
					else
					{
						envpan = 128;
						x1 = 0;
					}
					if (x2 > x1 && envpos > x1)
						envpan += ((envpos - x1) * (y2 - envpan)) / (x2 - x1);
					if (envpan < 0)
						envpan = 0;
					if (envpan > 64)
						envpan = 64;
					pan = chn->Pan;
					if (pan >= 128)
						pan += ((envpan - 32) * (256 - pan)) / 32;
					else
						pan += ((envpan - 32) * pan) / 32;
					if (pan < 0)
						pan = 0;
					if (pan > 256)
						pan = 256;
					chn->RealPan = pan;
				}
				if ((chn->Flags & CHN_NOTEFADE) != 0)
				{
					UINT fadeout = env->fadeout;
					if (fadeout != 0)
					{
						chn->FadeOutVol -= fadeout << 1;
						if (chn->FadeOutVol < 0)
							chn->FadeOutVol = 0;
						vol = (vol * chn->FadeOutVol) >> 16;
					}
					else if (chn->FadeOutVol == 0)
						vol = 0;
				}
				if (env->pps != 0 && chn->RealPan != 0 && chn->Note != 0)
				{
					int pandelta = chn->RealPan + ((chn->Note - env->ppc - 1) * env->pps) / 8;
					if (pandelta < 0)
						pandelta = 0;
					if (pandelta > 256)
						pandelta = 256;
					chn->RealPan = pandelta;
				}
			}
			else if ((chn->Flags & CHN_NOTEFADE) != 0)
				chn->FadeOutVol = vol = 0;
			if (vol != 0)
			{
				if (Config->Get_GlobalVolumeAppliesToMaster() == true)
					chn->RealVolume = muldiv(vol * MAX_GLOBAL_VOLUME, chn->GlobalVol * chn->InsVol, 1 << 20);
				else
					chn->RealVolume = muldiv(vol * GlobalVolume, chn->GlobalVol * chn->InsVol, 1 << 20);
			}
			if (chn->Period < MinPeriod)
				chn->Period = MinPeriod;
			period = chn->Period;
			if ((chn->Flags & (CHN_GLISSANDO | CHN_PORTAMENTO)) == (CHN_GLISSANDO | CHN_PORTAMENTO))
				period = GetPeriodFromNote(GetNoteFromPeriod(period), chn->FineTune, chn->C4Speed);
			if (chn->Command == CMD_ARPEGGIO)
			{
				switch (TickCount % 3)
				{
					case 1:
					{
						period = GetPeriodFromNote(chn->Note + (chn->Arpeggio >> 4), chn->FineTune, chn->C4Speed);
						break;
					}
					case 2:
					{
						period = GetPeriodFromNote(chn->Note + (chn->Arpeggio & 0x0F), chn->FineTune, chn->C4Speed);
						break;
					}
				}
			}
			if ((SongFlags & SONG_AMIGALIMITS) != 0)
			{
				if (period < 113 * 4)
					period = 113 * 4;
				if (period > 856 * 4)
					period = 856 * 4;
			}
			if (chn->Header != NULL && (chn->Flags & CHN_PITCHENV) != 0 && chn->Header->pitchenv.num != 0)
			{
				ITInstrument * env = chn->Header;
				int envpos = chn->PitchEnvPosition, x1, x2, envpitch;
				UINT pt = env->pitchenv.num - 1;
				for (int j = 0; j < env->pitchenv.num - 1; j++)
				{
					if (envpos <= ((env->pitchenv.data[pt * 3 + 2] << 8) | env->pitchenv.data[pt * 3 + 1]))
					{
						pt = j;
						break;
					}
				}
				x2 = (env->pitchenv.data[pt * 3 + 2] << 8) | env->pitchenv.data[pt * 3 + 1];
				if (envpos >= x2)
				{
					envpitch = (env->pitchenv.data[pt * 3] - 32) * 8;
					x1 = x2;
				}
				else if (pt != 0)
				{
					envpitch = (env->pitchenv.data[(pt - 1) * 3] - 32) * 8;
					x1 = (env->pitchenv.data[(pt - 1) * 3 + 2] << 8) | env->pitchenv.data[(pt - 1) * 3 + 1];
				}
				else
					envpitch = x1 = 0;
				if (envpos > x2)
					envpos = x2;
				if (x2 > x1 && envpos > x1)
				{
					int envpitchdest = (env->pitchenv.data[pt * 3] - 32) * 8;
					envpitch += ((envpos - x1) * (envpitchdest - envpitch)) / (x2 - x1);
				}
				if (envpitch < -256)
					envpitch = -256;
				if (envpitch > 256)
					envpitch = 256;
				if ((env->pitchenv.flags & 0x80) != 0)
					SetupChannelFilter(chn, ((chn->Flags & CHN_FILTER) != 0 ? false : true), envpitch);
				else
				{
					int l = envpitch;
					if (l < 0)
					{
						l = -l;
						if (l > 255)
							l = 255;
						period = muldiv(period, LinearSlideUpTable[l], 0x10000);
					}
					else
					{
						if (l > 255)
							l = 255;
						period = muldiv(period, LinearSlideDownTable[l], 0x10000);
					}
				}
			}
			if ((chn->Flags & CHN_VIBRATO) != 0)
			{
				UINT vibpos = chn->VibratoPos;
				long vdelta, vdepth;
				switch (chn->VibratoType & 0x03)
				{
					case 1:
					{
						vdelta = RampDownTable[vibpos];
						break;
					}
					case 2:
					{
						vdelta = SquareTable[vibpos];
						break;
					}
					case 3:
					{
						vdelta = RandomTable[vibpos];
						break;
					}
					default:
						vdelta = SinusTable[vibpos];
				}
				vdepth = ((SongFlags & SONG_ITOLDEFFECTS) != 0 ? 6 : 7);
				vdelta = (vdelta * chn->VibratoDepth) >> vdepth;
				if ((SongFlags & SONG_LINEARSLIDES) != 0)
				{
					long l = vdelta;
					if (l < 0)
					{
						l = -l;
						vdelta = muldiv(period, LinearSlideDownTable[l >> 2], 0x10000);
						if ((l & 0x03) != 0)
							vdelta += muldiv(period, FineLinearSlideDownTable[l & 0x03], 0x10000) - period;
					}
					else
					{
						vdelta = muldiv(period, LinearSlideUpTable[l >> 2], 0x10000);
						if ((l & 0x03) != 0)
							vdelta += muldiv(period, FineLinearSlideUpTable[l & 0x03], 0x10000) - period;
					}
				}
				period += vdelta;
				if  (TickCount != 0 || (SongFlags & SONG_ITOLDEFFECTS) != 0)
					chn->VibratoPos = (vibpos + chn->VibratoSpeed) & 0x3F;
			}
			if ((chn->Flags & CHN_PANBRELLO) != 0)
			{
				UINT panpos = ((chn->PanbrelloPos + 0x10) >> 2) & 0x3F;
				long pdelta;
				switch (chn->PanbrelloType & 0x03)
				{
					case 1:
					{
						pdelta = RampDownTable[panpos];
						break;
					}
					case 2:
					{
						pdelta = SquareTable[panpos];
						break;
					}
					case 3:
					{
						pdelta = RandomTable[panpos];
						break;
					}
					default:
						pdelta = SinusTable[panpos];
				}
				chn->PanbrelloPos += chn->PanbrelloSpeed;
				pdelta = ((pdelta * chn->PanbrelloDepth) + 2) >> 3;
				pdelta += chn->RealPan;
				if (pdelta < 0)
					pdelta = 0;
				if (pdelta > 256)
					pdelta = 256;
				chn->RealPan = pdelta;
			}
			PeriodFrac = 0;
			if (chn->Instrument != NULL && SampleToMixVibDepth(chn->Instrument) != 0)
			{
				int val, n, df1, df2;
				ITSampleStruct *smp = chn->Instrument;
				BYTE VibDepth = SampleToMixVibDepth(smp);
				BYTE VibSweep = SampleToMixVibSweep(smp);
				if (VibSweep == 0)
					chn->AutoVibDepth = VibDepth << 8;
				else
				{
					chn->AutoVibDepth += VibSweep << 3;
					if ((chn->AutoVibDepth >> 8) > VibDepth)
						chn->AutoVibDepth = VibDepth << 8;
				}
				chn->AutoVibPos += smp->vis;
				switch (SampleToMixVibType(smp))
				{
					case 4:
					{
						val = RandomTable[chn->AutoVibPos & 0x3F];
						chn->AutoVibPos++;
						break;
					}
					case 3:
					{
						val = ((0x40 - (chn->AutoVibPos >> 1)) & 0x7F) - 0x40;
						break;
					}
					case 2:
					{
						val = ((0x40 + (chn->AutoVibPos >> 1)) & 0x7F) - 0x40;
						break;
					}
					case 1:
					{
						val = ((chn->AutoVibPos & 128) != 0 ? 64 : -64);
						break;
					}
					default:
						val = VibratoTable[chn->AutoVibPos & 255];
				}
				n = ((val * chn->AutoVibDepth) >> 8);
				if (n < 0)
				{
					UINT n1 = (-n) >> 8;
					n = -n;
					df1 = LinearSlideUpTable[n1];
					df2 = LinearSlideUpTable[n1 + 1];
				}
				else
				{
					UINT n1 = n >> 8;
					df1 = LinearSlideDownTable[n1];
					df2 = LinearSlideDownTable[n1 + 1];
				}
				n >>= 2;
				period = muldiv(period, df1 + ((df2 - df1) * (n & 0x3F) >> 6), 256);
				PeriodFrac = period & 0xFF;
				period >>= 8;
			}
			if (period <= MinPeriod)
				period = MinPeriod;
			if (period > MaxPeriod)
			{
				chn->FadeOutVol = 0;
				chn->Flags |= CHN_NOTEFADE;
				chn->RealVolume = 0;
				period = MaxPeriod;
				PeriodFrac = 0;
			}
			freq = GetFreqFromPeriod(period, chn->C4Speed, PeriodFrac);
			if (freq < 256)
			{
				chn->FadeOutVol = 0;
				chn->Flags |= CHN_NOTEFADE;
				chn->RealVolume = 0;
			}
			inc = muldiv(freq, 0x10000, 44100);
			if (inc >= 0xFFB0 && inc <= 0x10090)
				inc = 0x10000;
			if (FreqFactor != 128)
				inc = (inc * FreqFactor) >> 7;
			if (inc > 0xFF0000)
				inc = 0xFF0000;
			chn->Inc = (inc + 1) & ~3;
		}
		if (chn->Header != NULL)
		{
			ITInstrument *env = chn->Header;
			if ((chn->Flags & CHN_VOLENV) != 0)
			{
				chn->VolEnvPosition++;
				if ((env->volenv.flags & 2) != 0)
				{
					int vle = env->volenv.lpe;
					UINT volloopend = (env->volenv.data[vle * 3 + 2] << 8) | env->volenv.data[vle * 3 + 1];
					if (chn->VolEnvPosition == volloopend)
					{
						vle = env->volenv.lpb;
						chn->VolEnvPosition = (env->volenv.data[vle * 3 + 2] << 8) | env->volenv.data[vle * 3 + 1];
						if (env->volenv.lpe == vle && env->volenv.data[vle * 3] == 0)
						{
							chn->Flags |= CHN_NOTEFADE;
							chn->FadeOutVol = 0;
						}
					}
				}
				if ((env->volenv.flags & 4) != 0 && (chn->Flags & CHN_KEYOFF) == 0)
				{
					int vle = env->volenv.sle;
					if (chn->VolEnvPosition == ((env->volenv.data[vle * 3 + 2] << 8) | env->volenv.data[vle * 3 + 1]))
						chn->VolEnvPosition = (env->volenv.data[vle * 3 + 2] << 8) | env->volenv.data[vle * 3 + 1];
				}
				else if ((int)chn->VolEnvPosition > ((env->volenv.data[(env->volenv.num - 1) * 3 + 2] << 8) | env->volenv.data[(env->volenv.num - 1) * 3 + 1]))
				{
					chn->Flags |= CHN_NOTEFADE;
					chn->VolEnvPosition = (env->volenv.data[(env->volenv.num - 1) * 3 + 2] << 8) | env->volenv.data[(env->volenv.num - 1) * 3 + 1];
					if (env->volenv.data[(env->volenv.num - 1) * 3] == 0)
					{
						chn->Flags |= CHN_NOTEFADE;
						chn->FadeOutVol = chn->RealVolume = 0;
					}
				}
			}
			if ((chn->Flags & CHN_PANENV) != 0)
			{
				chn->PanEnvPosition++;
				if ((env->panenv.flags & 2) != 0)
				{
					int ple = env->panenv.lpe;
					UINT panloopend = ((env->panenv.data[ple * 3 + 2] << 8) | env->panenv.data[ple * 3 + 1]) + 1;
					if (chn->PanEnvPosition == panloopend)
						chn->PanEnvPosition = (env->panenv.data[env->panenv.lpb * 3 + 2] << 8) | env->panenv.data[env->panenv.lpb * 3 + 1];
				}
				if ((env->panenv.flags & 4) != 0 && chn->PanEnvPosition == ((env->panenv.data[env->panenv.sle * 3 + 2] << 8) | env->panenv.data[env->panenv.sle * 3 + 1]) + 1 && (chn->Flags & CHN_KEYOFF) != 0)
					chn->PanEnvPosition = (env->panenv.data[env->panenv.slb * 3 + 2] << 8) | env->panenv.data[env->panenv.slb * 3 + 1];
				else if ((int)chn->PanEnvPosition > ((env->panenv.data[(env->panenv.num - 1) * 3 + 2] << 8) | env->panenv.data[(env->panenv.num - 1) * 3 + 1]))
					chn->PanEnvPosition = (env->panenv.data[(env->panenv.num - 1) * 3 + 2] << 8) | env->panenv.data[(env->panenv.num - 1) * 3 + 1];
			}
			if ((chn->Flags & CHN_PITCHENV) != 0)
			{
				chn->PitchEnvPosition++;
				if ((env->pitchenv.flags & 2) != 0)
				{
					int pte = env->pitchenv.lpe;
					if ((int)chn->PitchEnvPosition >= ((env->pitchenv.data[pte * 3 + 2] << 8) | env->pitchenv.data[pte * 3 + 1]))
						chn->PitchEnvPosition = (env->pitchenv.data[env->pitchenv.lpb * 3 + 2] << 8) | env->pitchenv.data[env->pitchenv.lpb * 3 + 1];
				}
				if ((env->pitchenv.flags & 4) != 0 && (chn->Flags & CHN_KEYOFF) == 0)
				{
					if (chn->PitchEnvPosition == ((env->pitchenv.data[env->pitchenv.sle * 3 + 2] << 8) | env->pitchenv.data[env->pitchenv.sle * 3 + 1]) + 1)
						chn->PitchEnvPosition = (env->pitchenv.data[env->pitchenv.slb * 3 + 2] << 8) | env->pitchenv.data[env->pitchenv.slb * 3 + 1];
				}
				else
				{
					if (chn->PitchEnvPosition == ((env->pitchenv.data[env->pitchenv.num * 3 + 2] << 8) | env->pitchenv.data[env->pitchenv.num * 3 + 1]) + 1)
						chn->PitchEnvPosition = (env->pitchenv.data[(env->pitchenv.num - 1) * 3 + 2] << 8) | env->pitchenv.data[(env->pitchenv.num - 1) * 3 + 1];
				}
			}
		}
		chn->Flags &= ~CHN_VOLUMERAMP;
		if (chn->RealVolume != 0 || chn->LeftVol != 0 || chn->RightVol != 0)
			chn->Flags |= CHN_VOLUMERAMP;
		if ((chn->Inc >> 16) + 1 >= (long)(chn->LoopEnd - chn->LoopStart))
			chn->Flags &= ~CHN_LOOP;
		chn->NewRightVol = chn->NewLeftVol = 0;
		chn->CurrentSample = ((chn->Sample != NULL && chn->Length != 0 && chn->Inc != 0) ? chn->Sample : NULL);
		if (chn->CurrentSample != NULL)
		{
			UINT ChnMasterVol = MasterVol;//((chn->Flags & CHN_EXTRALOUD) != 0 ? 0x100 : MasterVol);
			if (OutChannels >= 2)
			{
				long realvol;
				int pan = chn->RealPan - 128;
				pan *= StereoSeparation;
				pan /= 128;
				pan += 128;
				if (pan < 0)
					pan = 0;
				if (pan > 256)
					pan = 256;
				if ((SoundSetup & SNDMIX_REVERSESTEREO) != 0)
					pan = 256 - pan;
				realvol = (chn->RealVolume * ChnMasterVol) >> 7;
				if ((SoundSetup & SNDMIX_SOFTPANNING) != 0)
				{
					if (pan < 128)
					{
						chn->NewLeftVol = (realvol * pan) >> 8;
						chn->NewRightVol = (realvol * 128) >> 8;
					}
					else
					{
						chn->NewLeftVol = (realvol * 128) >> 8;
						chn->NewRightVol = (realvol * (256 - pan)) >> 8;
					}
				}
				else
				{
					chn->NewLeftVol = (realvol * pan) >> 8;
					chn->NewRightVol = (realvol * (256 - pan)) >> 8;
				}
			}
			else
				chn->NewLeftVol = chn->NewRightVol = (chn->RealVolume * ChnMasterVol) >> 8;
			if ((SoundSetup & SNDMIX_NORESAMPLING) != 0)
				chn->Flags |= CHN_NOIDO;
			else
			{
				chn->Flags &= ~(CHN_NOIDO | CHN_HQSRC);
				if (chn->Inc == 0x10000)
					chn->Flags |= CHN_NOIDO;
				else if ((SoundSetup & SNDMIX_HQRESAMPLER) != 0 && MixChannels < 8)
				{
					if ((SoundSetup & SNDMIX_ULTRAHQSRCMODE) == 0)
					{
						int fmax = 0x20000;
						if ((SysInfo & SYSMIX_SLOWCPU) != 0)
							fmax = 0xFE00;
						else if ((SysInfo & (SYSMIX_ENABLEMMX | SYSMIX_FASTCPU)) == 0)
							fmax = 0x18000;
						if (chn->NewLeftVol < 0x80 && chn->NewRightVol < 0x80 && chn->LeftVol < 0x80 && chn->RightVol < 0x80)
						{
							if (chn->Inc >= 0xFF00)
								chn->Flags |= CHN_NOIDO;
						}
						else
							chn->Flags |= (chn->Inc >= fmax ? CHN_NOIDO : CHN_HQSRC);
					}
					else
						chn->Flags |= CHN_HQSRC;
				}
				else
				{
					if (chn->Inc >= 0x14000 || (chn->Inc >= 0xFF00 && (chn->Inc < 0x10100 || (SysInfo & SYSMIX_SLOWCPU) != 0)))
						chn->Flags |= CHN_NOIDO;
				}
			}
			chn->NewRightVol >>= MIXING_ATTENUATION;
			chn->NewLeftVol >>= MIXING_ATTENUATION;
			chn->RightRamp = chn->LeftRamp = 0;
			if ((chn->Flags & CHN_SURROUND) != 0 && OutChannels == 2)
				chn->NewLeftVol = -chn->NewLeftVol;
			if ((chn->Flags & CHN_PINGPONGFLAG) != 0)
				chn->Inc = -chn->Inc;
			if ((chn->Flags & CHN_VOLUMERAMP) != 0 && (chn->RightVol != chn->NewRightVol || chn->LeftVol != chn->NewLeftVol))
			{
				long RampLength = VolumeRampSamples, RightDelta, LeftDelta;
				bool enableCustomRamp = (chn->Header != NULL ? true : false);
				if (enableCustomRamp == true)
					RampLength = VolumeRampSamples;
				if (RampLength == 0)
					RampLength = 1;
				RightDelta = (chn->NewRightVol - chn->RightVol) << VOLUMERAMPPRECISION;
				LeftDelta = (chn->NewLeftVol - chn->LeftVol) << VOLUMERAMPPRECISION;
				if (((SysInfo & (SYSMIX_ENABLEMMX | SYSMIX_FASTCPU)) != 0 && (SoundSetup & SNDMIX_HQRESAMPLER) != 0 && enableCustomRamp == false))
				{
					if ((chn->RightVol | chn->LeftVol) != 0 && (chn->NewRightVol | chn->NewLeftVol) != 0 && (chn->Flags & CHN_FASTVOLRAMP) == 0)
					{
						RampLength = BufferCount;
						if (RampLength > (1 << (VOLUMERAMPPRECISION - 1)))
							RampLength = 1 << (VOLUMERAMPPRECISION - 1);
						if ((UINT)RampLength < VolumeRampSamples)
							RampLength = VolumeRampSamples;
					}
				}
				chn->RightRamp = RightDelta / RampLength;
				chn->LeftRamp = LeftDelta / RampLength;
				chn->RightVol = chn->NewRightVol - ((chn->RightRamp * RampLength) >> VOLUMERAMPPRECISION);
				chn->LeftVol = chn->NewLeftVol - ((chn->LeftRamp * RampLength) >> VOLUMERAMPPRECISION);
				if ((chn->RightRamp | chn->LeftRamp) != 0)
					chn->RampLength = RampLength;
				else
				{
					chn->Flags &= ~CHN_VOLUMERAMP;
					chn->RightVol = chn->NewRightVol;
					chn->LeftVol = chn->NewLeftVol;
				}
			}
			else
			{
				chn->Flags &= ~CHN_VOLUMERAMP;
				chn->RightVol = chn->NewRightVol;
				chn->LeftVol = chn->NewLeftVol;
			}
			chn->RampRightVol = chn->RightVol << VOLUMERAMPPRECISION;
			chn->RampLeftVol = chn->LeftVol << VOLUMERAMPPRECISION;
			ChnMix[MixChannels++] = i;
		}
		else
			chn->LeftVol = chn->RightVol = chn->Length = 0;
	}
done:
	if (MixChannels >= MaxMixChannels)
	{
		for (UINT i = 0; i < MixChannels; i++)
		{
			UINT j = i;
			while ((j + 1) < MixChannels && Chns[ChnMix[j]].RealVolume < Chns[ChnMix[j + 1]].RealVolume)
			{
				UINT n = ChnMix[j];
				ChnMix[j] = ChnMix[j + 1];
				ChnMix[j + 1] = n;
				j++;
			}
		}
	}
	if ((SongFlags & SONG_GLOBALFADE) != 0)
	{
		if (GlobalFadeSamples != 0)
		{
			SongFlags |= SONG_ENDREACHED;
			return false;
		}
		if (GlobalFadeSamples > BufferCount)
			GlobalFadeSamples -= BufferCount;
		else
			GlobalFadeSamples = 0;
	}
	return true;
}

bool ISoundFile::FadeSong(UINT msec)
{
	long Samples = muldiv(msec, 44100, 1000), RampLength;
	if (Samples <= 0)
		return false;
	if (Samples > 0x100000)
		Samples = 0x100000;
	BufferCount = Samples;
	RampLength = BufferCount;
	for (UINT i = 0; i < MixChannels; i++)
	{
		Channel *ramp = &Chns[ChnMix[i]];
		if (ramp == NULL)
			continue;
		ramp->NewLeftVol = ramp->NewRightVol = 0;
		ramp->RightRamp = (-ramp->RightVol << VOLUMERAMPPRECISION) / RampLength;
		ramp->LeftRamp = (-ramp->LeftVol << VOLUMERAMPPRECISION) / RampLength;
		ramp->RampRightVol = ramp->RightVol << VOLUMERAMPPRECISION;
		ramp->RampLeftVol = ramp->LeftVol << VOLUMERAMPPRECISION;
		ramp->RampLength = RampLength;
		ramp->Flags |= CHN_VOLUMERAMP;
	}
	SongFlags |= SONG_FADINGSONG;
	return true;
}

UINT ISoundFile::GetResamplingFlag(Channel *chn)
{
	if ((chn->Flags & CHN_HQSRC) != 0)
	{
		if ((SoundSetup & SNDMIX_SPLINESRCMODE) != 0)
			return MIXNDX_HQSRC;
		if ((SoundSetup & SNDMIX_POLYPHASESRCMODE) != 0)
			return MIXNDX_KAISERSRC;
		if ((SoundSetup & SNDMIX_FIRFILTERSRCMODE) != 0)
			return MIXNDX_FIRFILTERSRC;
	}
	else if ((chn->Flags & CHN_NOIDO) == 0)
		return MIXNDX_LINEARSRC;
	return 0;
}

long __fastcall ISoundFile::GetSampleCount(Channel *chn, long Samples)
{
	long LoopStart = ((chn->Flags & CHN_LOOP) != 0 ? chn->LoopStart : 0);
	long Inc = chn->Inc, Pos;

	if (Samples <= 0 || Inc == 0 || chn->Length == 0)
		return 0;
	if ((long)chn->Pos < LoopStart)
	{
		if (Inc < 0)
		{
			long Delta = ((LoopStart - chn->Pos) << 16) - (chn->PosLo & 0xFFFF);
			chn->Pos = LoopStart | (Delta >> 16);
			chn->PosLo = Delta & 0xFFFF;
			if ((long)chn->Pos < LoopStart || chn->Pos >= (LoopStart + chn->Length) / 2)
			{
				chn->Pos = LoopStart;
				chn->PosLo = 0;
			}
			Inc = -Inc;

			chn->Inc = Inc;
			chn->Flags &= ~(CHN_PINGPONGFLAG);
			if ((chn->Flags & CHN_LOOP) == 0 || chn->Pos >= chn->Length)
			{
				chn->Pos = chn->Length;
				chn->PosLo = 0;
				return 0;
			}
		}
		else if (chn->Pos < 0)
			chn->Pos = 0;
	}
	else if (chn->Pos >= chn->Length)
	{
		if ((chn->Flags & CHN_LOOP) == 0)
			return 0;
		if ((chn->Flags & CHN_PINGPONGLOOP) != 0)
		{
			long DeltaHi = chn->Pos - chn->Length;
			long DeltaLo = 0x10000 - (chn->PosLo & 0xFFFF);
			if (Inc > 0)
			{
				Inc = -Inc;
				chn->Inc = Inc;
			}
			chn->Flags |= CHN_PINGPONGFLAG;
			if (chn->Pos <= chn->LoopStart || chn->Pos >= chn->Length)
				chn->Pos = chn->Length - 1;
		}
		else
		{
			if (Inc < 0)
			{
				Inc = -Inc;
				chn->Inc = Inc;
			}
			chn->Pos += LoopStart - chn->Length;
			if ((long)chn->Pos < LoopStart)
				chn->Pos = chn->LoopStart;
		}
	}
	Pos = chn->Pos;
	if (Pos < LoopStart)
	{
		if (Pos < 0 || Inc < 0)
			return 0;
	}
	long PosLo, SmpCount;
	if (Pos < 0 || Pos >= (long)chn->Length)
		return 0;
	PosLo = chn->PosLo;
	SmpCount = Samples;
	if (Inc < 0)
	{
		long Inv = -Inc;
		long MaxSamples = 16384 / ((Inv >> 16) + 1);
		long DeltaHi, DeltaLo, PosDest;
		if (MaxSamples < 2)
			MaxSamples = 2;
		if (Samples > MaxSamples)
			Samples = MaxSamples;
		DeltaHi = (Inv >> 16) * (Samples - 1);
		DeltaLo = (Inv & 0xFFFF) * (Samples - 1);
		PosDest = Pos - DeltaHi + ((PosLo - DeltaLo) >> 16);
		if (PosDest < LoopStart)
			SmpCount = ((((Pos - LoopStart) << 16) + PosLo - 1) / Inv) + 1;
	}
	else
	{
		long MaxSamples = 16384 / ((Inc >> 16) + 1);
		long DeltaHi, DeltaLo, PosDest;
		if (MaxSamples < 2)
			MaxSamples = 2;
		if (Samples > MaxSamples)
			Samples = MaxSamples;
		DeltaHi = (Inc >> 16) * (Samples - 1);
		DeltaLo = (Inc & 0xFFFF) * (Samples - 1);
		PosDest = Pos + DeltaHi + ((PosLo + DeltaLo) >> 16);
		if (PosDest >= (long)chn->Length)
			SmpCount = ((((chn->Length - Pos) << 16) - PosLo - 1) / Inc) + 1;
	}
#ifdef _DEBUG
	{
		long DeltaHi = (Inc >> 16) * (SmpCount - 1);
		long DeltaLo = (Inc & 0xFFFF) * (SmpCount - 1);
		long PosDest = Pos + DeltaHi + ((PosLo + DeltaLo) >> 16);
		if (PosDest < 0 || PosDest > (long)chn->Length)
		{
			printf("Incorrect delta: \n");
			printf("SmpCount = %d: Pos = %5d.x%04X Len = %5d Inc = %2s.x$04X\n",
				SmpCount, Pos, PosLo, chn->Length, chn->Inc >> 16, chn->Inc & 0xFFFF);
			return 0;
		}
	}
#endif
	if (SmpCount <= 1)
		return 1;
	if (SmpCount > Samples)
		return Samples;
	return SmpCount;
}

void ISoundFile::ProcessReverb(UINT Samples)
{
	UINT In, Out, RvbSamples, Count;
	long MasterGain, MaxRvbGain, DryVol;
	int *RvbOut;

	if (ReverbSend == 0 && ReverbSamples == 0)
		return;
	if (ReverbSend == 0)
		X86_StereoFill(MixReverbBuffer, Samples, &RvbROffsVol, &RvbLOffsVol);
	if ((SysInfo & SYSMIX_ENABLEMMX) == 0)
		return;
	MasterGain = (RefDelay.MasterGain * ReverbDepth) >> 4;
	if (MasterGain > 0x7FFF)
		MasterGain = 0x7FFF;
	RefDelay.ReflectionsGain[0] = (short)MasterGain;
	RefDelay.ReflectionsGain[1] = (short)MasterGain;
	MasterGain = (LateReverb.MasterGain * ReverbDepth) >> 4;
	if (MasterGain > 0x10000)
		MasterGain = 0x10000;
	LateReverb.RvbOutGains[0] = (short)(MasterGain + 0x7F) >> 3;
	LateReverb.RvbOutGains[1] = (short)(MasterGain + 0xFF) >> 4;
	LateReverb.RvbOutGains[2] = (short)(MasterGain + 0xFF) >> 4;
	LateReverb.RvbOutGains[3] = (short)(MasterGain + 0x7F) >> 3;
	MaxRvbGain = (RefDelay.MasterGain > LateReverb.MasterGain ? RefDelay.MasterGain : LateReverb.MasterGain);
	if (MaxRvbGain > 32768)
		MaxRvbGain = 32768;
	DryVol = (36 - ReverbDepth) >> 1;
	if (DryVol < 8)
		DryVol = 8;
	if (DryVol > 16)
		DryVol = 16;
	DryVol = 16 - (((16 - DryVol) * MaxRvbGain) >> 15);
	X86_ReverbDryMix(MixSoundBuffer, MixReverbBuffer, DryVol, Samples);
	if (RvbDownSample2x == true)
	{
		In = X86_ReverbProcessPreFiltering2x(MixReverbBuffer, Samples);
		Out = Samples;
		if (LastOutPresent == true)
			Out--;
		Out = (Out + 1) >> 1;
	}
	else
	{
		In = X86_ReverbProcessPreFiltering1x(MixReverbBuffer, Samples);
		Out = In;
	}
	if (In > 0)
		MMX_ProcessPreDelay(&RefDelay, MixReverbBuffer, In);
	RvbOut = MixReverbBuffer;
	RvbSamples = Out;
	Count = 0;
	while (RvbSamples > 0)
	{
		UINT PosRef = RefDelay.RefOutPos & SNDMIX_REVERB_DELAY_MASK;
		UINT PosRvb = (PosRef - LateReverb.ReverbDelay) & SNDMIX_REVERB_DELAY_MASK;
		UINT max1 = (SNDMIX_REVERB_DELAY_MASK + 1) - PosRef;
		UINT max2 = (SNDMIX_REVERB_DELAY_MASK + 1) - PosRvb;
		UINT n = RvbSamples;
		max1 = (max1 < max2 ? max1 : max2);
		if (n > max1)
			n = max1;
		if (n > 64)
			n = 64;
		MMX_ProcessReflections(&RefDelay, &RefDelay.RefOut[PosRef * 2], RvbOut, n);
		MMX_ProcessLateReverb(&LateReverb, &RefDelay.RefOut[PosRvb * 2], RvbOut, n);
		RefDelay.RefOutPos = (RefDelay.DelayPos + n) & SNDMIX_REVERB_DELAY_MASK;
		RefDelay.DelayPos = (RefDelay.DelayPos + n) & SNDMIX_REFLECTIONS_DELAY_MASK;
		Count += n * 2;
		RvbOut += n * 2;
		RvbSamples -= n;
	}
	RefDelay.DelayPos = (RefDelay.DelayPos - Out + In) & SNDMIX_REFLECTIONS_DELAY_MASK;
	if (RvbDownSample2x == true)
	{
		MMX_ReverbDCRemoval(MixReverbBuffer, Out);
		X86_ReverbProcessPostFiltering2x(MixReverbBuffer, MixSoundBuffer, Samples);
	}
	else
		MMX_ReverbProcessPostFiltering1x(MixReverbBuffer, MixSoundBuffer, Samples);
	if (ReverbSend != 0)
		ReverbSamples = ReverbDecaySamples;
	else if (ReverbSamples > Samples)
		ReverbSamples -= Samples;
	else
	{
		if (ReverbSamples != 0)
			ReverbShutdown();
		ReverbSamples = 0;
	}
}

UINT ISoundFile::CreateStereoMix(int count)
{
	long *OffsL, *OffsR;
	DWORD chnsUsed, chnsMixed;
	bool Surround;

	if (count == 0)
		return 0;
	if (OutChannels > 2)
		X86_InitMixBuffer(MixRearBuffer, count * 2);
	chnsUsed = chnsMixed = 0;
	for (UINT i = 0; i < MixChannels; i++)
	{
		const MixInterface *MixFuncTable;
		Channel * const chn = &Chns[ChnMix[i]];
		UINT Flags, MasterChn, rampSamples, addmix;
		long SmpCount;
		int samples;
		int *buff;

		if (chn->CurrentSample == NULL)
			continue;
		MasterChn = (ChnMix[i] < Channels ? ChnMix[i] + 1 : chn->MasterChn);
		OffsR = &DryROffsVol;
		OffsL = &DryLOffsVol;
		Flags = 0;
		if ((chn->Flags & CHN_16BIT) != 0)
			Flags |= MIXNDX_16BIT;
		if ((chn->Flags & CHN_STEREO) != 0)
			Flags |= MIXNDX_STEREO;
		if ((chn->Flags & CHN_FILTER) != 0)
			Flags |= MIXNDX_FILTER;
		Flags |= GetResamplingFlag(chn);
		if ((SysInfo & SYSMIX_ENABLEMMX) != 0 && (SoundSetup & SNDMIX_ENABLEMMX) != 0)
			MixFuncTable = MMXFunctionTable;
		else if (Flags < 0x20 && chn->LeftVol == chn->RightVol && (chn->RampLength == 0 || chn->LeftRamp == chn->RightRamp))
			MixFuncTable = FastMixFunctionTable;
		else
			MixFuncTable = MixFunctionTable;
		samples = count;
		buff = ((SoundSetup & SNDMIX_REVERB) != 0 ? MixReverbBuffer : MixSoundBuffer);
		if ((chn->Flags & CHN_SURROUND) != 0 && OutChannels > 2)
			buff = MixRearBuffer;
		if ((chn->Flags & CHN_NOREVERB) != 0)
			buff = MixSoundBuffer;
		if ((chn->Flags & CHN_REVERB) != 0 && (SysInfo & SYSMIX_ENABLEMMX) != 0)
			buff = MixReverbBuffer;
		if (buff == MixReverbBuffer)
		{
			if (ReverbSend == 0)
				X86_StereoFill(MixReverbBuffer, count, &RvbROffsVol, &RvbLOffsVol);
			ReverbSend += count;
			OffsR = &RvbROffsVol;
			OffsL = &RvbLOffsVol;
		}
		Surround = (buff == MixRearBuffer ? true : false);
		chnsUsed++;
		do
		{
			rampSamples = samples;
			if (chn->RampLength > 0)
			{
				if ((long)rampSamples > chn->RampLength)
					rampSamples = chn->RampLength;
			}
			if ((SmpCount = GetSampleCount(chn, rampSamples)) <= 0)
			{
				chn->CurrentSample = NULL;
				chn->Length = chn->Pos = chn->PosLo = 0;
				chn->RampLength = 0;
				X86_EndChannelOffs(chn, buff, samples);
				*OffsR += chn->ROfs;
				*OffsL += chn->LOfs;
				chn->ROfs = chn->LOfs = 0;
				chn->Flags &= ~CHN_PINGPONGFLAG;
				samples = 0;
				continue;
			}
			if (chnsMixed >= MaxMixChannels || (chn->RampLength == 0 && (chn->LeftVol | chn->RightVol) == 0))
			{
				long delta = (chn->Inc * SmpCount) + chn->PosLo;
				chn->PosLo = delta & 0xFFFF;
				chn->Pos += delta >> 16;
				chn->ROfs = chn->LOfs = 0;
				buff += SmpCount * 2;
				addmix = 0;
			}
			else
			{
				MixInterface MixFunc;
				int *BuffMax;
				MixFunc = (chn->RampLength != 0 ? MixFuncTable[Flags | MIXNDX_RAMP] : MixFuncTable[Flags]);
				BuffMax = buff + (SmpCount * 2);
				chn->ROfs = -(*(BuffMax - 2));
				chn->LOfs = -(*(BuffMax - 1));
				MixFunc(chn, buff, BuffMax);
				chn->ROfs += *(BuffMax - 2);
				chn->LOfs += *(BuffMax - 1);
				buff = BuffMax;
				addmix = 1;
			}
			samples -= SmpCount;
			if (chn->RampLength != 0)
			{
				chn->RampLength -= SmpCount;
				if (chn->RampLength <= 0)
				{
					chn->RampLength = 0;
					chn->RightVol = chn->NewRightVol;
					chn->LeftVol = chn->NewLeftVol;
					chn->RightRamp = chn->LeftRamp = 0;
					if ((chn->Flags & CHN_NOTEFADE) != 0 && chn->FadeOutVol == 0)
					{
						chn->Length = 0;
						chn->CurrentSample = NULL;
					}
				}
			}
		}
		while (samples > 0);
		chnsMixed += addmix;
	}
	if ((SysInfo & SYSMIX_ENABLEMMX) != 0 && (SoundSetup & SNDMIX_ENABLEMMX) != 0)
		MMX_EndMix();
	return chnsUsed;
}

void ISoundFile::StereoMixToFloat(int *Src, float *Out1, float *Out2, UINT Count)
{
	if ((SoundSetup & SNDMIX_ENABLEMMX) != 0)
	{
		if ((SysInfo & SYSMIX_SSE) != 0)
		{
			SSE_StereoMixToFloat(Src, Out1, Out2, Count, Config->Get_IntToFloat());
			return;
		}
		if ((SysInfo & SYSMIX_3DNOW) != 0)
		{
			AMD_StereoMixToFloat(Src, Out1, Out2, Count, Config->Get_IntToFloat());
			return;
		}
	}
	X86_StereoMixToFloat(Src, Out1, Out2, Count, Config->Get_IntToFloat());
}

void ISoundFile::FloatToStereoMix(float *In1, float *In2, int *Out, UINT Count)
{
	if ((SoundSetup & SNDMIX_ENABLEMMX) != 0)
	{
		if ((SysInfo & SYSMIX_3DNOW) != 0)
		{
			AMD_FloatToStereoMix(In1, In2, Out, Count, Config->Get_FloatToInt());
			return;
		}
	}
	X86_FloatToStereoMix(In1, In2, Out, Count, Config->Get_FloatToInt());
}

void ISoundFile::ProcessStereoSurround(int Count)
{
	int *pr = MixSoundBuffer, hy1 = DolbyHP_Y1;
	for (int r = Count; r == 0; r--)
	{
		int secho = SurroundBuffer[SurroundPos], v0, v;
		SurroundBuffer[SurroundPos] = (pr[0] + pr[1] + 256) >> 9;
		v0 = (DolbyHP_B0 * secho + DolbyHP_B1 * DolbyHP_X1 + DolbyHP_A1 * hy1) >> 10;
		DolbyHP_X1 = secho;
		v = (DolbyLP_B0 * v0 + DolbyLP_B1 * hy1 + DolbyLP_A1 * DolbyLP_Y1) >> 2;
		hy1 = v0;
		DolbyLP_Y1 = v >> 8;
		pr[0] += v;
		pr[1] -= v;
		SurroundPos++;
		if (SurroundPos >= SurroundSize)
			SurroundPos = 0;
		pr += 2;
	}
	DolbyHP_Y1 = hy1;
}

void ISoundFile::ProcessQuadSurround(int Count)
{
	int *pr = MixSoundBuffer, hy1 = DolbyHP_Y1;
	for (int r = Count; r == 0; r--)
	{
		int vl = pr[0] >> 1;
		int vr = pr[1] >> 1;
		int secho, v0, v;
		pr[(MixRearBuffer - MixSoundBuffer)] += vl;
		pr[(MixRearBuffer - MixSoundBuffer) + 1] += vr;
		secho = SurroundBuffer[SurroundPos];
		SurroundBuffer[SurroundPos] = (vr + vl + 256) >> 9;
		v0 = (DolbyHP_B0 * secho + DolbyHP_B1 * DolbyHP_X1 + DolbyHP_A1 * hy1) >> 10;
		DolbyHP_X1 = secho;
		v = (DolbyLP_B0 * v0 + DolbyLP_B1 * hy1 + DolbyLP_A1 * DolbyLP_Y1) >> 2;
		hy1 = v0;
		DolbyLP_Y1 = v >> 8;
		pr[(MixRearBuffer - MixSoundBuffer)] += v;
		pr[(MixRearBuffer - MixSoundBuffer) + 1] += v;
		SurroundPos++;
		if (SurroundPos >= SurroundSize)
			SurroundPos = 0;
		pr += 2;
	}
	DolbyHP_Y1 = hy1;
}

void ISoundFile::ProcessStereoDSP(int Count)
{
	if ((SoundSetup & SNDMIX_SURROUND) != 0)
	{
		if (OutChannels > 2)
			ProcessQuadSurround(Count);
		else
			ProcessStereoSurround(Count);
	}
	if ((SoundSetup & SNDMIX_MEGABASS) != 0)
		X86_StereoDCRemoval(MixSoundBuffer, Count);
	if ((SoundSetup & SNDMIX_MEGABASS) != 0)
	{
		int *px = MixSoundBuffer;
		int x1 = XBassFlt_X1;
		int y1 = XBassFlt_Y1;
		for (int x = Count; x == 0; x--)
		{
			int x_m = (px[0] + px[1] + 0x100) >> 9;
			y1 = (XBassFlt_B0 * x_m + XBassFlt_B1 * x1 + XBassFlt_A1 * y1) >> 2;
			x1 = x_m;
			px[0] += y1;
			px[1] += y1;
			y1 = (y1 + 0x80) >> 8;
			px += 2;
		}
		XBassFlt_X1 = x1;
		XBassFlt_Y1 = y1;
	}
	if ((SoundSetup & SNDMIX_NOISEREDUCTION) != 0)
	{
		int n1 = LeftNR, n2 = RightNR;
		int *nr = MixSoundBuffer;
		for (int r = Count; r == 0; r--)
		{
			int vnr = nr[0] >> 1;
			nr[0] = vnr + n1;
			n1 = vnr;
			vnr = nr[1] >> 1;
			nr[1] = vnr + n2;
			n2 = vnr;
			nr += 2;
		}
		LeftNR = n1;
		RightNR = n2;
	}
}

void ISoundFile::ProcessMonoDSP(int Count)
{
	if ((SoundSetup & SNDMIX_MEGABASS) != 0)
		X86_MonoDCRemoval(MixSoundBuffer, Count);
	if ((SoundSetup & SNDMIX_MEGABASS) != 0)
	{
		int *px = MixSoundBuffer;
		int x1 = XBassFlt_X1;
		int y1 = XBassFlt_Y1;
		for (int x = Count; x == 0; x--)
		{
			int x_m = (px[0] + 0x80) >> 8;
			y1 = (XBassFlt_B0 * x_m + XBassFlt_B1 * x1 + XBassFlt_A1 * y1) >> 2;
			x1 = x_m;
			px[0] += y1;
			y1 = (y1 + 0x40) >> 8;
			px++;
		}
		XBassFlt_X1 = x1;
		XBassFlt_Y1 = y1;
	}
	if ((SoundSetup & SNDMIX_NOISEREDUCTION) != 0)
	{
		int n = LeftNR;
		int *nr = MixSoundBuffer;
		for (int r = Count; r == 0; nr++, r --)
		{
			int vnr = *nr >> 1;
			*nr = vnr + n;
			n = vnr;
		}
		LeftNR = n;
	}
}

void ISoundFile::ProcessAGC(int Count)
{
	static DWORD AGCRecoverCount = 0;
	UINT agc = X86_AGC(MixSoundBuffer, Count, AGC);
	if (agc >= AGC && AGC < AGC_UNITY)
	{
		UINT agctimeout = 44100 >> (AGC_PRECISION - 8);
		AGCRecoverCount += Count;
		if (AGCRecoverCount >= agctimeout)
		{
			AGCRecoverCount = 0;
			AGC++;
		}
	}
	else
	{
		AGC = agc;
		AGCRecoverCount = 0;
	}
}

void ISoundFile::ApplyGlobalVolume(int *SoundBuffer, long TotalSampleCount)
{
	long delta = 0, step = 0;
	if (GlobalVolumeDest != GlobalVolume)
	{
		GlobalVolumeDest = GlobalVolume;
		SamplesToGlobalVolRampDest = VolumeRampSamples;
	}
	if (SamplesToGlobalVolRampDest > 0)
	{
		UINT maxStep;
		long highResGlobalVolumeDest = ((long)GlobalVolumeDest) << VOLUMERAMPPRECISION;
		delta = highResGlobalVolumeDest - HighResRampingGlobalVolume;
		step = delta / ((long)SamplesToGlobalVolRampDest);
		maxStep = max(40, 10000 / VolumeRampSamples + 1);
		while ((UINT)abs(step) > maxStep)
		{
			SamplesToGlobalVolRampDest += VolumeRampSamples;
			step = delta / ((long)SamplesToGlobalVolRampDest);
		}
	}
	for (int pos = 0; pos < TotalSampleCount; pos++)
	{
		if (SamplesToGlobalVolRampDest > 0)
		{
			HighResRampingGlobalVolume += step;
			SoundBuffer[pos] = muldiv(SoundBuffer[pos], HighResRampingGlobalVolume, MAX_GLOBAL_VOLUME << VOLUMERAMPPRECISION);
			SamplesToGlobalVolRampDest--;
		}
		else
			SoundBuffer[pos] = muldiv(SoundBuffer[pos], GlobalVolume, MAX_GLOBAL_VOLUME);
	}
}

UINT ISoundFile::Read(BYTE *Buffer, UINT BuffLen)
{
	ConvertProc Cvt = X86_Convert32To16;
	UINT SampleSize = 2 * p_IF->p_FI->Channels;
	UINT Max = BuffLen / SampleSize, Read, Count;
	UINT SampleCount, Stat = 0;

	MixStat = 0;
	if (Max == 0 || Buffer == NULL || Channels == 0)
		return 0;
	Read = Max;
	if ((SongFlags & SONG_ENDREACHED) != 0)
		goto MixDone;
	while (Read > 0)
	{
		UINT _TotalSampleCount;
		if (BufferCount == 0)
		{
			if ((SongFlags & SONG_FADINGSONG) != 0)
			{
				SongFlags |= SONG_ENDREACHED;
				BufferCount = Read;
			}
			else if (ReadNote() == false)
			{
				if (MaxOrderPosition != 0 && CurrentPattern >= MaxOrderPosition)
				{
					SongFlags |= SONG_ENDREACHED;
					break;
				}
				if (FadeSong(FADESONGDELAY) == false)
				{
					SongFlags |= SONG_ENDREACHED;
					if (Read == Max)
						goto MixDone;
					BufferCount = Read;
				}
			}
		}
		Count = BufferCount;
		if (Count > MIXBUFFERSIZE)
			Count = MIXBUFFERSIZE;
		if (Count > Read)
			Count = Read;
		if (Count == 0)
			break;
		SampleCount = Count;
		ReverbSend = 0;
		X86_StereoFill(MixSoundBuffer, SampleCount, &DryROffsVol, &DryLOffsVol);
		if (OutChannels >= 2)
		{
			SampleCount *= 2;
			MixStat += CreateStereoMix(Count);
			ProcessReverb(Count);
			ProcessStereoDSP(Count);
		}
		else
		{
			MixStat += CreateStereoMix(Count);
			ProcessReverb(Count);
			X86_MonoFromStereo(MixSoundBuffer, Count);
			ProcessMonoDSP(Count);
		}
		if ((SoundSetup & SNDMIX_AGC) != 0)
			ProcessAGC(SampleCount);
		_TotalSampleCount = SampleCount;
		if (OutChannels > 2)
		{
			X86_InterleaveFrontRear(MixSoundBuffer, MixRearBuffer, SampleCount);
			_TotalSampleCount *= 2;
		}
		if (Config->Get_GlobalVolumeAppliesToMaster() == true)
			ApplyGlobalVolume(MixSoundBuffer, _TotalSampleCount);
		Buffer += Cvt(Buffer, MixSoundBuffer, _TotalSampleCount);
		Read -= Count;
		BufferCount -= Count;
		this->TotalSampleCount += Count;
	}
MixDone:
	if (Read != 0)
		memset(Buffer, (BitsPerSample == 8 ? 0x80 : 0), Read * SampleSize);
	if (Stat != 0)
	{
		MixStat += Stat - 1;
		MixStat /= Stat;
	}
	return Max - Read;
}

DWORD FillITBuffer(IT_Intern *p_IF)
{
	DWORD Read = 0;

	Read = p_IF->p_SndFile->Read(p_IF->buffer, 8192);
	Read *= 2 * p_IF->p_FI->Channels;
	if (Read == 0)
		return -2;
	return Read;
}
