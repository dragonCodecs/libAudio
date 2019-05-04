#ifndef __GenericModule_H__
#define __GenericModule_H__

#include "../libAudio.hxx"
#include "../fixedVector.hxx"
#include "../managedPtr.hxx"
#include <array>

/***************************\
|* ----=== WARNING ===---- *|
|*   This set of classes   *|
|* makes heavy use of the  *|
|*  friend keyword - make  *|
|* sure you do not modify  *|
|*   the relationships!!   *|
\***************************/

class ModuleFile;
class Channel;
class ModuleSample;
class ModulePattern;

using stringPtr_t = std::unique_ptr<char []>;

template<typename T> struct moduleIntern : T
{
	FILE *f_Module;
	uint8_t buffer[8192];

	moduleIntern() noexcept : T(), f_Module(nullptr) { }
};

struct aonIntern { modAON_t inner; };
typedef moduleIntern<aonIntern> AON_Intern;

#include "effects.h"

#define MIXBUFFERSIZE		512

#define MODULE_MOD		1
#define MODULE_S3M		2
#define MODULE_STM		3
#define MODULE_AON		4
#define MODULE_FC1x		5
#define MODULE_IT		6

#define E_BAD_MOD		1
#define E_BAD_S3M		2
#define E_BAD_STM		3
#define E_BAD_AON		4
#define E_BAD_FC1x		5
#define E_BAD_IT		6

#define FILE_FLAGS_AMIGA_SLIDES		0x01
#define FILE_FLAGS_AMIGA_LIMITS		0x02
#define FILE_FLAGS_FAST_SLIDES		0x04
#define FILE_FLAGS_LINEAR_SLIDES	0x08

#define SAMPLE_FLAGS_LOOP		0x01
#define SAMPLE_FLAGS_STEREO		0x02
#define SAMPLE_FLAGS_16BIT		0x04
#define SAMPLE_FLAGS_LPINGPONG	0x08
#define SAMPLE_FLAGS_LREVERSE	0x10

#define ENVELOPE_VOLUME		0
#define ENVELOPE_PANNING	1
#define ENVELOPE_PITCH		2

class ModuleLoaderError
{
private:
	const uint32_t Error;

public:
	ModuleLoaderError(const uint32_t Error);
	const char *GetError() const noexcept { return error(); }
	const char *error() const noexcept;
};

class ModuleAllocator
{
public:
	void *operator new(const size_t s);
	void *operator new[](const size_t s);
	void *operator new(const size_t s, const std::nothrow_t &);
	void *operator new[](const size_t s, const std::nothrow_t &);
};

class ModuleHeader final : public ModuleAllocator
{
private:
	// Common fields
	std::unique_ptr<char []> Name;
	std::unique_ptr<char []> Remark;
	uint16_t nOrders;
	uint16_t nSamples;
	uint16_t nInstruments;
	uint16_t nPatterns;
	std::unique_ptr<uint8_t []> Orders;
	std::unique_ptr<uint16_t []> Panning;
	uint16_t Flags;
	uint16_t CreationVersion;
	uint16_t FormatVersion;
	managedPtr_t<void> InstrumentPtrs;
	// Slightly badly named
	// SamplePtrs = pointers to where the sample *descriptors* are
	managedPtr_t<void> SamplePtrs;
	// PatternPtrs = pointers to where the compressed pattern data is
	managedPtr_t<void> PatternPtrs;

	// Fields specific to certain formats

	// MOD/AON
	uint8_t RestartPos;

	// S3M
	uint8_t Type;
	uint8_t GlobalVolume;
	uint8_t InitialSpeed;
	uint8_t InitialTempo;
	uint8_t MasterVolume;
	uint8_t ChannelSettings[32];

	// AON
	std::unique_ptr<char []> Author;
	char ArpTable[16][4];

#ifdef ENABLE_FC1x
	// FC1x
	uint32_t SeqLength;
	uint32_t PatternOffs;
	uint32_t PatLength;
	uint32_t FrequenciesOffs;
	uint32_t FrequenciesLength;
	uint32_t VolumeOffs;
	uint32_t VolumeLength;
	uint32_t SampleOffs;
	uint32_t SampleLength;
#endif

