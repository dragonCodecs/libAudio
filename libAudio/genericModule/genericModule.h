// SPDX-License-Identifier: BSD-3-Clause
#ifndef GENERIC_MODULE__H
#define GENERIC_MODULE__H

#include <substrate/fixed_vector>
#include <substrate/managed_ptr>
#include "../libAudio.hxx"
#include <array>
#include <exception>

using substrate::fixedVector_t;
using substrate::managedPtr_t;

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

#include "effects.h"

#define MIXBUFFERSIZE		512

#define MODULE_MOD		1U
#define MODULE_S3M		2U
#define MODULE_STM		3U
#define MODULE_AON		4U
#define MODULE_FC1x		5U
#define MODULE_IT		6U

#define E_BAD_MOD		1U
#define E_BAD_S3M		2U
#define E_BAD_STM		3U
#define E_BAD_AON		4U
#define E_BAD_FC1x		5U
#define E_BAD_IT		6U

#define FILE_FLAGS_AMIGA_SLIDES		0x01U
#define FILE_FLAGS_AMIGA_LIMITS		0x02U
#define FILE_FLAGS_FAST_SLIDES		0x04U
#define FILE_FLAGS_LINEAR_SLIDES	0x08U
#define FILE_FLAGS_OLD_IT_EFFECTS	0x10U

#define SAMPLE_FLAGS_LOOP			0x01U
#define SAMPLE_FLAGS_STEREO			0x02U
#define SAMPLE_FLAGS_16BIT			0x04U
#define SAMPLE_FLAGS_LPINGPONG		0x08U
#define SAMPLE_FLAGS_LREVERSE		0x10U
#define SAMPLE_FLAGS_SUSTAINLOOP	0x20U

#define ENVELOPE_VOLUME		0U
#define ENVELOPE_PANNING	1U
#define ENVELOPE_PITCH		2U

class ModuleLoaderError : std::exception
{
private:
	const uint32_t Error;

public:
	ModuleLoaderError(const uint32_t Error);
	const char *GetError() const noexcept { return error(); }
	const char *error() const noexcept;
	const char *what() const noexcept final { return error(); }
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

class ModuleSample
{
protected:
	const uint8_t _type;

private:
	uint32_t _id;

protected:
	constexpr ModuleSample(const uint32_t id, const uint8_t type) noexcept : _type(type), _id(id) { }
	void resetID(const uint32_t id) noexcept { _id = id; }

public:
	static ModuleSample *LoadSample(const modMOD_t &file, const uint32_t i);
	static ModuleSample *LoadSample(const modS3M_t &file, const uint32_t i);
	static ModuleSample *LoadSample(const modSTM_t &file, const uint32_t i);
	static ModuleSample *LoadSample(const modAON_t &file, const uint32_t i, char *Name, const uint32_t *const pcmLengths);
	static ModuleSample *LoadSample(const modIT_t &file, const uint32_t i);

	virtual ~ModuleSample() noexcept = default;
	uint8_t GetType() const noexcept { return _type; }
	uint32_t id() const noexcept { return _id; }
	virtual uint32_t GetLength() = 0;
	virtual uint32_t GetLoopStart() = 0;
	virtual uint32_t GetLoopEnd() = 0;
	virtual uint32_t GetSustainLoopBegin() = 0;
	virtual uint32_t GetSustainLoopEnd() = 0;
	virtual uint8_t GetFineTune() = 0;
	virtual uint32_t GetC4Speed() = 0;
	virtual uint8_t GetVolume() = 0;
	virtual uint8_t GetSampleVolume() = 0;
	virtual uint8_t GetVibratoSpeed() = 0;
	virtual uint8_t GetVibratoDepth() = 0;
	virtual uint8_t GetVibratoType() = 0;
	virtual uint8_t GetVibratoRate() = 0;
	virtual uint16_t GetPanning() = 0;
	virtual bool Get16Bit() = 0;
	virtual bool GetStereo() = 0;
	virtual bool GetLooped() = 0;
	virtual bool GetSustainLooped() = 0;
	virtual bool GetBidiLoop() = 0;
	virtual bool GetPanned() = 0;
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
	uint32_t SamplePos; // actually 24-bit for S3M
	uint8_t Packing;
	uint8_t Flags;
	uint8_t SampleFlags;
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
	ModuleSampleNative(const modAON_t &file, const uint32_t i, char *Name, const uint32_t *const pcmLengths);
	ModuleSampleNative(const modIT_t &file, const uint32_t i);
	~ModuleSampleNative() noexcept = default;

