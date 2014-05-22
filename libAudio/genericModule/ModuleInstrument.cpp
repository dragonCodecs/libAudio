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

uint16_t ModuleOldInstrument::GetFadeOut() const
{
	return 0;
}

bool ModuleOldInstrument::GetEnvEnabled(uint8_t env) const
{
	return false;
}

bool ModuleOldInstrument::GetEnvLooped(uint8_t env) const
{
	return false;
}

ModuleEnvelope *ModuleOldInstrument::GetEnvelope(uint8_t env) const
{
	return NULL;
}

ModuleNewInstrument::ModuleNewInstrument(IT_Intern *p_IT, uint32_t i) : ModuleInstrument(i)
{
	uint8_t Const, env;
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

	Envelopes = new ModuleEnvelope *[3];
	for (env = 0; env < 3; env++)
		Envelopes[env] = new ModuleEnvelope(p_IT, env);
}

ModuleNewInstrument::~ModuleNewInstrument()
{
	uint8_t i;
	for (i = 0; i < 3; i++)
		delete Envelopes[i];
	delete [] Envelopes;
	delete [] Name;
	delete [] FileName;
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

uint16_t ModuleNewInstrument::GetFadeOut() const
{
	return FadeOut;
}

bool ModuleNewInstrument::GetEnvEnabled(uint8_t env) const
{
	return Envelopes[env]->GetEnabled();
}

bool ModuleNewInstrument::GetEnvLooped(uint8_t env) const
{
	return Envelopes[env]->GetLooped();
}

ModuleEnvelope *ModuleNewInstrument::GetEnvelope(uint8_t env) const
{
	return Envelopes[env];
}

ModuleEnvelope::ModuleEnvelope(IT_Intern *p_IT, uint8_t env) : Type(env)
{
	uint8_t DontCare;
	FILE *f_IT = p_IT->f_IT;

	fread(&Flags, 1, 1, f_IT);
	fread(&nNodes, 1, 1, f_IT);
	fread(&LoopBegin, 1, 1, f_IT);
	fread(&LoopEnd, 1, 1, f_IT);
	fread(&SusLoopBegin, 1, 1, f_IT);
	fread(&SusLoopEnd, 1, 1, f_IT);
	fread(Nodes, 75, 1, f_IT);
	fread(&DontCare, 1, 1, f_IT);
}

int16_t ModuleEnvelope::Apply(uint8_t Tick)
{
	uint8_t pt, n1, n2;
	int16_t ret;
	for (pt = 0; pt < (nNodes - 1); pt++)
	{
		if (Tick <= Nodes[pt].Tick)
			break;
	}
	if (Tick >= Nodes[pt].Tick)
		return Nodes[pt].Value;
	if (pt != 0)
	{
		n1 = Nodes[pt - 1].Tick;
		ret = Nodes[pt - 1].Value;
	}
	else
	{
		n1 = 0;
		ret = 0;
	}
	n2 = Nodes[pt].Tick;
	if (n2 > n1 && Tick > n1)
	{
		int16_t val = pt - n1;
		val *= ((int16_t)Nodes[pt].Value) - ret;
		n2 -= n1;
		return ret + (val / n2);
	}
	else
		return ret;
}

bool ModuleEnvelope::GetEnabled() const
{
	return (Flags & 0x01) != 0;
}

bool ModuleEnvelope::GetLooped() const
{
	return (Flags & 0x02) != 0;
}

bool ModuleEnvelope::HasNodes() const
{
	return nNodes != 0;
}