	// IT
	uint8_t Separation;
	uint32_t MessageOffs;
	std::array<uint8_t, 64> Volumes;
	std::array<bool, 64> PanSurround;

private:
	uint8_t nChannels;
	friend class ModuleFile;
	friend class Channel;
	ModuleHeader();

public:
	ModuleHeader(const modMOD_t &file);
	ModuleHeader(const modS3M_t &file);
	ModuleHeader(const modSTM_t &file);
	ModuleHeader(const modAON_t &file);
#ifdef ENABLE_FC1x
	ModuleHeader(const modFC1x_t &file);
#endif
	ModuleHeader(const modIT_t &file);
	~ModuleHeader() noexcept = default;
};

class ModuleSample : public ModuleAllocator
{
protected:
	const uint8_t _type;

private:
	uint32_t _id;

protected:
	ModuleSample(const uint32_t id, const uint8_t type) noexcept : _type(type), _id(id) { }
	void ResetID(const uint32_t id) noexcept { _id = id; }

public:
	static ModuleSample *LoadSample(const modMOD_t &file, const uint32_t i);
	static ModuleSample *LoadSample(const modS3M_t &file, const uint32_t i);
	static ModuleSample *LoadSample(const modSTM_t &file, const uint32_t i);
	static ModuleSample *LoadSample(AON_Intern *p_AF, uint32_t i, char *Name, uint32_t *pcmLengths);
	static ModuleSample *LoadSample(const modIT_t &file, const uint32_t i);

	virtual ~ModuleSample() noexcept = default;
	uint8_t GetType() const noexcept { return _type; }
	uint32_t id() const noexcept { return _id; }
	virtual uint32_t GetLength() = 0;
	virtual uint32_t GetLoopStart() = 0;
	virtual uint32_t GetLoopEnd() = 0;
	virtual uint8_t GetFineTune() = 0;
	virtual uint32_t GetC4Speed() = 0;
	virtual uint8_t GetVolume() = 0;
	virtual uint8_t GetVibratoSpeed() = 0;
	virtual uint8_t GetVibratoDepth() = 0;
	virtual uint8_t GetVibratoType() = 0;
	virtual uint8_t GetVibratoRate() = 0;
	virtual bool Get16Bit() = 0;
	virtual bool GetStereo() = 0;
	virtual bool GetLooped() = 0;
	virtual bool GetBidiLoop() = 0;
};

class ModuleSampleNative final : public ModuleSample
{
private:
	std::unique_ptr<char []> Name;
	uint32_t Length;
	uint8_t FineTune;
	uint8_t Volume;
	uint8_t InstrVol;
	uint32_t LoopStart;
	uint32_t LoopEnd;

private:
	std::unique_ptr<char []> FileName;
	uint32_t SamplePos; // actually 24-bit..
	uint8_t Packing;
	uint8_t Flags, SampleFlags;
	uint32_t C4Speed;
	uint8_t DefaultPan;
	uint8_t VibratoSpeed;
	uint8_t VibratoDepth;
	uint8_t VibratoType;
	uint8_t VibratoRate;
	uint32_t SusLoopBegin;
	uint32_t SusLoopEnd;
	friend class ModuleFile;

public:
	ModuleSampleNative(const modMOD_t &file, const uint32_t i);
	ModuleSampleNative(const modS3M_t &file, const uint32_t i, const uint8_t Type);
	ModuleSampleNative(const modSTM_t &file, const uint32_t i);
	ModuleSampleNative(AON_Intern *p_AF, uint32_t i, char *Name, uint32_t *pcmLengths);
	ModuleSampleNative(const modIT_t &file, const uint32_t i);
	~ModuleSampleNative() = default;

