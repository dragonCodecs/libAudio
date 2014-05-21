#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"

ModuleHeader::ModuleHeader(MOD_Intern *p_MF)
{
	char MODMagic[4];
	FILE *f_MOD = p_MF->f_MOD;

	Name = new char[21];
	fread(Name, 20, 1, f_MOD);
	if (Name[19] != 0)
		Name[20] = 0;

	// Defaults
	nSamples = 31;
	nChannels = 4;
	// Now find out the actual values
	fseek(f_MOD, (30 * 31) + 130, SEEK_CUR);
	fread(MODMagic, 4, 1, f_MOD);
	if (strncmp(MODMagic, "M.K.", 4) == 0 || strncmp(MODMagic, "M!K!", 4) == 0 ||
		strncmp(MODMagic, "M&K!", 4) == 0 || strncmp(MODMagic, "N.T.", 4) == 0)
		nChannels = 4;
	else if (strncmp(MODMagic, "CD81", 4) == 0 || strncmp(MODMagic, "OKTA", 4) == 0)
		nChannels = 8;
	else if (strncmp(MODMagic, "FLT", 3) == 0 && MODMagic[3] >= '4' && MODMagic[3] <= '9')
		nChannels = MODMagic[3] - '0';
	else if (strncmp(MODMagic + 1, "CHN", 3) == 0 && MODMagic[0] >= '4' && MODMagic[0] <= '9')
		nChannels = MODMagic[0] - '0';
	else if (strncmp(MODMagic + 2, "CH", 2) == 0 && MODMagic[0] == '1' && MODMagic[1] >= '0' && MODMagic[1] <= '9')
		nChannels = MODMagic[1] - '0' + 10;
	else if (strncmp(MODMagic + 2, "CH", 2) == 0 && MODMagic[0] == '2' && MODMagic[1] >= '0' && MODMagic[1] <= '9')
		nChannels = MODMagic[1] - '0' + 20;
	else if (strncmp(MODMagic + 2, "CH", 2) == 0 && MODMagic[0] == '3' && MODMagic[1] >= '0' && MODMagic[1] <= '9')
		nChannels = MODMagic[1] - '0' + 30;
	else if (strncmp(MODMagic, "TDZ", 3) == 0 && MODMagic[3] >= '4' && MODMagic[3] <= '9')
		nChannels = MODMagic[3] - '0';
	else if (strncmp(MODMagic, "16CN", 4) == 0)
		nChannels = 16;
	else if (strncmp(MODMagic, "32CN", 4) == 0)
		nChannels = 32;
	else
		nSamples = 15;

	fseek(f_MOD, -134, SEEK_CUR);
	nOrders = 0;
	fread(&nOrders, 1, 1, f_MOD);
	fread(&RestartPos, 1, 1, f_MOD);
	Orders = new uint8_t[128];
	fread(Orders, 128, 1, f_MOD);
	if (nOrders > 128)
		nOrders = 128;
	if (RestartPos > 127)
		RestartPos = 127;

	/********************************************\
	|* The following block just initialises the *|
	|* unused fields to harmless values.        *|
	\********************************************/
	Type = 0;
	Flags = 0;
	nInstruments = 0;
	CreationVersion = FormatVersion = 0;
	GlobalVolume = MasterVolume = 64;
	Separation = 128;
	InitialSpeed = 6;
	InitialTempo = 125;
	SamplePtrs = PatternPtrs = InstrumentPtrs = NULL;
	Panning = NULL;
	Author = NULL;
	Remark = NULL;
}

