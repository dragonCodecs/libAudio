/*#ifndef __INTERFACES_H__
#define __INTERFACES_H__
#include "mixIT/Interfaces.h"
#endif /*__INTERFACES__H__*/

#ifndef _REVERB_STRUCTS_
#define _REVERB_STRUCTS_
#define ENVIRONMENT_NUMREFLECTIONS		8

typedef struct _SNDMIX_REVERB_PROPERTIES
{
	long Room;
	long RoomHF;
	float DecayTime;
	float DecayHFRatio;
	long Reflections;
	float ReflectionsDelay;
	long Reverb;
	float ReverbDelay;
	float Diffusion;
	float Density;
} SNDMIX_REVERB_PROPERTIES;

typedef struct _SNDMIX_RVBPRESET
{
	SNDMIX_REVERB_PROPERTIES Preset;
	const char *Name;
} SNDMIX_RVBPRESET;

typedef struct _ENVIRONMENTREFLECTION
{
	short GainLL, GainRR, GainLR, GainRL;
	ULONG Delay;
} ENVIRONMENTREFLECTION;

typedef struct _ENVIRONMENTREVERB
{
	long ReverbLevel;
	long ReflectionsLevel;
	long RoomHF;
	ULONG ReverbDecay;
	long PreDiffusion;
	long TankDiffusion;
	ULONG ReverbDelay;
	float ReverbDamping;
	long ReverbDecaySamples;
	ENVIRONMENTREFLECTION Reflections[ENVIRONMENT_NUMREFLECTIONS];
} ENVIRONMENTREVERB;
#endif /*_REVERB_STRUCTS_*/

class IPlayConfig;
class ISoundFile;

//typedef bool (__CDECL__ *PluginCreateProc)(MixPlugin *);

#define MAX_PATTERN_ROWS		1024
#define MAX_ORDERS				256
#define MAX_PATTERNS			240
#define MAX_SAMPLES				4000
#define MAX_INSTRUMENTS			256
#define MAX_CHANNELS			256
#define MAX_BASECHANNELS		128
#define MAX_ENVPOINTS			32
#define MIN_PERIOD				0x0020
#define MAX_PERIOD				0xFFFF
#define MAX_PLUGPRESETS			1000
#define MAX_GLOBAL_VOLUME		256

#pragma pack(push, 1)

// Impulse File Header
typedef struct _ITFileHeader
{
	CHAR id[4]; // "IMPM"
	CHAR songname[26];
	WORD reserved1; // 0x1004
	WORD ordnum;
	WORD insnum;
	WORD smpnum;
	WORD patnum;
	WORD cwtv;
	WORD cmwt;
	WORD flags;
	WORD special;
	BYTE globalvol;
	BYTE mv;
	BYTE speed;
	BYTE tempo;
	BYTE sep;
	BYTE zero;
	WORD msglength;
	DWORD msgoffset;
	DWORD reserved2;
	BYTE chnpan[64];
	BYTE chnvol[64];
} ITFileHeader;

// Impulse Envelope Format
typedef struct _ITEnvelope
{
	BYTE flags;
	BYTE num;
	BYTE lpb;
	BYTE lpe;
	BYTE slb;
	BYTE sle;
	BYTE data[75];
	BYTE reserved;
} ITEnvelope;

// Old Impulse Instrument Format (cmwt < 0x200)
typedef struct _ITOldInstrument
{
	CHAR id[4]; // "IMPI"
	CHAR filename[12]; // DOS file name
	BYTE zero;
	BYTE flags;
	BYTE vls;
	BYTE vle;
	BYTE sls;
	BYTE sle;
	WORD reserved1;
	WORD fadeout;
	BYTE nna;
	BYTE dnc;
	WORD trkvers;
	BYTE nos;
	BYTE reserved2;
	CHAR name[26];
	WORD reserved3[3];
	BYTE keyboard[240];
	BYTE volenv[200];
	BYTE nodes[50];
} ITOldInstrument;

