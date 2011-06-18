#ifndef __ModuleMixer_H__
#define __ModuleMixer_H__ 1

#define MIXBUFFERSIZE		512

#pragma pack(push, 1)

typedef struct _int16dot16_t
{
	uint16_t Lo;
	int16_t Hi;
} int16dot16_t;

typedef union _int16dot16
{
	int32_t iValue;
	int16dot16_t Value;
} int16dot16;

typedef struct _Channel
{
	uint8_t *Sample, *NewSample;
	uint8_t Note, NewNote, NewSamp;
	uint32_t LoopStart, LoopEnd, Length;
	ModuleSample *Samp;
	uint8_t RowNote, RowSample, Volume;
	uint8_t FineTune, Flags, Pan;
	uint32_t Period, Pos, PosLo;
	int16dot16 Increment;
	int PortamentoDest;
	uint8_t Arpeggio, LeftVol, RightVol, RampLength;
	uint8_t NewLeftVol, NewRightVol;
	uint16_t RowEffect, PortamentoSlide;
	short LeftRamp, RightRamp;
	int Filter_Y1, Filter_Y2, Filter_Y3, Filter_Y4;
	int Filter_A0, Filter_B0, Filter_B1, Filter_HP;
	uint8_t TremoloDepth, TremoloSpeed, TremoloPos, TremoloType;
	uint8_t VibratoDepth, VibratoSpeed, VibratoPos, VibratoType;
	int DCOffsR, DCOffsL;
} Channel;

typedef struct _MixerState
{
	uint32_t MixRate, MixOutChannels, MixBitsPerSample;
	uint32_t Channels, Samples;
	uint32_t TickCount, BufferCount;
	uint32_t Row, NextRow;
	uint32_t MusicSpeed, MusicTempo;
	uint32_t Pattern, CurrentPattern, NextPattern, RestartPos;
	uint8_t *Orders, MaxOrder, PatternDelay;
	ModuleSample **Samp;
	uint8_t **SamplePCM;
	uint32_t RowsPerBeat, SamplesPerTick;
	Channel *Chns;
	uint32_t MixChannels, *ChnMix;
	ModulePattern **Patterns;
	uint8_t SongFlags, SoundSetup;
	uint8_t PatternLoopCount, PatternLoopStart;
	int MixBuffer[MIXBUFFERSIZE * 2];
	int DCOffsR, DCOffsL;
} MixerState;

#pragma pack(pop)

#endif /*__ModuleMixer_H__*/