	uint32_t GetLength() override final { return Length; }
	uint32_t GetLoopStart() override final { return LoopStart; }
	uint32_t GetLoopEnd() override final { return LoopEnd; }
	uint32_t GetSustainLoopBegin() override final { return SusLoopBegin; }
	uint32_t GetSustainLoopEnd() override final { return SusLoopEnd; }
	uint8_t GetFineTune() override final { return FineTune; }
	uint32_t GetC4Speed() override final { return C4Speed; }
	uint8_t GetVolume() override final { return Volume << 1U; }
	uint8_t GetSampleVolume() override final { return InstrVol; }
	uint8_t GetVibratoSpeed() override final { return VibratoSpeed; }
	uint8_t GetVibratoDepth() override final { return VibratoDepth; }
	uint8_t GetVibratoType() override final { return VibratoType; }
	uint8_t GetVibratoRate() override final { return VibratoRate; }
	uint16_t GetPanning() override final { return uint16_t(DefaultPan & 0x7FU) << 2; }
	bool Get16Bit() override final;
	bool GetStereo() override final;
	bool GetLooped() override final;
	bool GetSustainLooped() override final;
	bool GetBidiLoop() override final;
	bool GetPanned() override final { return DefaultPan & 0x80U; }
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
	uint32_t GetSustainLoopBegin();
	uint32_t GetSustainLoopEnd();
	uint8_t GetFineTune();
	uint32_t GetC4Speed();
	uint8_t GetVolume();
	uint8_t GetSampleVolume();
	uint8_t GetVibratoSpeed();
	uint8_t GetVibratoDepth();
	uint8_t GetVibratoType();
	uint8_t GetVibratoRate();
	uint16_t GetPanning();
	bool Get16Bit();
	bool GetStereo();
	bool GetLooped();
	bool GetSustainLooped();
	bool GetBidiLoop();
	bool GetPanned();
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
	bool GetCarried() const noexcept;
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
	uint8_t SampleMapping[240];

	ModuleInstrument(const uint32_t id) noexcept : _id(id) { }

public:
	static ModuleInstrument *LoadInstrument(const modIT_t &file, const uint32_t i, const uint16_t FormatVersion);

	std::pair<uint8_t, uint8_t> mapNote(const uint8_t note) noexcept;
	virtual ~ModuleInstrument() noexcept = default;
	virtual uint16_t GetFadeOut() const noexcept = 0;
	virtual bool GetEnvEnabled(uint8_t env) const noexcept = 0;
	virtual bool GetEnvLooped(uint8_t env) const noexcept = 0;
	virtual bool GetEnvCarried(uint8_t env) const noexcept = 0;
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
	std::unique_ptr<ModuleEnvelope> Envelope;

public:
	ModuleOldInstrument(const modIT_t &file, const uint32_t i);
	~ModuleOldInstrument() = default;

	uint16_t GetFadeOut() const noexcept override final;
	bool GetEnvEnabled(uint8_t env) const noexcept override final;
	bool GetEnvLooped(uint8_t env) const noexcept override final;
	bool GetEnvCarried(uint8_t env) const noexcept override final;
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
	std::array<std::unique_ptr<ModuleEnvelope>, 3> Envelopes;

public:
	ModuleNewInstrument(const modIT_t &file, const uint32_t i);
	~ModuleNewInstrument() = default;

	uint16_t GetFadeOut() const noexcept override final;
	bool GetEnvEnabled(uint8_t env) const noexcept override final;
	bool GetEnvLooped(uint8_t env) const noexcept override final;
	bool GetEnvCarried(uint8_t env) const noexcept override final;
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
	ModulePattern(const modMOD_t &file, const uint32_t channels);
	ModulePattern(const modS3M_t &file, const uint32_t channels);
	ModulePattern(const modSTM_t &file);
	ModulePattern(const modAON_t &file, const uint32_t channels);
	ModulePattern(const modIT_t &file, const uint32_t channels);