ModuleHeader::ModuleHeader(S3M_Intern *p_SF)
{
	char DontCare[10];
	char Magic[4];
	uint8_t Const;
	uint16_t Special, RawFlags;
	FILE *f_S3M = p_SF->f_S3M;

	Name = new char[29];
	fread(Name, 28, 1, f_S3M);
	if (Name[27] != 0)
		Name[28] = 0;
	fread(&Const, 1, 1, f_S3M);
	fread(&Type, 1, 1, f_S3M);
	fread(DontCare, 2, 1, f_S3M);
	fread(&nOrders, 2, 1, f_S3M);
	fread(&nSamples, 2, 1, f_S3M);
	fread(&nPatterns, 2, 1, f_S3M);
	fread(&RawFlags, 2, 1, f_S3M);
	fread(&CreationVersion, 2, 1, f_S3M);
	fread(&FormatVersion, 2, 1, f_S3M);
	fread(Magic, 4, 1, f_S3M);
	fread(&GlobalVolume, 1, 1, f_S3M);
	fread(&InitialSpeed, 1, 1, f_S3M);
	fread(&InitialTempo, 1, 1, f_S3M);
	fread(&MasterVolume, 1, 1, f_S3M);
	fread(DontCare, 10, 1, f_S3M);
	fread(&Special, 2, 1, f_S3M);
	fread(ChannelSettings, 32, 1, f_S3M);

	if (Const != 0x1A || Type != 16 || FormatVersion > 2 || FormatVersion == 0 ||
		memcmp(Magic, "SCRM", 4) != 0)
		throw new ModuleLoaderError(E_BAD_S3M);

	if ((RawFlags & 0x04) != 0)
		Flags |= FILE_FLAGS_AMIGA_SLIDES;
	if ((RawFlags & 0x10) != 0)
		Flags |= FILE_FLAGS_AMIGA_LIMITS;

	Orders = new uint8_t[nOrders];
	fread(Orders, nOrders, 1, f_S3M);
	SamplePtrs = new uint16_t[nSamples];
	fread(SamplePtrs, nSamples, 2, f_S3M);
	PatternPtrs = new uint16_t[nPatterns];
	fread(PatternPtrs, nPatterns, 2, f_S3M);

	// Panning?
	if (DontCare[1] == 0xFC)
	{
		uint32_t i;
		Panning = new uint8_t[32];
		fread(Panning, 32, 1, f_S3M);
		for (i = 0; i < 32; i++)
		{
			if ((Panning[i] & 0x20) != 0)
				Panning[i] = (Panning[i] & 0x0F) << 4;
		}
	}
	else
		Panning = NULL;

	/********************************************\
	|* The following block just initialises the *|
	|* unused fields to harmless values.        *|
	\********************************************/
	RestartPos = 255;
	Separation = 128;
	nInstruments = 0;
	InstrumentPtrs = NULL;
	Author = NULL;
	Remark = NULL;
}

ModuleHeader::ModuleHeader(STM_Intern *p_SF)
{
	char Const[9];
	char Reserved[13];
	uint8_t _nPatterns, i;
	FILE *f_STM = p_SF->f_STM;

	Name = new char[21];
	fread(Name, 20, 1, f_STM);
	if (Name[19] != 0)
		Name[20] = 0;
	fread(Const, 9, 1, f_STM);
	fread(&Type, 1, 1, f_STM);
	fread(&FormatVersion, 2, 1, f_STM);
	fread(&InitialSpeed, 1, 1, f_STM);
	InitialSpeed >>= 4;
	fread(&_nPatterns, 1, 1, f_STM);
	nPatterns = _nPatterns;
	fread(&GlobalVolume, 1, 1, f_STM);
	fread(Reserved, 13, 1, f_STM);

	if (strncmp(Const, "!Scream!\x1A", 9) != 0 || Type != 2)
		throw new ModuleLoaderError(E_BAD_STM);

	nOrders = 128;
	Orders = new uint8_t[128];
	fseek(f_STM, 1040, SEEK_SET);
	fread(Orders, 128, 1, f_STM);
	for (i = 0; i < nOrders; i++)
	{
		if (Orders[i] >= 99)
			Orders[i] = 255;
	}
	fseek(f_STM, 48, SEEK_SET);
	nSamples = 31;
	nChannels = 4;

	/********************************************\
	|* The following block just initialises the *|
	|* unused fields to harmless values.        *|
	\********************************************/
	Flags = 0;
	MasterVolume = 64;
	Separation = 128;
	RestartPos = 255;
	InitialTempo = 125;
	nInstruments = 0;
	SamplePtrs = PatternPtrs = InstrumentPtrs = NULL;
	Author = NULL;
	Remark = NULL;
}