// Impulse Instrument Format
typedef struct _ITInstrument
{
	CHAR id[4]; // IMPI
	CHAR filename[12];
	BYTE zero;
	BYTE nna;
	BYTE dct;
	BYTE dca;
	WORD fadeout;
	signed char pps;
	BYTE ppc;
	BYTE gbv;
	BYTE dfp;
	BYTE rv;
	BYTE rp;
	WORD trkvers;
	BYTE nos;
	BYTE reserved1;
	CHAR name[26];
	BYTE ifc;
	BYTE ifr;
	BYTE mch;
	BYTE mpr;
	WORD mbank;
	BYTE keyboard[240];
	ITEnvelope volenv;
	ITEnvelope panenv;
	ITEnvelope pitchenv;
	BYTE dummy[4]; // was 7, but IT v2.17 saves 554 bytes
} ITInstrument;

// IT Sample Format
typedef struct _ITSampleStruct
{
	CHAR id[4]; // IMPS
	CHAR filename[12];
	BYTE zero;
	BYTE gvl;
	BYTE flags;
	BYTE vol;
	CHAR name[26];
	BYTE cvt;
	BYTE dfp;
	DWORD length;
	DWORD loopbegin;
	DWORD loopend;
	DWORD C5Speed;
	DWORD susloopbegin;
	DWORD susloopend;
	DWORD samplepointer;
	BYTE vis;
	BYTE vid;
	BYTE vir;
	BYTE vit;
} ITSampleStruct;

typedef struct _Command
{
	BYTE note;
	BYTE instr;
	BYTE volcmd;
	BYTE command;
	BYTE vol;
	BYTE param;
} Command;

typedef struct _Patern
{
	USHORT PatLen;
	USHORT nRows;
	Command *p_Commands;
} Patern;

typedef struct _SampleData
{
	char BPS;
	BYTE *PCM;
	int length;
} SampleData;

typedef struct _Names
{
	UINT nNames;
	BYTE **p_Names;
} Names;

typedef struct _MidiConfig
{
	char MidiGlb[288];
	char MidiSFXExt[512];
	char MidiZXXExt[4096];
} MidiConfig;

typedef struct _IT_Intern
{
	FILE *f_IT;
	FileInfo *p_FI;
	Playback *p_Playback;
	BYTE buffer[8192];
	int nChannels;
	ITFileHeader *p_Head;
	UINT *p_InstOffsets;
	ITOldInstrument *p_OldIns;
	ITInstrument *p_Ins;
	UINT *p_SampOffsets;
	ITSampleStruct *p_Samp;
	SampleData *p_Samples;
	BYTE *p_PaternOrder;
	UINT *p_PaternOffsets;
	Patern *p_Paterns;
	Names ChanName;
	Names PatName;
	ISoundFile *p_SndFile;
	MidiConfig *p_MidiCfg;
} IT_Intern;

typedef struct _Channel
{
	BYTE *CurrentSample;
	DWORD Pos;
	DWORD PosLo;
	long Inc;
	long RightVol;
	long LeftVol;
	long RightRamp;
	long LeftRamp;
	DWORD Length;
	DWORD Flags;
	DWORD LoopStart;
	DWORD LoopEnd;
	long RampRightVol;
	long RampLeftVol;
	long Filter_Y1, Filter_Y2, Filter_Y3, Filter_Y4;
	long Filter_A0, Filter_B0, Filter_B1, Filter_HP;
	long ROfs, LOfs;
	long RampLength;
	BYTE *Sample;
	long NewRightVol, NewLeftVol;
	long RealVolume, RealPan;
	long Volume, Pan, FadeOutVol;
	long Period, C4Speed, PortamentoDest;
	ITInstrument *Header;
	ITSampleStruct *Instrument;
	DWORD VolEnvPosition, PanEnvPosition, PitchEnvPosition;
	DWORD MasterChn;
	long GlobalVol, InsVol;
	long FineTune;
	long PortamentoSlide, AutoVibDepth;
	UINT AutoVibPos, VibratoPos, TremoloPos, PanbrelloPos;
	long VolSwing, PanSwing;
	long CutSwing, ResSwing;
	BYTE Note, NNA;
	BYTE NewNote, NewInst, Command, Arpeggio;
	BYTE OldVolumeSlide, OldFineVolUpDown;
	BYTE OldPortaUpDown, OldFinePortaUpDown;
	BYTE OldPanSlide, OldChnVolSlide;
	BYTE VibratoType, VibratoSpeed, VibratoDepth;
	BYTE TremoloType, TremoloSpeed, TremoloDepth;
	BYTE PanbrelloType, PanbrelloSpeed, PanbrelloDepth;
	BYTE OldCmdEx, OldVolParam, OldTempo;
	BYTE OldOffset, OldHiOffset;
	BYTE CutOff, Resonance;
	BYTE RetrigCount, RetrigParam;
	BYTE TremorCount, TremorParam;
	BYTE PaternLoop, PaternLoopCount;
	BYTE RowNote, RowInstr;
	BYTE RowVolCmd, RowVolume;
	BYTE RowCommand;
	UINT RowParam;
	BYTE ActiveMacro, FilterMode;
	float PlugParamValueStep;
	float PlugInitialParamValue;
} Channel;