	uint32_t GetLength() override final { return Length; }
	uint32_t GetLoopStart() override final { return LoopStart; }
	uint32_t GetLoopEnd() override final { return LoopEnd; }
	uint8_t GetFineTune() override final { return FineTune; }
	uint32_t GetC4Speed() override final { return C4Speed; }
	uint8_t GetVolume() override final { return Volume << 1; }
	uint8_t GetVibratoSpeed() override final { return VibratoSpeed; }
	uint8_t GetVibratoDepth() override final { return VibratoDepth; }
	uint8_t GetVibratoType() override final { return VibratoType; }
	uint8_t GetVibratoRate() override final { return VibratoRate; }
	bool Get16Bit() override final;
	bool GetStereo() override final;
	bool GetLooped() override final;
	bool GetBidiLoop() override final;
};

class ModuleSampleAdlib : public ModuleSample
{
private:
	char *FileName;
	uint8_t D00;
	uint8_t D01;
	uint8_t D02;
	uint8_t D03;
	uint8_t D04;
	uint8_t D05;
	uint8_t D06;
	uint8_t D07;
	uint8_t D08;
	uint8_t D09;
	uint8_t D0A;
	uint8_t D0B;
	uint8_t Volume;
	uint8_t DONTKNOW;
	uint32_t C4Speed;
	char *Name;

public:
	ModuleSampleAdlib(const modS3M_t &file, const uint32_t i, const uint8_t Type);
	~ModuleSampleAdlib();

	uint32_t GetLength();
	uint32_t GetLoopStart();
	uint32_t GetLoopEnd();
	uint8_t GetFineTune();
	uint32_t GetC4Speed();
	uint8_t GetVolume();
	uint8_t GetVibratoSpeed();
	uint8_t GetVibratoDepth();
	uint8_t GetVibratoType();
	uint8_t GetVibratoRate();
	bool Get16Bit();
	bool GetStereo();
	bool GetLooped();
	bool GetBidiLoop();
};

#pragma pack(push, 1)
struct envelopeNode_t final
{
	uint8_t Value;
	uint16_t Tick;
};
#pragma pack(pop)

class ModuleEnvelope : public ModuleAllocator
{
private:
	uint8_t Type;
	uint8_t Flags;
	uint8_t nNodes;
	uint8_t LoopBegin;
	uint8_t LoopEnd;
	uint8_t SusLoopBegin;
	uint8_t SusLoopEnd;
	std::array<envelopeNode_t, 25> Nodes;

public:
	ModuleEnvelope(const modIT_t &file, const uint8_t env);
	ModuleEnvelope(const modIT_t &file, const uint8_t Flags, const uint8_t LoopBegin,
		const uint8_t LoopEnd, const uint8_t SusLoopBegin, const uint8_t SusLoopEnd) noexcept;
	uint8_t Apply(const uint16_t Tick) noexcept;
	bool GetEnabled() const noexcept;
	bool GetLooped() const noexcept;
	bool GetSustained() const noexcept;
	bool HasNodes() const noexcept;
	bool IsAtEnd(const uint16_t Tick) const noexcept;
	bool IsZeroLoop() const noexcept;
	uint16_t GetLoopEnd() const noexcept;
	uint16_t GetLoopBegin() const noexcept;
	uint16_t GetSustainEnd() const noexcept;
	uint16_t GetSustainBegin() const noexcept;
	uint16_t GetLastTick() const noexcept;
};

class ModuleInstrument : public ModuleAllocator
{
private:
	const uint32_t _id;

protected:
	ModuleInstrument(const uint32_t id) noexcept : _id(id) { }

public:
	static ModuleInstrument *LoadInstrument(const modIT_t &file, const uint32_t i, const uint16_t FormatVersion);

	virtual ~ModuleInstrument() noexcept { }
	virtual uint8_t Map(uint8_t Note) noexcept = 0;
	virtual uint16_t GetFadeOut() const noexcept = 0;
	virtual bool GetEnvEnabled(uint8_t env) const = 0;
	virtual bool GetEnvLooped(uint8_t env) const = 0;
	virtual ModuleEnvelope *GetEnvelope(uint8_t env) const noexcept = 0;
	virtual bool IsPanned() const noexcept = 0;
	virtual bool HasVolume() const noexcept = 0;
	virtual uint8_t GetPanning() const noexcept = 0;
	virtual uint8_t GetVolume() const noexcept = 0;
	virtual uint8_t GetNNA() const noexcept = 0;
	virtual uint8_t GetDCT() const noexcept = 0;
	virtual uint8_t GetDNA() const noexcept = 0;
};

