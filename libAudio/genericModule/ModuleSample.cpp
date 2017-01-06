#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"
#include <stdlib.h>

ModuleSample *ModuleSample::LoadSample(const modMOD_t &file, const uint32_t i)
	{ return new ModuleSampleNative(file, i); }

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

ModuleSample *ModuleSample::LoadSample(AON_Intern *p_AF, uint32_t i, char *Name, uint32_t *pcmLengths)
{
	return new ModuleSampleNative(p_AF, i, Name, pcmLengths);
}

ModuleSample *ModuleSample::LoadSample(const modIT_t &file, const uint32_t i)
	{ return new ModuleSampleNative(file, i); }

ModuleSampleNative::ModuleSampleNative(const modMOD_t &file, const uint32_t i) : ModuleSample(i, 1)
{
	uint16_t length16, loopStart16, loopEnd16;
	const fd_t &fd = file.fd();

	Name = makeUnique<char []>(23);

	if (!Name ||
		!fd.read(Name.get(), 22) ||
		!fd.read(&length16, 2) ||
		!fd.read(&FineTune, 1) ||
		!fd.read(&Volume, 1) ||
		!fd.read(&loopStart16, 2) ||
		!fd.read(&loopEnd16, 2))
		throw ModuleLoaderError(E_BAD_MOD);

	Length = Swap16(length16) * 2;
	LoopStart = Swap16(loopStart16) * 2;
	LoopEnd = Swap16(loopEnd16) * 2;
	if (Name[21] != 0)
		Name[22] = 0;
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
	Name = makeUnique<char []>(29);
	FileName = makeUnique<char []>(13);

	fread(FileName.get(), 12, 1, f_S3M);
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
	fread(Name.get(), 28, 1, f_S3M);
	if (Name[27] != 0)
		Name[28] = 0;
	fread(Magic, 4, 1, f_S3M);
	if (_type == 1)
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

	Name = makeUnique<char []>(13);
	fread(Name.get(), 12, 1, f_STM);
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
	FineTune = 0;
	VibratoSpeed = VibratoDepth = VibratoType = VibratoRate = 0;
}

ModuleSampleNative::ModuleSampleNative(AON_Intern *p_AF, uint32_t i, char *name, uint32_t *pcmLengths) : ModuleSample(i, 1), Name(name)
{
	uint8_t Type, ID;
	FILE *f_AON = p_AF->f_Module;

	fread(&Type, 1, 1, f_AON);
	fread(&Volume, 1, 1, f_AON);
	fread(&FineTune, 1, 1, f_AON);
	fread(&ID, 1, 1, f_AON);
	ResetID(ID);
	Length = pcmLengths[ID];
	SampleFlags = 0;

	printf("%u(%u, %u) => %u, %u - ", ID, i, Type, Length, FineTune);
	if (Type == 0)
	{
		uint32_t SampleStart, SampleLen;
		VibratoSpeed = VibratoDepth = VibratoType = 0;

		fread(&SampleStart, 4, 1, f_AON);
		SampleStart = Swap32(SampleStart) << 1;
		fread(&SampleLen, 4, 1, f_AON);
		Length = Swap32(SampleLen) << 1;
		fread(&LoopStart, 4, 1, f_AON);
		LoopStart = Swap32(LoopStart) << 1;
		fread(&LoopEnd, 4, 1, f_AON);
		LoopEnd = (Swap32(LoopEnd) << 1) + LoopStart;
		if (LoopEnd == 2)
			LoopEnd = LoopStart = 0;
		if (LoopEnd != LoopStart)
			SampleFlags |= SAMPLE_FLAGS_LOOP;
		printf("%u, %u, %u, %u(%u)", SampleStart, Length, LoopStart, LoopEnd, LoopEnd - LoopStart);
	}
	else if (Type == 1)
	{
		uint8_t LoopLen, Const;
		/*
		; Wavetable 8 bit
		synth8_partwaveDmaLen	rs.b	1	; in words (--> up to 512 bytes)

		rs.b	1	; Unused
		rs.b	1
		rs.b	1
		rs.b	1
		rs.b	1

		synth8_VIBpara		rs.b	1	; the same param. like with effect '4'
		synth8_vibdelay		rs.b	1	; framecnt
		synth8_vibwave		rs.b	1	; sine,triangle,rectangle
		synth8_WAVEspd		rs.b	1	; framecnt
		synth8_WAVElen		rs.b	1
		synth8_WAVErep		rs.b	1
		synth8_WAVEreplen	rs.b	1
		synth8_WAVErepCtrl	rs.b	1	; 0=Repeatnormal/1=Backwards/1=PingPong

		rsset	32-4
		instr_Astart		rs.b	1	; Vol_startlevel
		instr_Aadd			rs.b	1	; Zeit bis maximalLevel
		instr_Aend			rs.b	1	; Vol_endlevel
		instr_Asub			rs.b	1	; Zeit bis endlevel
		*/

		fread(&LoopLen, 1, 1, f_AON);
		for (uint8_t i = 0; i < 9; ++i)
			fread(&Const, 1, 1, f_AON);
		fread(&VibratoDepth, 1, 1, f_AON);
		fread(&VibratoSpeed, 1, 1, f_AON);
		fread(&VibratoType, 1, 1, f_AON);
		for (uint8_t i = 0; i < 4; ++i)
		{
			fread(&Const, 1, 1, f_AON);
			printf("%u, ", Const);
		}
		fread(&Const, 0, 0, f_AON);
		if (Const == 1)
			SampleFlags |= SAMPLE_FLAGS_LREVERSE;
		else if (Const == 2)
			SampleFlags |= SAMPLE_FLAGS_LPINGPONG;
		for (uint8_t i = 0; i < 11; ++i)
			fread(&Const, 1, 1, f_AON);
		fread(&Const, 1, 1, f_AON);
		fread(&Const, 1, 1, f_AON);
		fread(&Const, 1, 1, f_AON);
		fread(&Const, 1, 1, f_AON);

		printf("%u, %u", FineTune, LoopLen);
		if (LoopEnd != LoopStart)
			SampleFlags |= SAMPLE_FLAGS_LOOP;
		if (VibratoType == 3)
			VibratoSpeed = VibratoDepth = 0;
		Length = 0; // For the sec as we process the wavtable data completely wrong..
	}
	if (Volume > 64)
		Volume = 64;
	printf("\n");

	/********************************************\
	|* The following block just initialises the *|
	|* unused fields to harmless values.        *|
	\********************************************/
	C4Speed = 8363;
	VibratoRate = 0;
}