ModuleHeader::ModuleHeader(AON_Intern *p_AF)
{
	char Magic1[4], Magic2[42];
	char StrMagic[4];
	uint32_t BlockLen;
	uint8_t Const, i;
	FILE *f_AON = p_AF->f_AON;

	fread(Magic1, 4, 1, f_AON);
	fread(Magic2, 42, 1, f_AON);
	fread(StrMagic, 4, 1, f_AON);
	fread(&BlockLen, 4, 1, f_AON);
	BlockLen = Swap32(BlockLen);

	if (strncmp(Magic1, "AON4", 4) != 0 && strncmp(Magic1, "AON8", 4) != 0 &&
		strncmp(Magic2, "artofnoise by bastian spiegel (twice/lego)", 42) != 0 &&
		strncmp(StrMagic, "NAME", 4) != 0)
		throw new ModuleLoaderError(E_BAD_AON);

	nChannels = Magic1[3] - '0';

	Name = new char[BlockLen + 1];
	fread(Name, BlockLen, 1, f_AON);
	Name[BlockLen] = 0;

	fread(StrMagic, 4, 1, f_AON);
	if (strncmp(StrMagic, "AUTH", 4) != 0)
		throw new ModuleLoaderError(E_BAD_AON);
	fread(&BlockLen, 4, 1, f_AON);
	BlockLen = Swap32(BlockLen);
	Author = new char [BlockLen + 1];
	fread(Author, BlockLen, 1, f_AON);
	Author[BlockLen] = 0;

	fread(StrMagic, 4, 1, f_AON);
	if (strncmp(StrMagic, "DATE", 4) != 0)
		throw new ModuleLoaderError(E_BAD_AON);
	fread(&BlockLen, 4, 1, f_AON);
	BlockLen = Swap32(BlockLen);
	fseek(f_AON, BlockLen, SEEK_CUR);

	fread(StrMagic, 4, 1, f_AON);
	if (strncmp(StrMagic, "RMRK", 4) != 0)
		throw new ModuleLoaderError(E_BAD_AON);
	fread(&BlockLen, 4, 1, f_AON);
	if (BlockLen != 0)
	{
		BlockLen = Swap32(BlockLen);
		Remark = new char[BlockLen + 1];
		fread(Remark, BlockLen, 1, f_AON);
		Remark[BlockLen] = 0;
	}
	else
		Remark = NULL;

	fread(StrMagic, 4, 1, f_AON);
	fread(&BlockLen, 4, 1, f_AON);
	fread(&Const, 1, 1, f_AON);
	if (strncmp(StrMagic, "INFO", 4) != 0 || BlockLen != Swap32(4) ||
		Const != 0x34)
		throw new ModuleLoaderError(E_BAD_AON);
	fread(&Const, 1, 1, f_AON);
	nOrders = Const;
	fread(&Const, 1, 1, f_AON);
	RestartPos = Const;
	fread(&Const, 1, 1, f_AON);

	// Skip over the arpeggio table..
	fread(StrMagic, 4, 1, f_AON);
	fread(&BlockLen, 4, 1, f_AON);
	if (strncmp(StrMagic, "ARPG", 4) != 0 || BlockLen != Swap32(64))
		throw new ModuleLoaderError(E_BAD_AON);
	for (i = 0; i < 16; i++)
	{
		for (int j = 0; j < 4; j++)
			fread(&ArpTable[i][j], 1, 1, f_AON);
	}
	if (ArpTable[0][0] != 0 || ArpTable[0][1] != 0 ||
		ArpTable[0][2] != 0 || ArpTable[0][3] != 0)
		throw new ModuleLoaderError(E_BAD_AON);

	fread(StrMagic, 4, 1, f_AON);
	fread(&BlockLen, 4, 1, f_AON);
	BlockLen = Swap32(BlockLen);
	// If odd number of orders
	if ((nOrders & 1) != 0)
		// Get rid of fill byte from count
		BlockLen--;
	if (strncmp(StrMagic, "PLST", 4) != 0 || BlockLen != nOrders)
		throw new ModuleLoaderError(E_BAD_AON);
	Orders = new uint8_t[nOrders];
	for (i = 0; i < nOrders; i++)
		fread(&Orders[i], 1, 1, f_AON);
	// If odd read length
	if ((BlockLen & 1) != 0)
		// Read the fill-byte
		fread(&Const, 1, 1, f_AON);

	/********************************************\
	|* The following block just initialises the *|
	|* unused fields to harmless values.        *|
	\********************************************/
	Type = 0;
	Flags = 0;
	CreationVersion = FormatVersion = 0;
	GlobalVolume = MasterVolume = 64;
	Separation = 128;
	InitialSpeed = 6;
	InitialTempo = 125;
	SamplePtrs = PatternPtrs = InstrumentPtrs = NULL;
	Panning = NULL;
}