class ModuleOldInstrument final : public ModuleInstrument
{
private:
	std::unique_ptr<char []> FileName;
	uint8_t Flags;
	uint16_t FadeOut;
	uint8_t NNA;
	uint8_t DNC;
	uint16_t TrackerVersion;
	uint8_t nSamples;
	std::unique_ptr<char []> Name;
	uint8_t SampleMapping[240];
	std::unique_ptr<ModuleEnvelope> Envelope;

public:
	ModuleOldInstrument(const modIT_t &file, const uint32_t i);
	~ModuleOldInstrument() = default;

	uint8_t Map(uint8_t Note) noexcept override final;
	uint16_t GetFadeOut() const noexcept override final;
	bool GetEnvEnabled(uint8_t env) const noexcept override final;
	bool GetEnvLooped(uint8_t env) const noexcept override final;
	ModuleEnvelope *GetEnvelope(uint8_t env) const noexcept override final;
	bool IsPanned() const noexcept override final;
	bool HasVolume() const noexcept override final;
	uint8_t GetPanning() const noexcept override final;
	uint8_t GetVolume() const noexcept override final;
	uint8_t GetNNA() const noexcept override final;
	uint8_t GetDCT() const noexcept override final;
	uint8_t GetDNA() const noexcept override final;
};

class ModuleNewInstrument final : public ModuleInstrument
{
private:
	std::unique_ptr<char []> FileName;
	uint8_t NNA;
	uint8_t DCT;
	uint8_t DNA;
	uint16_t FadeOut;
	uint8_t PPS;
	uint8_t PPC;
	uint8_t Volume;
	uint8_t Panning;
	uint8_t RandVolume;
	uint8_t RandPanning;
	uint16_t TrackerVersion;
	uint8_t nSamples;
	std::unique_ptr<char []> Name;
	uint8_t SampleMapping[240];
	std::array<std::unique_ptr<ModuleEnvelope>, 3> Envelopes;

public:
	ModuleNewInstrument(const modIT_t &file, const uint32_t i);
	~ModuleNewInstrument() = default;

	uint8_t Map(uint8_t Note) noexcept override final;
	uint16_t GetFadeOut() const noexcept override final;
	bool GetEnvEnabled(uint8_t env) const override final;
	bool GetEnvLooped(uint8_t env) const override final;
	ModuleEnvelope *GetEnvelope(uint8_t env) const noexcept override final;
	bool IsPanned() const noexcept override final;
	bool HasVolume() const noexcept override final;
	uint8_t GetPanning() const noexcept override final;
	uint8_t GetVolume() const noexcept override final;
	uint8_t GetNNA() const noexcept override final;
	uint8_t GetDCT() const noexcept override final;
	uint8_t GetDNA() const noexcept override final;
};

class ModuleCommand : public ModuleAllocator
{
private:
	uint8_t Sample;
	uint8_t Note;
	uint8_t VolEffect;
	uint8_t VolParam;
	uint8_t Effect;
	uint8_t Param;
	uint8_t ArpIndex;

	inline uint8_t MODPeriodToNoteIndex(const uint16_t Period) noexcept;
	void TranslateMODEffect(const uint8_t Effect, const uint8_t Param) noexcept;
	friend class ModuleFile;
	friend class Channel;

public:
	void SetSample(const uint8_t _sample) noexcept { Sample = _sample; }
	void SetVolume(const uint8_t Volume) noexcept;
	void SetMODData(const std::array<uint8_t, 4> &Data) noexcept;
	void SetS3MNote(uint8_t Note, uint8_t Sample);
	void SetS3MVolume(uint8_t Volume);
	void SetS3MEffect(uint8_t Effect, uint8_t Param);
	void SetSTMNote(uint8_t Note);
	void SetSTMEffect(uint8_t Effect, uint8_t Param);
	void SetAONNote(uint8_t Note);
	void SetAONArpIndex(uint8_t Index);
	void SetAONEffect(uint8_t Effect, uint8_t Param);
	void SetITRepVal(const uint8_t channelMask, const ModuleCommand &lastCommand) noexcept;
	void SetITNote(uint8_t note) noexcept;
	void SetITVolume(const uint8_t volume) noexcept;
	void SetITEffect(const uint8_t Effect, const uint8_t Param);
};

