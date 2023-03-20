// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2012-2023 Rachel Mant <git@dragonmux.network>
#include <cstdlib>
#include <algorithm>
#include "genericModule.h"

constexpr std::array<char, 4> s3mSampleMagic{{'S', 'C', 'R', 'S'}};

ModuleSample *ModuleSample::LoadSample(const modMOD_t &file, const uint32_t i)
	{ return new ModuleSampleNative(file, i); }

ModuleSample *ModuleSample::LoadSample(const modS3M_t &file, const uint32_t i)
{
	const auto &fd{file.fd()};
	uint8_t type{};

	if (!fd.read(type))
		throw ModuleLoaderError{E_BAD_S3M};
	if (type > 1)
		return new ModuleSampleAdlib(file, i, type);
	else
		return new ModuleSampleNative(file, i, type);
}

ModuleSample *ModuleSample::LoadSample(const modSTM_t &file, const uint32_t i)
	{ return new ModuleSampleNative(file, i); }

ModuleSample *ModuleSample::LoadSample(const modAON_t &file, const uint32_t i, char *Name, const uint32_t *const pcmLengths)
	{ return new ModuleSampleNative(file, i, Name, pcmLengths); }

ModuleSample *ModuleSample::LoadSample(const modIT_t &file, const uint32_t i)
	{ return new ModuleSampleNative(file, i); }

ModuleSampleNative::ModuleSampleNative(const modMOD_t &file, const uint32_t i) : ModuleSample(i, 1),
	Name{makeUnique<char []>(23)}, Length{}, InstrVol{64U}, LoopStart{}, LoopEnd{}, FileName{}, SamplePos{},
	Packing{}, Flags{}, SampleFlags{}, C4Speed{8363U}, DefaultPan{}, VibratoSpeed{}, VibratoDepth{},
	VibratoType{}, VibratoRate{}, SusLoopBegin{}, SusLoopEnd{}
{
	const auto &fd{file.fd()};
	uint16_t length16{};
	uint16_t loopStart16{};
	uint16_t loopEnd16{};

	if (!Name ||
		!fd.read(Name, 22U) ||
		!fd.readBE(length16) ||
		!fd.read(FineTune) ||
		!fd.read(Volume) ||
		!fd.readBE(loopStart16) ||
		!fd.readBE(loopEnd16))
		throw ModuleLoaderError{E_BAD_MOD};

	Length = length16 * 2U;
	LoopStart = loopStart16 * 2U;
	LoopEnd = loopEnd16 * 2U;
	if (Name[21] != 0)
		Name[22] = 0;
	if (Volume > 64U)
		Volume = 64U;
	FineTune &= 0x0FU;
	LoopEnd = LoopStart < Length && LoopEnd > 2U ? LoopStart + LoopEnd : 0U;

	if (LoopEnd != 0)
		SampleFlags |= SAMPLE_FLAGS_LOOP;
}

bool readLE24b(const fd_t &fd, uint32_t &dest) noexcept
{
	std::array<uint8_t, 3> data{};
	if (!fd.read(data))
		return false;
	dest = (uint32_t(data[2]) << 16U) | (uint32_t(data[1]) << 8U) | data[0];
	dest >>= 8U; // Format says all 24 bits are used, but S3M format spec is a liar and only [8:23] are used
	return true;
}

