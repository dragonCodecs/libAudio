#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"

ModuleSample::ModuleSample(uint32_t id, uint8_t type) : Type(type), ID(id)
{
}

ModuleSample::~ModuleSample()
{
}

ModuleSample *ModuleSample::LoadSample(MOD_Intern *p_MF, uint32_t i)
{
	return new ModuleSampleNative(p_MF, i);
}

ModuleSample *ModuleSample::LoadSample(S3M_Intern *p_SF, uint32_t i)
{
	uint8_t Type;
	fread(&Type, 1, 1, p_SF->f_Module);
	if (Type > 1)
		return new ModuleSampleAdlib(p_SF, i, Type);
	else
		return new ModuleSampleNative(p_SF, i, Type);
}

ModuleSample *ModuleSample::LoadSample(STM_Intern *p_SF, uint32_t i)
{
	return new ModuleSampleNative(p_SF, i);
}

ModuleSample *ModuleSample::LoadSample(AON_Intern *p_AF, uint32_t i, char *Name)
{
	return new ModuleSampleNative(p_AF, i, Name);
}

ModuleSample *ModuleSample::LoadSample(IT_Intern *p_IF, uint32_t i)
{
	return new ModuleSampleNative(p_IF, i);
}

uint8_t ModuleSample::GetType()
{
	return Type;
}

ModuleSampleNative::ModuleSampleNative(MOD_Intern *p_MF, uint32_t i) : ModuleSample(i, 1)
{
	uint16_t Short;
	FILE *f_MOD = p_MF->f_Module;
	Name = new char[23];

	fread(Name, 22, 1, f_MOD);
	if (Name[21] != 0)
		Name[22] = 0;

	fread(&Short, 2, 1, f_MOD);
	Length = Swap16(Short) * 2;
	fread(&FineTune, 1, 1, f_MOD);
	fread(&Volume, 1, 1, f_MOD);
	fread(&Short, 2, 1, f_MOD);
	LoopStart = Swap16(Short) * 2;
	fread(&Short, 2, 1, f_MOD);
	LoopEnd = Swap16(Short) * 2;
	if (Volume > 64)
		Volume = 64;
	FineTune &= 0x0F;
	LoopEnd = (LoopStart < Length && LoopEnd > 2 ? LoopStart + LoopEnd : 0);

	SampleFlags = 0;
	if (LoopEnd != 0)
		SampleFlags |= SAMPLE_FLAGS_LOOP;

	/********************************************\
	|* The following block just initialises the *|
	|* unused fields to harmless values.        *|
	\********************************************/
	Packing = 0;
	C4Speed = 8363;
	FileName = nullptr;
	VibratoSpeed = VibratoDepth = VibratoType = VibratoRate = 0;
}

void fread_24bit(uint32_t &dest, FILE *file) noexcept
{
	uint16_t P1;
	uint8_t P2;
	fread(&P2, 1, 1, file);
	fread(&P1, 2, 1, file);
	dest = (uint32_t(P2) << 16) | P1;
}

ModuleSampleNative::ModuleSampleNative(S3M_Intern *p_SF, uint32_t i, uint8_t type) : ModuleSample(i, type)
{
	uint8_t DontCare[12];
	char Magic[4];
	FILE *f_S3M = p_SF->f_Module;
	Name = new char[29];
	FileName = new char[13];

	fread(FileName, 12, 1, f_S3M);
	if (FileName[11] != 0)
		FileName[12] = 0;
	fread_24bit(SamplePos, f_S3M);
	fread(&Length, 4, 1, f_S3M);
	fread(&LoopStart, 4, 1, f_S3M);
	fread(&LoopEnd, 4, 1, f_S3M);
	fread(&Volume, 1, 1, f_S3M);
	fread(DontCare, 1, 1, f_S3M);
	fread(&Packing, 1, 1, f_S3M);
	if (Packing == 1)
		printf("%d => ADPCM sample\n", i);
	fread(&Flags, 1, 1, f_S3M);
	if ((Flags & 2) != 0)
		printf("%d => Stereo\n", i);
	fread(&C4Speed, 4, 1, f_S3M);
	fread(DontCare, 12, 1, f_S3M);
	fread(Name, 28, 1, f_S3M);
	if (Name[27] != 0)
		Name[28] = 0;
	fread(Magic, 4, 1, f_S3M);
	if (Type == 1)
	{
		if (memcmp(Magic, "SCRS", 4) != 0)
			throw new ModuleLoaderError(E_BAD_S3M);
	}

	// If looping not enabled, zero the Loop fields
	if ((Flags & 1) == 0)
		LoopStart = LoopEnd = 0;

	SampleFlags = 0;
	SampleFlags |= (Flags & 1) ? SAMPLE_FLAGS_LOOP : 0;
	SampleFlags |= (Flags & 2) ? SAMPLE_FLAGS_STEREO : 0;
	SampleFlags |= (Flags & 4) ? SAMPLE_FLAGS_16BIT : 0;

	/********************************************\
	|* The following block just initialises the *|
	|* unused fields to harmless values.        *|
	\********************************************/
	VibratoSpeed = VibratoDepth = VibratoType = VibratoRate = 0;
}