class ModulePattern final : public ModuleAllocator
{
public:
	using commandPtr_t = std::unique_ptr<ModuleCommand []>;

private:
	const uint32_t Channels;
	fixedVector_t<commandPtr_t> _commands;
	uint16_t _rows;

	ModulePattern(const uint32_t _channels, const uint16_t rows, const uint32_t type);

public:
	ModulePattern(const modMOD_t &file, const uint32_t nChannels);
	ModulePattern(const modS3M_t &file, const uint32_t nChannels);
	ModulePattern(const modSTM_t &file);
	ModulePattern(AON_Intern *p_AF, uint32_t nChannels);
	ModulePattern(const modIT_t &file, const uint32_t nChannels);

	const fixedVector_t<commandPtr_t> &commands() const { return _commands; }
	uint16_t rows() const noexcept { return _rows; }
};

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

class Channel : public ModuleAllocator
{
public:
	uint8_t *SampleData, *NewSampleData;
	uint8_t Note, RampLength;
	uint8_t NewNote, NewSample;
	uint32_t LoopStart, LoopEnd, Length;
	uint8_t RawVolume, Volume, VolumeSlide, FineVolumeSlide;
	uint8_t ChannelVolume, ChannelVolumeSlide;
	uint8_t AutoVibratoPos, reserved;
	ModuleSample *Sample;
	ModuleInstrument *Instrument;
	uint8_t FineTune, PanningSlide;
	uint16_t RawPanning, Panning;
	uint8_t RowNote, RowSample, RowVolEffect;
	uint8_t RowEffect, RowVolParam, RowParam;
	uint16_t PortamentoSlide, Flags;
	uint32_t Period, C4Speed;
	uint32_t Pos, PosLo, StartTick;
	int16dot16 Increment;
	uint32_t PortamentoDest;
	uint8_t Portamento, Arpeggio;
	uint8_t Tremor, TremorCount;
	uint8_t LeftVol, RightVol;
	uint8_t NewLeftVol, NewRightVol;
	short LeftRamp, RightRamp;
	int Filter_Y1, Filter_Y2, Filter_Y3, Filter_Y4;
	int Filter_A0, Filter_B0, Filter_B1, Filter_HP;
	uint8_t TremoloDepth, TremoloSpeed, TremoloPos, TremoloType;
	uint8_t VibratoDepth, VibratoSpeed, VibratoPos, VibratoType;
	uint8_t PanbrelloDepth, PanbrelloSpeed, PanbrelloPos, PanbrelloType;
	uint16_t EnvVolumePos, EnvPanningPos, EnvPitchPos, FadeOutVol;
	int DCOffsL, DCOffsR;

public:
	Channel();
	void SetData(ModuleCommand *Command, ModuleHeader *p_Header);

	// Channel effects
	void NoteOff();
	void NoteCut(bool Triggered);
	void Vibrato(uint8_t param, uint8_t Multiplier);
	void Panbrello(uint8_t param);
	void ChannelEffect(uint8_t param);

	// Channel mixing processing
	uint32_t GetSampleCount(uint32_t Samples);
};

class ModuleFile : public ModuleAllocator
{
private:
	uint8_t ModuleType;
	ModuleHeader *p_Header;
	ModuleSample **p_Samples;
	ModulePattern **p_Patterns;
	ModuleInstrument **p_Instruments;
	uint8_t **p_PCM;
	uint32_t *LengthPCM;
	uint32_t nPCM;

	// Mixer info
	uint32_t MixSampleRate, MixBitsPerSample;
	uint32_t TickCount, SamplesToMix, MinPeriod, MaxPeriod;
	uint16_t MixChannels, Row, NextRow, Rows;
	uint32_t MusicSpeed, MusicTempo;
	uint16_t Pattern, NewPattern, NextPattern;
	uint32_t RowsPerBeat, SamplesPerTick;
	Channel *Channels;
	uint32_t nMixerChannels, *MixerChannels;