ModuleSampleNative::ModuleSampleNative(const modS3M_t &file, const uint32_t i, const uint8_t type) :
	ModuleSample(i, type), Name{makeUnique<char []>(29)}, FineTune{}, InstrVol{64U},
	FileName{makeUnique<char []>(13)}, SampleFlags{}, DefaultPan{}, VibratoSpeed{}, VibratoDepth{},
	VibratoType{}, VibratoRate{}, SusLoopBegin{}, SusLoopEnd{}
{
	const auto &fd{file.fd()};
	std::array<uint8_t, 12> dontCare{};
	std::array<char, 4> magic{};

	if (!Name || !FileName ||
		!fd.read(FileName, 12) ||
		!readLE24b(fd, SamplePos) ||
		!fd.readLE(Length) ||
		!fd.readLE(LoopStart) ||
		!fd.readLE(LoopEnd) ||
		!fd.read(Volume) ||
		!fd.read<1>(dontCare) ||
		!fd.read(Packing) ||
		!fd.read(Flags) ||
		!fd.read(C4Speed) ||
		!fd.read(dontCare) ||
		!fd.read(Name, 28) ||
		!fd.read(magic))
		throw ModuleLoaderError{E_BAD_S3M};

	if (_type == 1 && magic != s3mSampleMagic)
		throw ModuleLoaderError{E_BAD_S3M};

	if (FileName[11] != 0)
		FileName[12] = 0;
	if (Packing == 1)
		printf("%d => ADPCM sample\n", i);
	if (Flags & 2U)
		printf("%d => Stereo\n", i);
	if (Name[27] != 0)
		Name[28] = 0;
	// If looping not enabled, zero the Loop fields
	if (!(Flags & 1U))
		LoopStart = LoopEnd = 0;

	SampleFlags |= (Flags & 1U) ? SAMPLE_FLAGS_LOOP : 0;
	SampleFlags |= (Flags & 2U) ? SAMPLE_FLAGS_STEREO : 0;
	SampleFlags |= (Flags & 4U) ? SAMPLE_FLAGS_16BIT : 0;
}

ModuleSampleNative::ModuleSampleNative(const modSTM_t &file, const uint32_t i) : ModuleSample(i, 1),
	Name{makeUnique<char []>(13)}, FineTune{}, InstrVol{64}, FileName{}, SamplePos{}, Packing{},
	Flags{}, SampleFlags{}, DefaultPan{}, VibratoSpeed{}, VibratoDepth{}, VibratoType{}, VibratoRate{},
	SusLoopBegin{}, SusLoopEnd{}
{
	const auto &fd{file.fd()};
	uint8_t id{};
	uint8_t disk{};
	uint8_t reserved2{};
	uint16_t reserved1{};
	uint16_t c3Speed{};
	uint16_t unknown{};
	uint16_t length_{};
	uint16_t loopStart_{};
	uint16_t loopEnd_{};
	uint32_t reserved3{};

	if (!Name ||
		!fd.read(Name, 12) ||
		!fd.read(id) || id > 0 ||
		!fd.read(disk) ||
		!fd.read(reserved1) ||
		!fd.read(length_) ||
		!fd.read(loopStart_) ||
		!fd.read(loopEnd_) ||
		!fd.read(Volume) ||
		!fd.read(reserved2) ||
		!fd.read(c3Speed) ||
		!fd.read(reserved3) ||
		// XXX: One spec says this is the "Length in Paragraphs", need to figure out what that means.
		!fd.read(unknown))
		throw ModuleLoaderError(E_BAD_STM);

	Length = length_;
	LoopStart = loopStart_;
	LoopEnd = loopEnd_;
	// TODO: What's the diff. between them?
	C4Speed = c3Speed;
	if (Name[11] != 0)
		Name[12] = 0;
	if (LoopEnd < LoopStart || LoopEnd == 0xFFFFU)
	{
		LoopEnd = 0U;
		LoopStart = 0U;
	}
	else
		SampleFlags |= SAMPLE_FLAGS_LOOP;
	if (Volume > 64U)
		Volume = 64U;
}

