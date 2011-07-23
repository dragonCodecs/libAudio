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
	CreationVersion = FormatVersion = 0;
	GlobalVolume = MasterVolume = 64;
	InitialSpeed = 6;
	InitialTempo = 125;
	SamplePtrs = PatternPtrs = NULL;
	Panning = NULL;
}

ModuleHeader::ModuleHeader(S3M_Intern *p_SF)
{
	char DontCare[10];
	char Magic[4];
	uint8_t Const;
	uint16_t Special;
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
	fread(&Flags, 2, 1, f_S3M);
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
}

ModuleHeader::~ModuleHeader()
{
	delete [] Panning;
	delete [] PatternPtrs;
	delete [] SamplePtrs;
	delete [] Orders;
	delete [] Name;
}