ModuleSampleNative::ModuleSampleNative(const modIT_t &file, const uint32_t i) : ModuleSample(i, 1)
{
	char Const;
	std::array<char, 4> magic;
	const fd_t &fd = file.fd();

	if (!fd.read(magic) ||
		strncmp(magic.data(), "IMPS", 4) != 0)
		throw ModuleLoaderError(E_BAD_IT);

	FileName = makeUnique<char []>(13);
	Name = makeUnique<char []>(27);

	if (!FileName || !Name ||
		!fd.read(FileName, 12) ||
		!fd.read(&Const, 1) ||
		!fd.read(&InstrVol, 1) ||
		!fd.read(&Flags, 1) ||
		!fd.read(&Volume, 1) ||
		!fd.read(Name, 26) ||
		!fd.read(&Packing, 1) ||
		!fd.read(&DefaultPan, 1) ||
		!fd.read(&Length, 4) ||
		!fd.read(&LoopStart, 4) ||
		!fd.read(&LoopEnd, 4) ||
		!fd.read(&C4Speed, 4) ||
		!fd.read(&SusLoopBegin, 4) ||
		!fd.read(&SusLoopEnd, 4) ||
		!fd.read(&SamplePos, 4) ||
		!fd.read(&VibratoSpeed, 1) ||
		!fd.read(&VibratoDepth, 1) ||
		!fd.read(&VibratoType, 1) ||
		!fd.read(&VibratoRate, 1))
		throw ModuleLoaderError(E_BAD_IT);

	if (FileName[11] != 0)
		FileName[12] = 0;
	if (Volume > 64)
		Volume = 64;
	if (Name[25] != 0)
		Name[26] = 0;

	if (Const != 0 || Packing > 63 || VibratoSpeed > 64 || VibratoDepth > 64 ||
		/*VibratoType > 4  ||*/ (VibratoType < 4 && VibratoRate > 64) || InstrVol > 64)
		throw ModuleLoaderError(E_BAD_IT);

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

bool ModuleSampleNative::Get16Bit()
	{ return (SampleFlags & SAMPLE_FLAGS_16BIT) != 0; }

bool ModuleSampleNative::GetStereo()
	{ return (SampleFlags & SAMPLE_FLAGS_STEREO) != 0; }

bool ModuleSampleNative::GetLooped()
	{ return (SampleFlags & SAMPLE_FLAGS_LOOP) != 0; }

bool ModuleSampleNative::GetBidiLoop()
	{ return (SampleFlags & SAMPLE_FLAGS_LPINGPONG) != 0; }

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