typedef struct _ChannelSettings
{
	UINT Pan;
	UINT Volume;
	DWORD Flags;
	char *Name;
} ChannelSettings;

class ISoundFile
{
	static UINT XBassDepth, XBassRange;
	static UINT StereoSeparation, VolumeRampSamples, AGC;
	static UINT MaxMixChannels;
	static UINT ReverbDepth, ReverbType;
	static DWORD SoundSetup, SysInfo, OutChannels, BitsPerSample, Quality;
	static UINT ProLogicDepth, ProLogicDelay;
	IT_Intern *p_IF;
	DWORD SongFlags;
	UINT Channels, Instruments;
	UINT BufferCount, TickCount, PatternDelay, FrameDelay;
	UINT Row, NextRow, TotalSampleCount;
	UINT MusicSpeed, MusicTempo;
	UINT TotalCount, MaxOrderPosition;
	UINT Pattern, CurrentPattern, NextPattern, RestartPos, SeqOverride;
	bool PatternTransitionOccurred;
	BYTE *Order, TempoMode, PlugMixMode;
	Patern *Patterns;
	Channel Chns[MAX_CHANNELS];
	ChannelSettings ChnSettings[MAX_BASECHANNELS];
	long MinPeriod, MaxPeriod, RepeatCount;
	ITSampleStruct *Ins;
	ITInstrument *Headers[MAX_INSTRUMENTS];
	UINT GlobalVolume, OldGlbVolSlide;
	MidiConfig MidiCfg;
	DWORD GlobalFadeSamples, GlobalFadeMaxSamples;
	double BufferDiff;
	UINT RowsPerBeat, SamplesPerTick;
	UINT MasterVolume, SongPreAmp, SamplesToGlobalVolRampDest;
	UINT GlobalVolumeDest;
	long HighResRampingGlobalVolume;
	UINT MixChannels, MixStat, FreqFactor;
	UINT ChnMix[MAX_CHANNELS];
	IPlayConfig *Config;

