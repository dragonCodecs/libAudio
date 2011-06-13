#ifndef __ProTracker_H__
#define __ProTracker_H__ 1

#pragma pack(push, 1)

#define BE2LE(var) (uint16_t)((((uint16_t)(var & 0xFF)) << 8) | (var >> 8))

typedef struct _MODHeader
{
	char Name[20];
	uint8_t nOrders;
	uint8_t RestartPos;
	uint8_t Orders[128];
} MODHeader;

typedef struct _MODSample
{
	char Name[22];
	uint16_t Length;
	uint8_t FineTune;
	uint8_t Volume;
	uint16_t LoopStart;
	uint16_t LoopLen;
} MODSample;

typedef struct _MODCommand
{
	uint8_t Sample;
	uint16_t Period;
	uint16_t Effect;
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
	uint8_t buffer[8192];
	MODHeader *p_Header;
	uint8_t nChannels;
	uint8_t nSamples;
	MODSample *p_Samples;
	uint8_t nPatterns;
	MODPattern *p_Patterns;
	uint8_t **p_PCM;
	void *p_Mixer;
} MOD_Intern;

#pragma pack(pop)

#endif /*__ProTracker_H__*/
