#ifndef __ProTracker_H__
#define __ProTracker_H__ 1

#pragma pack(push, 1)

#define BE2LE(var) (WORD)((((WORD)(var & 0xFF)) << 8) | (var >> 8))

typedef struct _MODHeader
{
	char Name[20];
	BYTE nOrders;
	BYTE RestartPos;
	BYTE Orders[128];
} MODHeader;

typedef struct _MODSample
{
	char Name[22];
	WORD Length;
	BYTE FineTune;
	BYTE Volume;
	WORD LoopStart;
	WORD LoopLen;
} MODSample;

typedef struct _MODCommand
{
	BYTE Sample;
	WORD Period;
	WORD Effect;
} MODCommand;

typedef struct _MODPattern
{
	MODCommand (*Commands)[64]; // Pointer to blocks of 64 commands
} MODPattern;

typedef struct _MOD_Intern
{
	FILE *f_MOD;
	FileInfo *p_FI;
	Playback *p_Playback;
	BYTE buffer[8192];
	MODHeader *p_Header;
	BYTE nChannels;
	BYTE nSamples;
	MODSample *p_Samples;
	BYTE nPatterns;
	MODPattern *p_Patterns;
	BYTE **p_PCM;
	void *p_Mixer;
} MOD_Intern;

#pragma pack(pop)

#endif /*__ProTracker_H__*/
