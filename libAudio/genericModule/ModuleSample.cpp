#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"

#define BE2LE(var) (uint32_t)((((uint16_t)(var & 0xFF)) << 8) | (var >> 8))

ModuleSample::ModuleSample(uint32_t id, uint8_t type) : Type(type), ID(id)
{
}

ModuleSample *ModuleSample::LoadSample(MOD_Intern *p_MF, uint32_t i)
{
	return new ModuleSampleNative(p_MF, i);
}

ModuleSample *ModuleSample::LoadSample(S3M_Intern *p_SF, uint32_t i)
{
	uint8_t Type;
	fread(&Type, 1, 1, p_SF->f_S3M);
	if (Type > 1)
		return new ModuleSampleAdlib(p_SF, i, Type);
	else
		return new ModuleSampleNative(p_SF, i, Type);
}

uint8_t ModuleSample::GetType()
{
	return Type;
}

ModuleSampleNative::ModuleSampleNative(MOD_Intern *p_MF, uint32_t i) : ModuleSample(i, 1)
{
	uint16_t Short;
	FILE *f_MOD = p_MF->f_MOD;
	Name = new char[23];

	fread(Name, 22, 1, f_MOD);
	if (Name[21] != 0)
		Name[22] = 0;

	fread(&Short, 2, 1, f_MOD);
	Length = BE2LE(Short) * 2;
	fread(&FineTune, 1, 1, f_MOD);
	fread(&Volume, 1, 1, f_MOD);
	fread(&Short, 2, 1, f_MOD);
	LoopStart = BE2LE(Short) * 2;
	fread(&Short, 2, 1, f_MOD);
	LoopLen = BE2LE(Short) * 2;
	if (Volume > 64)
		Volume = 64;
	FineTune &= 0x0F;
	if (LoopLen > 2 && Length > (LoopStart + LoopLen))
		Length = LoopStart + LoopLen;

	/********************************************\
	|* The following block just initialises the *|
	|* unused fields to harmless values.        *|
	\********************************************/
	Packing = Flags = 0;
	C4Speed = 8363;
	FileName = NULL;
}

#define fread_24bit(Dest, File) \
{ \
	uint16_t P1; \
	uint8_t P2; \
	fread(&P2, 1, 1, File); \
	fread(&P1, 2, 1, File); \
	Dest = (((uint32_t)P2) << 16) | P1; \
}

ModuleSampleNative::ModuleSampleNative(S3M_Intern *p_SF, uint32_t i, uint8_t type) : ModuleSample(i, type)
{
	uint8_t DontCare[12];
	char Magic[4];
	FILE *f_S3M = p_SF->f_S3M;
	Name = new char[29];
	FileName = new char[13];

	fread(FileName, 12, 1, f_S3M);
	if (FileName[11] != 0)
		FileName[12] = 0;
	fread_24bit(SamplePos, f_S3M);
	fread(&Length, 4, 1, f_S3M);
	fread(&LoopStart, 4, 1, f_S3M);
	fread(&LoopLen, 4, 1, f_S3M);
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
}

ModuleSampleNative::~ModuleSampleNative()
{
	delete Name;
}

uint32_t ModuleSampleNative::GetLength()
{
	return Length;
}

uint32_t ModuleSampleNative::GetLoopStart()
{
	return LoopStart;
}

uint32_t ModuleSampleNative::GetLoopLen()
{
	return LoopLen;
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

bool ModuleSampleNative::Get16Bit()
{
	return (Flags & 4) != 0;
}

ModuleSampleAdlib::ModuleSampleAdlib(S3M_Intern *p_SF, uint32_t i, uint8_t Type) : ModuleSample(i, Type)
{
	uint8_t DontCare[12];
	char Magic[4];
	FILE *f_S3M = p_SF->f_S3M;
	Name = new char[29];
	FileName = new char[13];

	fread(FileName, 12, 1, f_S3M);
	if (FileName[11] != 0)
		FileName[12] = 0;
	fread(DontCare, 3, 1, f_S3M);
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

uint32_t ModuleSampleAdlib::GetLength()
{
	return 0;
}

uint32_t ModuleSampleAdlib::GetLoopStart()
{
	return 0;
}

uint32_t ModuleSampleAdlib::GetLoopLen()
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

bool ModuleSampleAdlib::Get16Bit()
{
	return false;
}
