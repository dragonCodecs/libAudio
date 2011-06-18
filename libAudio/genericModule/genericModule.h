#ifndef __GenericModule_H__
#define __GenericModule_H__

/***************************\
|* ----=== WARNING ===---- *|
|*   This set of classes   *|
|* makes heavy use of the  *|
|*  friend keyword - make  *|
|* sure you do not modify  *|
|*   the relationships!!   *|
\***************************/

class ModuleFile;
class ModuleSample;
class ModulePattern;

#include "../ProTracker.h"
#include "../ScreamTracker.h"
#include "../moduleMixer/moduleMixer.h"

#define MODULE_MOD		1
#define MODULE_S3M		2

#define E_BAD_S3M		1

class ModuleLoaderError
{
private:
	uint32_t Error;

public:
	ModuleLoaderError(uint32_t Error);
	const char *GetError();
};

class ModuleHeader
{
private:
	// Common fields
	char *Name;
	uint16_t nOrders;
	uint16_t nSamples;
	uint16_t nPatterns;
	uint8_t *Orders;

	// Fields specific to certain formats

	// MOD
	uint8_t RestartPos;

	// S3M
	uint8_t Type;
	uint16_t Flags;
	uint16_t CreationVersion;
	uint16_t FormatVersion;
	uint8_t GlobalVolume;
	uint8_t InitialSpeed;
	uint8_t InitialTempo;
	uint8_t MasterVolume;
	uint8_t ChannelSettings[32];
	// Slightly badly named
	// SamplePtrs = pointers to where the sample *descriptors* are
	uint16_t *SamplePtrs;
	// PatternPtrs = pointers to where the compressed pattern data is
	uint16_t *PatternPtrs;

private:
	uint8_t nChannels;
	friend class ModuleFile;

public:
	ModuleHeader(MOD_Intern *p_MF);
	ModuleHeader(S3M_Intern *p_SF);
	~ModuleHeader();
};

class ModuleSample
{
private:
	uint8_t Type;

private:
	uint32_t ID;
	friend class ModuleFile;

protected:
	ModuleSample(uint32_t ID, uint8_t Type);

public:
	static ModuleSample *LoadSample(MOD_Intern *p_MF, uint32_t i);
	static ModuleSample *LoadSample(S3M_Intern *p_SF, uint32_t i);

	uint8_t GetType();
	virtual uint32_t GetLength() = 0;
	virtual uint32_t GetLoopStart() = 0;
	virtual uint32_t GetLoopLen() = 0;
	virtual uint8_t GetFineTune() = 0;
	virtual uint8_t GetVolume() = 0;
	uint32_t GetID();
};

class ModuleSampleNative : public ModuleSample
{
private:
	char *Name;
	uint32_t Length;
	uint8_t FineTune;
	uint8_t Volume;
	uint32_t LoopStart;
	uint32_t LoopLen;

private:
	char *FileName;
	uint32_t SamplePos; // actually 24-bit..
	uint8_t Packing;
	uint8_t Flags;
	uint32_t C4Speed;
	friend class ModuleFile;

public:
	ModuleSampleNative(MOD_Intern *p_MF, uint32_t i);
	ModuleSampleNative(S3M_Intern *p_SF, uint32_t i);
	~ModuleSampleNative();

	uint32_t GetLength();
	uint32_t GetLoopStart();
	uint32_t GetLoopLen();
	uint8_t GetFineTune();
	uint8_t GetVolume();
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
	ModuleSampleAdlib(S3M_Intern *p_SF, uint32_t i, uint8_t Type);
	~ModuleSampleAdlib();

	uint32_t GetLength();
	uint32_t GetLoopStart();
	uint32_t GetLoopLen();
	uint8_t GetFineTune();
	uint8_t GetVolume();
};

class ModuleCommand
{
private:
	uint8_t Sample;
	uint16_t Period;
	uint16_t Effect;

public:
	void SetMODData(uint8_t Data[4]);
	void SetS3MData(uint8_t Note, uint8_t sample);
	uint8_t GetSample();
	uint16_t GetPeriod();
	uint16_t GetEffect();
};

class ModulePattern
{
private:
	ModuleCommand (*Commands)[64];

public:
	ModulePattern(MOD_Intern *p_MF, uint32_t nChannels);
	ModulePattern(S3M_Intern *p_SF, uint32_t nChannels);
	~ModulePattern();

	ModuleCommand **GetCommands();
};

class ModuleFile
{
private:
	uint8_t ModuleType;
	ModuleHeader *p_Header;
	ModuleSample **p_Samples;
	ModulePattern **p_Patterns;
	uint8_t **p_PCM;
	MixerState *p_Mixer;

private:
	void MODLoadPCM(FILE *f_MOD);
	void S3MLoadPCM(FILE *f_S3M);

public:
	ModuleFile(MOD_Intern *p_MF);
	ModuleFile(S3M_Intern *p_SF);
	~ModuleFile();

	const char *GetTitle();
	uint8_t GetChannels();
	void CreateMixer(FileInfo *p_FI);
	long FillBuffer(uint8_t *buffer, long toRead, FileInfo *p_FI);
};

#endif /*__GenericModule_H__*/