	const fixedVector_t<commandPtr_t> &commands() const { return _commands; }
	uint16_t rows() const noexcept { return _rows; }
};

struct int16dot16_t
{
	uint16_t Lo;
	int16_t Hi;
};

union int16dot16
{
	int32_t iValue;
	int16dot16_t Value;
};

class Channel : public ModuleAllocator
{
public:
	uint8_t *SampleData;
	uint8_t *NewSampleData;
	uint8_t Note, RampLength;
	uint8_t NewNote, NewSample;
	uint32_t LoopStart, LoopEnd, Length;
	uint8_t RawVolume, volume, _sampleVolumeSlide, _fineSampleVolumeSlide;
	uint8_t channelVolume, _channelVolumeSlide, sampleVolume;
	uint16_t AutoVibratoDepth;
	uint8_t AutoVibratoPos;
	ModuleSample *Sample;
	ModuleInstrument *Instrument;
	uint8_t FineTune, _panningSlide;
	uint16_t RawPanning;
	uint16_t panning; // TODO: panning should be uint8_t?
	uint8_t RowNote, RowSample, RowVolEffect;
	uint8_t RowEffect, RowVolParam, RowParam;
	uint16_t Flags;
	uint32_t Period, C4Speed;
	uint32_t Pos, PosLo, StartTick;
	int16dot16 increment;
	uint32_t portamentoTarget;
	uint8_t portamento, portamentoSlide;
	uint8_t Arpeggio, extendedCommand;
	uint8_t Tremor, TremorCount;
	uint8_t leftVol;
	uint8_t rightVol;
	uint8_t NewLeftVol, NewRightVol;
	short LeftRamp, RightRamp;
	uint8_t patternLoopCount;
	uint16_t patternLoopStart;
	int Filter_Y1, Filter_Y2, Filter_Y3, Filter_Y4;
	int Filter_A0, Filter_B0, Filter_B1, Filter_HP;
	uint8_t TremoloDepth, TremoloSpeed, TremoloPos, TremoloType;
	uint8_t vibratoDepth, vibratoSpeed, vibratoPosition, vibratoType;
	uint8_t panbrelloDepth, panbrelloSpeed;
	uint16_t panbrelloPosition;
	uint8_t panbrelloType;
	uint16_t EnvVolumePos, EnvPanningPos, EnvPitchPos, FadeOutVol;
	int DCOffsL, DCOffsR;

public:
	Channel();
	void SetData(ModuleCommand *Command, ModuleHeader *p_Header);

	// Channel effects
	void noteChange(ModuleFile &module, uint8_t note, bool handlePorta = false);
	void noteCut(ModuleFile &module, uint32_t tick) noexcept;
	void noteOff() noexcept;
	int32_t patternLoop(const uint8_t param, const uint16_t row) noexcept;
	void portamentoUp(const ModuleFile &module, uint8_t param) noexcept;
	void portamentoDown(const ModuleFile &module, uint8_t param) noexcept;
	void finePortamentoUp(const ModuleFile &module, uint8_t param) noexcept;
	void finePortamentoDown(const ModuleFile &module, uint8_t param) noexcept;
	void extraFinePortamentoUp(const ModuleFile &module, uint8_t param) noexcept;
	void extraFinePortamentoDown(const ModuleFile &module, uint8_t param) noexcept;
	void tonePortamento(const ModuleFile &module, uint8_t param);
	void vibrato(uint8_t param, uint8_t multiplier);
	void panbrello(uint8_t param);
	void channelVolumeSlide(const ModuleFile &module, uint8_t param) noexcept;
	void sampleVolumeSlide(const ModuleFile &module, uint8_t param);
	void fineSampleVolumeSlide(const ModuleFile &module, uint8_t param,
		uint16_t (*op)(const uint16_t, const uint8_t)) noexcept;
	void panningSlide(const ModuleFile &module, uint8_t param);
	void ChannelEffect(uint8_t param);

	int16_t applyVibrato(const ModuleFile &module, const uint32_t period) noexcept;
	int16_t applyAutoVibrato(const ModuleFile &module, const uint32_t period, int8_t &fractionalPeriod) noexcept;
	void applyPanbrello() noexcept;

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
	std::unique_ptr<uint32_t []> lengthPCM;
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