#ifdef __FC1x_EXPERIMENTAL__
ModuleHeader::ModuleHeader(FC1x_Intern *p_FF)
{
	char Magic[4];
	uint32_t IDK1, IDK2;
	uint32_t Special;
	FILE *f_FC1x = p_FF->f_FC1x;

	fread(Magic, 4, 1, f_FC1x);
	fread(&SeqLength, 4, 1, f_FC1x);
	fread(&PatternOffs, 4, 1, f_FC1x);
	fread(&PatLength, 4, 1, f_FC1x);
	fread(&IDK1, 4, 1, f_FC1x);
	fread(&IDK2, 4, 1, f_FC1x);
	fread(&IDK1, 4, 1, f_FC1x);
	fread(&IDK2, 4, 1, f_FC1x);
	fread(&SampleOffs, 4, 1, f_FC1x);
	fread(&Special, 4, 1, f_FC1x);

	if (strncmp(Magic, "SMOD", 4) != 0 && strncmp(Magic, "FC14", 4) != 0)
		throw new ModuleLoaderError(E_BAD_FC1x);

	//

	/********************************************\
	|* The following block just initialises the *|
	|* unused fields to harmless values.        *|
	\********************************************/
	Name = NULL;
	RestartPos = 255;
	nInstruments = 0;
	Separation = 128;
	SamplePtrs = PatternPtrs = InstrumentPtrs = NULL;
	Author = NULL;
	Remark = NULL;
}
#endif

ModuleHeader::ModuleHeader(IT_Intern *p_IF) : Remark(NULL)
{
	char Magic[4], DontCare[4];
	uint16_t MsgLength;
	uint8_t Const;
	FILE *f_IT = p_IF->f_IT;

	fread(Magic, 4, 1, f_IT);
	if (strncmp(Magic, "IMPM", 4) != 0)
		throw new ModuleLoaderError(E_BAD_IT);

	Name = new char[27];
	fread(Name, 26, 1, f_IT);
	if (Name[25] != 0)
		Name[26] = 0;

	fread(DontCare, 2, 1, f_IT);
	fread(&nOrders, 2, 1, f_IT);
	fread(&nInstruments, 2, 1, f_IT);
	fread(&nSamples, 2, 1, f_IT);
	fread(&nPatterns, 2, 1, f_IT);
	fread(&CreationVersion, 2, 1, f_IT);
	fread(&FormatVersion, 2, 1, f_IT);
	fread(&Flags, 2, 1, f_IT);
	// TODO: Handle special.
	fread(DontCare, 2, 1, f_IT);
	fread(&GlobalVolume, 1, 1, f_IT);
	fread(&MasterVolume, 1, 1, f_IT);
	fread(&InitialSpeed, 1, 1, f_IT);
	fread(&InitialTempo, 1, 1, f_IT);
	fread(&Separation, 1, 1, f_IT);
	fread(&Const, 1, 1, f_IT);

	if (Const != 0)
		throw new ModuleLoaderError(E_BAD_IT);

	fread(&MsgLength, 2, 1, f_IT);
	fread(&MessageOffs, 4, 1, f_IT);
	fread(&DontCare, 4, 1, f_IT);

	if (MessageOffs != 0)
	{
		size_t PartialHeader = ftell(f_IT);
		fseek(f_IT, SEEK_SET, MessageOffs);
		Remark = new char[MsgLength + 1];
		fread(Remark, MsgLength, 1, f_IT);
		fseek(f_IT, SEEK_SET, PartialHeader);
		Remark[MsgLength] = 0;
	}

	Panning = new uint8_t[64];
	fread(Panning, 64, 1, f_IT);
	//Volumes = new uint8_t[64];
	fread(Volumes, 64, 1, f_IT);

	Orders = new uint8_t[nOrders];
	fread(Orders, nOrders, 1, f_IT);

	InstrumentPtrs = new uint32_t[nInstruments];
	fread(InstrumentPtrs, nInstruments, 4, f_IT);
	SamplePtrs = new uint32_t[nSamples];
	fread(SamplePtrs, nSamples, 4, f_IT);
	PatternPtrs = new uint32_t[nPatterns];
	fread(PatternPtrs, nPatterns, 4, f_IT);

	Flags = 0;
	RestartPos = 255;
	Separation = 128;
	Author = NULL;
}

ModuleHeader::~ModuleHeader()
{
	delete [] Panning;
	delete [] (uint16_t *)InstrumentPtrs;
	delete [] (uint16_t *)PatternPtrs;
	delete [] (uint16_t *)SamplePtrs;
	delete [] Orders;
	delete [] Name;
	delete [] Author;
	delete [] Remark;
}