ModuleSampleNative::ModuleSampleNative(STM_Intern *p_SF, uint32_t i) : ModuleSample(i, 1)
{
	uint8_t ID, Disk, Reserved2;
	uint16_t Reserved1, C3Speed, Unknown;
	uint32_t Reserved3;
	FILE *f_STM = p_SF->f_Module;

	Name = new char[13];
	fread(Name, 12, 1, f_STM);
	if (Name[11] != 0)
		Name[12] = 0;
	fread(&ID, 1, 1, f_STM);
	fread(&Disk, 1, 1, f_STM);
	fread(&Reserved1, 2, 1, f_STM);
	fread(&Length, 2, 1, f_STM);
	fread(&LoopStart, 2, 1, f_STM);
	fread(&LoopEnd, 2, 1, f_STM);
	fread(&Volume, 1, 1, f_STM);
	fread(&Reserved2, 1, 1, f_STM);
	fread(&C3Speed, 2, 1, f_STM);
	// TODO: What's the diff. between them?
	C4Speed = C3Speed;
	fread(&Reserved3, 4, 1, f_STM);
	// XXX: One spec says this is the "Length in Paragraphs", need to figure out what that means.
	fread(&Unknown, 2, 1, f_STM);
	if (ID > 0)
		throw new ModuleLoaderError(E_BAD_STM);

	SampleFlags = 0;
	if (LoopEnd < LoopStart || LoopEnd == 0xFFFF)
	{
		LoopEnd = 0;
		LoopStart = 0;
	}
	else
		SampleFlags |= SAMPLE_FLAGS_LOOP;
	if (Volume > 64)
		Volume = 64;

	/********************************************\
	|* The following block just initialises the *|
	|* unused fields to harmless values.        *|
	\********************************************/
	Packing = SampleFlags = 0;
	Name = nullptr;
	FineTune = 0;
	VibratoSpeed = VibratoDepth = VibratoType = VibratoRate = 0;
}

ModuleSampleNative::ModuleSampleNative(AON_Intern *p_AF, uint32_t i, char *name) : ModuleSample(i, 1)
{
	FILE *f_AON = p_AF->f_Module;

	Name = name;
	fread(&Length, 4, 1, f_AON);
	Length = Swap32(Length);
}

ModuleSampleNative::ModuleSampleNative(IT_Intern *p_IF, uint32_t i) : ModuleSample(i, 1)
{
	char Magic[4], Const;
	FILE *f_IT = p_IF->f_Module;

	fread(&Magic, 4, 1, f_IT);
	if (strncmp(Magic, "IMPS", 4) != 0)
		throw new ModuleLoaderError(E_BAD_IT);
	FileName = new char[13];
	Name = new char[27];
	fread(FileName, 12, 1, f_IT);
	if (FileName[11] != 0)
		FileName[12] = 0;
	fread(&Const, 1, 1, f_IT);
	fread(&InstrVol, 1, 1, f_IT);
	fread(&Flags, 1, 1, f_IT);
	fread(&Volume, 1, 1, f_IT);
	if (Volume > 64)
		Volume = 64;
	fread(Name, 26, 1, f_IT);
	if (Name[25] != 0)
		Name[26] = 0;
	fread(&Packing, 1, 1, f_IT);
	fread(&DefaultPan, 1, 1, f_IT);
	fread(&Length, 4, 1, f_IT);
	fread(&LoopStart, 4, 1, f_IT);
	fread(&LoopEnd, 4, 1, f_IT);
	fread(&C4Speed, 4, 1, f_IT);
	fread(&SusLoopBegin, 4, 1, f_IT);
	fread(&SusLoopEnd, 4, 1, f_IT);
	fread(&SamplePos, 4, 1, f_IT);
	fread(&VibratoSpeed, 1, 1, f_IT);
	fread(&VibratoDepth, 1, 1, f_IT);
	fread(&VibratoType, 1, 1, f_IT);
	fread(&VibratoRate, 1, 1, f_IT);

	if (Const != 0 || Packing > 63 || VibratoSpeed > 64 || VibratoDepth > 64 ||
		/*VibratoType > 4  ||*/ (VibratoType < 4 && VibratoRate > 64) || InstrVol > 64)
		throw new ModuleLoaderError(E_BAD_IT);

	if (C4Speed == 0)
		C4Speed = 8363;
	else if (C4Speed < 256)
		C4Speed = 256;
	/*else
		C4Speed /= 2;*/

	// If looping not enabled, zero the Loop fields
	if ((Flags & 0x10) == 0)
		LoopStart = LoopEnd = 0;

	SampleFlags = 0;
	SampleFlags |= (Flags & 0x10) ? SAMPLE_FLAGS_LOOP : 0;
	SampleFlags |= (Flags & 0x04) ? SAMPLE_FLAGS_STEREO : 0;
	SampleFlags |= (Flags & 0x02) ? SAMPLE_FLAGS_16BIT : 0;
	SampleFlags |= (Flags & 0x40) ? SAMPLE_FLAGS_LPINGPONG : 0;
}