	uint8_t GlobalVolume, GlobalVolSlide;
	uint8_t PatternLoopCount, PatternLoopStart;
	uint8_t PatternDelay, FrameDelay;
	int MixBuffer[MIXBUFFERSIZE * 2];
	int DCOffsR, DCOffsL;

	// Effects functions
	void VolumeSlide(Channel *channel, uint8_t param);
	void FineVolumeSlide(Channel *channel, uint8_t param, uint16_t (*op)(const uint16_t, const uint8_t));
	void ChannelVolumeSlide(Channel *channel, uint8_t param);
	void GlobalVolumeSlide(uint8_t param);
	void PanningSlide(Channel *channel, uint8_t param);
	void PortamentoUp(Channel *channel, uint8_t param);
	void PortamentoDown(Channel *channel, uint8_t param);
	void FinePortamentoUp(Channel *channel, uint8_t param);
	void FinePortamentoDown(Channel *channel, uint8_t param);
	void ExtraFinePortamentoUp(Channel *channel, uint8_t param);
	void ExtraFinePortamentoDown(Channel *channel, uint8_t param);
	void TonePortamento(Channel *channel, uint8_t param);
	int PatternLoop(uint32_t param);
	void ProcessMODExtended(Channel *channel);
	void ProcessS3MExtended(Channel *channel);

	// Processing functions
	bool AdvanceTick();
	bool Tick();
	bool ProcessEffects();
	void ResetChannelPanning();
	void SampleChange(Channel &channel, const uint32_t sample);
	void ReloadSample(Channel &channel);
	void NoteChange(Channel * const channel, uint8_t note, uint8_t cmd);
	void HandleNNA(Channel *channel, uint32_t sample, uint8_t note);
	uint8_t FindFreeNNAChannel() const;
	uint32_t GetPeriodFromNote(uint8_t Note, uint8_t FineTune, uint32_t C4Speed);
	uint32_t GetFreqFromPeriod(uint32_t Period, uint32_t C4Speed, int32_t PeriodFrac);

	// Mixing functions
	inline void FixDCOffset(int *p_DCOffsL, int *p_DCOffsR, int *buff, uint32_t samples);
	void DCFixingFill(uint32_t samples);
	void CreateStereoMix(uint32_t count);
	inline void MonoFromStereo(uint32_t count);

private:
	void modLoadPCM(const modMOD_t &file);
	void s3mLoadPCM(const modS3M_t &file);
	void stmLoadPCM(const modSTM_t &file);
	void AONLoadPCM(FILE *f_AON);
	void itLoadPCM(const modIT_t &file);
	void DeinitMixer();

public:
	ModuleFile(const modMOD_t &file);
	ModuleFile(const modS3M_t &file);
	ModuleFile(const modSTM_t &file);
	ModuleFile(AON_Intern *p_AF);
#ifdef ENABLE_FC1x
	ModuleFile(const modFC1x_t &file);
#endif
	ModuleFile(const modIT_t &file);
	virtual ~ModuleFile();

	stringPtr_t title() const noexcept;
	stringPtr_t author() const noexcept;
	stringPtr_t remark() const noexcept;
	uint8_t channels() const noexcept;
	void InitMixer(fileInfo_t &info);
	int32_t Mix(uint8_t *Buffer, uint32_t BuffLen);
};

inline uint32_t swapBytes(const uint32_t val) noexcept
{
	return ((val >> 24) & 0xFF) | ((val >> 8) & 0xFF00) |
		((val & 0xFF00) << 8) | ((val & 0xFF) << 24);
}

inline uint16_t swapBytes(const uint16_t val) noexcept
	{ return ((val >> 8) & 0xFF) | ((val & 0xFF) << 8); }

inline uint32_t Swap32(uint32_t i) noexcept { return swapBytes(i); }
inline uint16_t Swap16(uint16_t i) noexcept { return swapBytes(i); }

struct moduleFile_t::decoderContext_t
{
	uint8_t playbackBuffer[8192];
	std::unique_ptr<ModuleFile> mod;
};

#endif /*__GenericModule_H__*/
