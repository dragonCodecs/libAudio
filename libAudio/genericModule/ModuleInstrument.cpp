#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"

ModuleInstrument::ModuleInstrument(uint32_t id) : ID(id)
{
}

ModuleInstrument *ModuleInstrument::LoadInstrument(IT_Intern *p_IT, uint32_t i, uint16_t FormatVersion)
{
	if (FormatVersion < 0x0200)
		return new ModuleOldInstrument(p_IT, i);
	else
		return new ModuleNewInstrument(p_IT, i);
}

ModuleOldInstrument::ModuleOldInstrument(IT_Intern *p_IT, uint32_t i) : ModuleInstrument(i)
{
}

uint8_t ModuleOldInstrument::Map(uint8_t Note)
{
	return 0;
}

ModuleNewInstrument::ModuleNewInstrument(IT_Intern *p_IT, uint32_t i) : ModuleInstrument(i)
{
	uint8_t Const;
	char Magic[4], DontCare[6];
	FILE *f_IT = p_IT->f_IT;

	fread(Magic, 4, 1, f_IT);
	if (strncmp(Magic, "IMPI", 4) != 0)
		throw new ModuleLoaderError(E_BAD_IT);
	FileName = new char[13];
	fread(FileName, 12, 1, f_IT);
	if (FileName[11] != 0)
		FileName[12] = 0;
	fread(&Const, 1, 1, f_IT);
	fread(&NNA, 1, 1, f_IT);
	fread(&DCT, 1, 1, f_IT);
	fread(&DCA, 1, 1, f_IT);
	fread(&FadeOut, 2, 1, f_IT);
	fread(&PPS, 1, 1, f_IT);
	fread(&PPC, 1, 1, f_IT);
	fread(&Volume, 1, 1, f_IT);
	fread(&Panning, 1, 1, f_IT);
	fread(&RandVolume, 1, 1, f_IT);
	fread(&RandPanning, 1, 1, f_IT);
	fread(&TrackerVersion, 2, 1, f_IT);
	fread(&nSamples, 1, 1, f_IT);
	fread(DontCare, 1, 1, f_IT);
	Name = new char[27];
	fread(Name, 26, 1, f_IT);
	if (Name[25] != 0)
		Name[26] = 0;
	fread(DontCare, 6, 1, f_IT);
	fread(SampleMapping, 240, 1, f_IT);

	if (Const != 0 || NNA > 3 || DCT > 3 || DCA > 2)
		throw new ModuleLoaderError(E_BAD_IT);
}

uint8_t ModuleNewInstrument::Map(uint8_t Note)
{
	uint16_t i;
	uint8_t sample = 0;
	for (i = 0; i < 120; i++)
	{
		if (SampleMapping[i << 1] <= Note)
			sample = SampleMapping[(i << 1) + 1];
	}
	return sample;
}