ModuleSampleNative::ModuleSampleNative(const modAON_t &file, const uint32_t i, char *name, const uint32_t *const pcmLengths) : ModuleSample(i, 1), Name(name)
{
	uint8_t Type, ID;
	const fd_t &fd = file.fd();

	if (!fd.read(Type) ||
		!fd.read(Volume) ||
		!fd.read(FineTune) ||
		!fd.read(ID))
		throw ModuleLoaderError(E_BAD_AON);
	resetID(ID);
	Length = pcmLengths[ID];
	SampleFlags = 0;

	printf("%u(%u, %u) => %u, %u - ", ID, i, Type, Length, FineTune);
	if (Type == 0)
	{
		uint32_t SampleStart, SampleLen;
		VibratoSpeed = VibratoDepth = VibratoType = 0;

		if (!fd.readBE(SampleStart) ||
			!fd.readBE(SampleLen) ||
			!fd.readBE(LoopStart) ||
			!fd.readBE(LoopEnd))
			throw ModuleLoaderError(E_BAD_AON);
		SampleStart <<= 1;
		Length <<= 1;
		LoopStart <<= 1;
		LoopEnd <<= 1;
		if (LoopEnd == 2)
			LoopEnd = LoopStart = 0;
		LoopEnd += LoopStart;
		if (LoopEnd != LoopStart)
			SampleFlags |= SAMPLE_FLAGS_LOOP;
		printf("%u, %u, %u, %u(%u)", SampleStart, Length, LoopStart, LoopEnd, LoopEnd - LoopStart);
	}
	else if (Type == 1)
	{
		uint8_t LoopLen, Const;
		std::array<uint8_t, 9> reserved;
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

		if (!fd.read(LoopLen) ||
			!fd.read(reserved) ||
			!std::all_of(reserved.begin(), reserved.end(), [](const uint8_t value) { return value == 0; }) ||
			!fd.read(VibratoDepth) ||
			!fd.read(VibratoSpeed) ||
			!fd.read(VibratoType))
			throw ModuleLoaderError(E_BAD_AON);
		for (uint8_t i = 0; i < 4; ++i)
		{
			if (!fd.read(Const))
				throw ModuleLoaderError(E_BAD_AON);
			printf("%u, ", Const);
		}
		if (!fd.read(Const))
			throw ModuleLoaderError(E_BAD_AON);
		else if (Const == 1)
			SampleFlags |= SAMPLE_FLAGS_LREVERSE;
		else if (Const == 2)
			SampleFlags |= SAMPLE_FLAGS_LPINGPONG;
		for (uint8_t i = 0; i < 11; ++i)
		{
			if (!fd.read(Const))
				throw ModuleLoaderError(E_BAD_AON);
		}
		if (!fd.read(Const) ||
			!fd.read(Const) ||
			!fd.read(Const) ||
			!fd.read(Const))
			throw ModuleLoaderError(E_BAD_AON);

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
	InstrVol = 64;
}

ModuleSampleNative::ModuleSampleNative(const modIT_t &file, const uint32_t i) : ModuleSample(i, 1),
	Name{makeUnique<char []>(27)}, FineTune{}, FileName{makeUnique<char []>(13)}, SampleFlags{}
{
	const auto &fd{file.fd()};
	uint8_t _const{};
	std::array<char, 4> magic{};

	if (!fd.read(magic) ||
		strncmp(magic.data(), "IMPS", 4) != 0)
		throw ModuleLoaderError{E_BAD_IT};

	if (!FileName || !Name ||
		!fd.read(FileName, 12) ||
		!fd.read(_const) ||
		!fd.read(InstrVol) ||
		!fd.read(Flags) ||
		!fd.read(Volume) ||
		!fd.read(Name, 26) ||
		!fd.read(Packing) ||
		!fd.read(DefaultPan) ||
		!fd.readLE(Length) ||
		!fd.readLE(LoopStart) ||
		!fd.readLE(LoopEnd) ||
		!fd.readLE(C4Speed) ||
		!fd.readLE(SusLoopBegin) ||
		!fd.readLE(SusLoopEnd) ||
		!fd.readLE(SamplePos) ||
		!fd.read(VibratoSpeed) ||
		!fd.read(VibratoDepth) ||
		!fd.read(VibratoRate) ||
		!fd.read(VibratoType))
		throw ModuleLoaderError(E_BAD_IT);

	if (FileName[11])
		FileName[12] = 0;
	if (Volume > 64)
		Volume = 64;
	if (Name[25])
		Name[26] = 0;

	// ITTECH.TXT from Impulse Tracker 2.14v5 says VibratoType can be at most 3, however
	// we have run into several files in the wild where it is at least 4. MPT says 7 max.
	if (_const || Packing > 63U || VibratoSpeed > 64U || VibratoDepth > 64U ||
		VibratoType >= 8U || /*(VibratoType < 4U && VibratoRate > 64U) ||*/ InstrVol > 64U)
		throw ModuleLoaderError{E_BAD_IT};
	else if (VibratoRate > 64)
		VibratoRate = 64;

	if (C4Speed == 0U)
		C4Speed = 8363U;
	else if (C4Speed < 256U)
		C4Speed = 256U;
	/*else
		C4Speed /= 2;*/
	VibratoRate <<= 1U;

	// If looping not enabled, zero the Loop fields
	if (!(Flags & 0x10U))
		LoopStart = LoopEnd = 0U;

	SampleFlags |= (Flags & 0x10U) ? SAMPLE_FLAGS_LOOP : 0U;
	SampleFlags |= (Flags & 0x20U) ? SAMPLE_FLAGS_SUSTAINLOOP : 0U;
	SampleFlags |= (Flags & 0x04U) ? SAMPLE_FLAGS_STEREO : 0U;
	SampleFlags |= (Flags & 0x02U) ? SAMPLE_FLAGS_16BIT : 0U;
	SampleFlags |= (Flags & 0x40U) ? SAMPLE_FLAGS_LPINGPONG : 0U;
}

bool ModuleSampleNative::Get16Bit()
	{ return SampleFlags & SAMPLE_FLAGS_16BIT; }

bool ModuleSampleNative::GetStereo()
	{ return SampleFlags & SAMPLE_FLAGS_STEREO; }

bool ModuleSampleNative::GetLooped()
	{ return SampleFlags & SAMPLE_FLAGS_LOOP; }

bool ModuleSampleNative::GetSustainLooped()
	{ return SampleFlags & SAMPLE_FLAGS_SUSTAINLOOP; }

bool ModuleSampleNative::GetBidiLoop()
	{ return SampleFlags & SAMPLE_FLAGS_LPINGPONG; }

ModuleSampleAdlib::ModuleSampleAdlib(const modS3M_t &file, const uint32_t i, const uint8_t type) : ModuleSample(i, type)
{
	std::array<char, 4> magic;
	std::array<uint8_t, 12> dontCare;
	uint32_t zero;
	const fd_t &fd = file.fd();

	Name = new char[29];
	FileName = new char[13];

	if (!fd.read(FileName, 12) ||
		!readLE24b(fd, zero) || zero ||
		!fd.read(D00) ||
		!fd.read(D01) ||
		!fd.read(D02) ||
		!fd.read(D03) ||
		!fd.read(D04) ||
		!fd.read(D05) ||
		!fd.read(D06) ||
		!fd.read(D07) ||
		!fd.read(D08) ||
		!fd.read(D09) ||
		!fd.read(D0A) ||
		!fd.read(D0B) ||
		!fd.read(Volume) ||
		!fd.read(DONTKNOW) ||
		!fd.read<2>(dontCare) ||
		!fd.read(C4Speed) ||
		!fd.read(dontCare) ||
		!fd.read(Name, 28) ||
		!fd.read(magic) ||
		memcmp(magic.data(), "SCRI", 4) != 0)
		throw ModuleLoaderError(E_BAD_S3M);

	C4Speed = uint16_t(C4Speed);
	if (FileName[11] != 0)
		FileName[12] = 0;
	if (Name[27] != 0)
		Name[28] = 0;
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

uint32_t ModuleSampleAdlib::GetSustainLoopBegin()
{
	return 0;
}

uint32_t ModuleSampleAdlib::GetSustainLoopEnd()
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

uint8_t ModuleSampleAdlib::GetSampleVolume()
{
	return 0;
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

uint16_t ModuleSampleAdlib::GetPanning()
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

bool ModuleSampleAdlib::GetSustainLooped()
{
	return false;
}

bool ModuleSampleAdlib::GetBidiLoop()
{
	return false;
}

bool ModuleSampleAdlib::GetPanned()
{
	return false;
}