	uint16_t GlobalVolume;
	uint8_t globalVolumeSlide;
	uint8_t PatternDelay, FrameDelay;
	int MixBuffer[MIXBUFFERSIZE * 2];
	int DCOffsR, DCOffsL;

	// Effects functions
	void applyGlobalVolumeSlide(uint8_t param);
	void ProcessMODExtended(Channel *channel);
	void ProcessS3MExtended(Channel *channel);

	// Processing functions
	bool AdvanceTick();
	bool Tick();
	bool ProcessEffects();
	void processEffects(Channel &channel, uint8_t param, int16_t &breakRow, int16_t &positionJump);
	void ResetChannelPanning();
	void SampleChange(Channel &channel, const uint32_t sample, const bool doPortamento);
	void ReloadSample(Channel &channel);
	void HandleNNA(Channel *channel, uint32_t sample, uint8_t note);
	uint8_t FindFreeNNAChannel() const;
	bool handleNavigationEffects(const int32_t patternLoopRow, const int16_t breakRow,
		const int16_t positionJump) noexcept;
	uint32_t GetPeriodFromNote(uint8_t Note, uint8_t fineTune, uint32_t C4Speed);
	uint32_t GetFreqFromPeriod(uint32_t Period, uint32_t C4Speed, int8_t PeriodFrac);

	// Mixing functions
	inline void FixDCOffset(int *p_DCOffsL, int *p_DCOffsR, int *buff, uint32_t samples);
	void DCFixingFill(uint32_t samples);
	void CreateStereoMix(uint32_t count);
	inline void MonoFromStereo(uint32_t count);

private:
	void modLoadPCM(const fd_t &fd);
	void s3mLoadPCM(const fd_t &fd);
	void stmLoadPCM(const fd_t &fd);
	void aonLoadPCM(const fd_t &fd);
	void itLoadPCM(const fd_t &fd);
	void DeinitMixer();
	friend class Channel;

public:
	ModuleFile(const modMOD_t &file);
	ModuleFile(const modS3M_t &file);
	ModuleFile(const modSTM_t &file);
	ModuleFile(const modAON_t &file);
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

	uint32_t ticks() const noexcept { return TickCount; }
	uint32_t speed() const noexcept { return MusicSpeed; }
	uint32_t tempo() const noexcept { return MusicTempo; }
	uint32_t minimumPeriod() const noexcept { return MinPeriod; }
	uint32_t maximumPeriod() const noexcept { return MaxPeriod; }
	uint16_t totalSamples() const noexcept { return p_Header->nSamples; }
	uint16_t totalInstruments() const noexcept { return p_Header->nInstruments; }
	ModuleSample *sample(uint16_t index) const noexcept
		{ return index && index <= totalSamples() ? p_Samples[index - 1] : nullptr; }

	template<uint32_t type> bool typeIs() const noexcept { return ModuleType == type; }
	template<uint32_t type, uint32_t... types> typename std::enable_if<sizeof...(types) != 0, bool>::type
		typeIs() const noexcept { return typeIs<type>() || typeIs<types...>(); }

	bool hasLinearSlides() const noexcept { return p_Header->Flags & FILE_FLAGS_LINEAR_SLIDES; }
	bool hasFastSlides() const noexcept { return p_Header->Flags & FILE_FLAGS_FAST_SLIDES; }
	bool useOldEffects() const noexcept { return p_Header->Flags & FILE_FLAGS_OLD_IT_EFFECTS; }
};

inline uint32_t swapBytes(const uint32_t val) noexcept
{
	return ((val >> 24U) & 0xFFU) | ((val >> 8U) & 0xFF00U) |
		((val & 0xFF00U) << 8U) | ((val & 0xFFU) << 24U);
}

inline uint16_t swapBytes(const uint16_t val) noexcept
	{ return ((val >> 8U) & 0xFFU) | ((val & 0xFFU) << 8U); }

inline uint32_t Swap32(uint32_t i) noexcept { return swapBytes(i); }
inline uint16_t Swap16(uint16_t i) noexcept { return swapBytes(i); }

struct moduleFile_t::decoderContext_t
{
	uint8_t playbackBuffer[8192];
	std::unique_ptr<ModuleFile> mod;
};

#endif /*GENERIC_MODULE__H*/