	void ResetMidiCfg();
	DWORD CutOffToFrequency(UINT CutOff, int flt_modifier);
	void SetupChannelFilter(Channel *chn, bool Reset, int flt_modifier = 256);
	bool MuteChannel(UINT nChn, bool Mute);
	bool IsChannelMuted(UINT nChn);
	void HandlePatternTransitionEvents();
	UINT GetPeriodFromNote(UINT note, int FineTune, UINT C4Speed);
	UINT GetNoteFromPeriod(UINT period);
	UINT GetFreqFromPeriod(UINT period, UINT C4Speed, int PeriodFrac);
	void KeyOff(UINT nChn);
	UINT GetNNAChannel(UINT nChn);
	void CheckNNA(UINT nChn, BYTE instr, BYTE note, bool ForceCut);
	void InstrumentChange(Channel *chn, UINT instr, bool Porta = false, bool UpdVol = true, bool ResetEnv = true);
	void ProcessMidiOut(UINT nChn, Channel *chn);
	int PatternLoop(Channel *chn, UINT param);
	void NoteChange(UINT nChn, int note, bool Porta = false, bool ResetEnv = true, bool Manual = false);
	void GlobalVolSlide(UINT param);
	DWORD IsSongFinished(UINT StartOrder, UINT StartRow);
	bool GlobalFadeSong(UINT msec);
	void SetSpeed(UINT param);
	void SetTempo(UINT param);
	void FineVolumeUp(Channel *chn, UINT param);
	void FineVolumeDown(Channel *chn, UINT param);
	void VolumeSlide(Channel *chn, UINT param);
	void Vibrato(Channel *chn, UINT param);
	void FineVibrato(Channel *chn, UINT param);
	void PanningSlide(Channel *chn, UINT param);
	void DoFreqSlide(Channel *chn, long FreqSlide);
	void Tremolo(Channel *chn, UINT param);
	void ChannelVolSlide(Channel *chn, UINT param);
	void Panbrello(Channel *chn, UINT param);
	void ExtraFinePortamentoUp(Channel *chn, UINT param);
	void ExtraFinePortamentoDown(Channel *chn, UINT param);
	void FinePortamentoUp(Channel *chn, UINT param);
	void FinePortamentoDown(Channel *chn, UINT param);
	void PortamentoUp(Channel *chn, UINT param);
	void PortamentoDown(Channel *chn, UINT param);
	void TonePortamento(Channel *chn, UINT param);
	void SampleOffset(UINT nChn, UINT param, bool Porta);
	void RetrigNote(UINT nChn, UINT param, UINT offset = 0);
	void NoteCut(UINT nChn, UINT Tick);
	void ExtendedChannelEffect(Channel *chn, UINT param);
	void ExtendedS3MCommands(UINT nChn, UINT param);
	void ProcessSmoothMidiMacro(UINT nChn, char *MidiMacro, UINT param);
	void ProcessMidiMacro(UINT nChn, char *MidiMacro, UINT param);
	bool ProcessEffects();
	bool ProcessRow();
	bool ReadNote();
	bool FadeSong(UINT msec);
	UINT GetResamplingFlag(Channel *chn);
	static long __FASTCALL__ GetSampleCount(Channel *chn, long Samples);
	void ProcessReverb(UINT Samples);
	UINT CreateStereoMix(int count);
	void StereoMixToFloat(int *Src, float *Out1, float *Out2, UINT Count);
	void FloatToStereoMix(float *In1, float *In2, int *Out, UINT Count);
	void ProcessStereoSurround(int Count);
	void ProcessQuadSurround(int Count);
	void ProcessStereoDSP(int Count);
	void ProcessMonoDSP(int Count);
	void ProcessAGC(int Count);
	void ApplyGlobalVolume(int *SoundBuffer, long TotalSampleCount);
	static void getsinc(short *Sinc, double Beta, double LowPassFactor);
	static double Zero(double y);
	long OnePoleLowPassCoef(long Scale, float g, float c, float s);
	long BToLinear(long Scale, long dB);
	float BToLinear(long dB);
	inline void I3dl2_to_Generic(SNDMIX_REVERB_PROPERTIES *Reverb, ENVIRONMENTREVERB *Rvb, float OutputFreq, long MinRefDelay, long MaxRefDelay, long MinRvbDelay, long MaxRvbDelay, long TankLength);
	DWORD InitSysInfo();
	void UpdateAudioParameters(bool Reset);
	bool SetResamplingMode(UINT Mode);
	void SetDspEffects(bool Surround, bool Reverb, bool MegaBass, bool NR, bool EQ);
	void SetCurrentPos(UINT Pos);
	inline BYTE SampleToChannelFlags(const ITSampleStruct * const smp);
	inline BYTE SampleToMixVibType(const ITSampleStruct * const smp);
	inline BYTE SampleToMixVibDepth(const ITSampleStruct * const smp);
	inline BYTE SampleToMixVibSweep(const ITSampleStruct * const smp);

public:
	ISoundFile(IT_Intern *p_ITFile);
	UINT Read(BYTE *Buffer, UINT BuffLen);
	bool InitPlayer(bool Reset);
	void SndMixInitializeTables();
	void InitializeDSP(bool Reset);
	void InitializeReverb(bool Reset);
};

#pragma pack(pop)

DWORD FillITBuffer(IT_Intern *p_IF);