ModuleSampleNative::~ModuleSampleNative()
{
	delete [] Name;
	delete [] FileName;
}

uint32_t ModuleSampleNative::GetLength()
{
	return Length;
}

uint32_t ModuleSampleNative::GetLoopStart()
{
	return LoopStart;
}

uint32_t ModuleSampleNative::GetLoopEnd()
{
	return LoopEnd;
}

uint8_t ModuleSampleNative::GetFineTune()
{
	return FineTune;
}

uint32_t ModuleSampleNative::GetC4Speed()
{
	return C4Speed;
}

uint8_t ModuleSampleNative::GetVolume()
{
	return Volume << 1;
}

uint8_t ModuleSampleNative::GetVibratoSpeed()
{
	return VibratoSpeed;
}

uint8_t ModuleSampleNative::GetVibratoDepth()
{
	return VibratoDepth;
}

uint8_t ModuleSampleNative::GetVibratoType()
{
	return VibratoType;
}

uint8_t ModuleSampleNative::GetVibratoRate()
{
	return VibratoRate;
}

bool ModuleSampleNative::Get16Bit()
{
	return (SampleFlags & SAMPLE_FLAGS_16BIT) != 0;
}

bool ModuleSampleNative::GetStereo()
{
	return (SampleFlags & SAMPLE_FLAGS_STEREO) != 0;
}

bool ModuleSampleNative::GetLooped()
{
	return (SampleFlags & SAMPLE_FLAGS_LOOP) != 0;
}

bool ModuleSampleNative::GetBidiLoop()
{
	return (SampleFlags & SAMPLE_FLAGS_LPINGPONG) != 0;
}

ModuleSampleAdlib::ModuleSampleAdlib(S3M_Intern *p_SF, uint32_t i, uint8_t Type) : ModuleSample(i, Type)
{
	uint8_t DontCare[12];
	char Magic[4];
	FILE *f_S3M = p_SF->f_Module;
	Name = new char[29];
	FileName = new char[13];

	fread(FileName, 12, 1, f_S3M);
	if (FileName[11] != 0)
		FileName[12] = 0;
	fread(DontCare, 3, 1, f_S3M);
	if (DontCare[0] != DontCare[1] || DontCare[1] != DontCare[2] || DontCare[0] != 0)
		throw new ModuleLoaderError(E_BAD_S3M);
	fread(&D00, 1, 1, f_S3M);
	fread(&D01, 1, 1, f_S3M);
	fread(&D02, 1, 1, f_S3M);
	fread(&D03, 1, 1, f_S3M);
	fread(&D04, 1, 1, f_S3M);
	fread(&D05, 1, 1, f_S3M);
	fread(&D06, 1, 1, f_S3M);
	fread(&D07, 1, 1, f_S3M);
	fread(&D08, 1, 1, f_S3M);
	fread(&D09, 1, 1, f_S3M);
	fread(&D0A, 1, 1, f_S3M);
	fread(&D0B, 1, 1, f_S3M);
	fread(&Volume, 1, 1, f_S3M);
	fread(&DONTKNOW, 1, 1, f_S3M);
	fread(DontCare, 2, 1, f_S3M);
	fread(&C4Speed, 4, 1, f_S3M);
	fread(DontCare, 12, 1, f_S3M);
	fread(Name, 28, 1, f_S3M);
	if (Name[27] != 0)
		Name[28] = 0;
	fread(Magic, 4, 1, f_S3M);
	if (memcmp(Magic, "SCRI", 4) != 0)
		throw new ModuleLoaderError(E_BAD_S3M);
}

ModuleSampleAdlib::~ModuleSampleAdlib()
{
	delete [] Name;
	delete [] FileName;
}

uint32_t ModuleSampleAdlib::GetLength()
{
	return 0;
}

uint32_t ModuleSampleAdlib::GetLoopStart()
{
	return 0;
}

uint32_t ModuleSampleAdlib::GetLoopEnd()
{
	return 0;
}

uint8_t ModuleSampleAdlib::GetFineTune()
{
	return 0;
}

uint32_t ModuleSampleAdlib::GetC4Speed()
{
	return C4Speed;
}

uint8_t ModuleSampleAdlib::GetVolume()
{
	return Volume << 1;
}

uint8_t ModuleSampleAdlib::GetVibratoSpeed()
{
	return 0;
}

uint8_t ModuleSampleAdlib::GetVibratoDepth()
{
	return 0;
}

uint8_t ModuleSampleAdlib::GetVibratoType()
{
	return 0;
}

uint8_t ModuleSampleAdlib::GetVibratoRate()
{
	return 0;
}

bool ModuleSampleAdlib::Get16Bit()
{
	return false;
}

bool ModuleSampleAdlib::GetStereo()
{
	return false;
}

bool ModuleSampleAdlib::GetLooped()
{
	return false;
}

bool ModuleSampleAdlib::GetBidiLoop()
{
	return false;
}
