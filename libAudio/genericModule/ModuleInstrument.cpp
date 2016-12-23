#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"

ModuleInstrument *ModuleInstrument::LoadInstrument(const modIT_t &file, uint32_t i, uint16_t FormatVersion)
{
	if (FormatVersion < 0x0200)
		return new ModuleOldInstrument(file, i);
	else
		return new ModuleNewInstrument(file, i);
}

ModuleOldInstrument::ModuleOldInstrument(const modIT_t &file, uint32_t i) : ModuleInstrument(i)
{
	uint8_t LoopBegin, LoopEnd;
	uint8_t SusLoopBegin, SusLoopEnd;
	uint8_t Const;
	char Magic[4], DontCare[6];
	const fd_t &fd = file.fd();

	fd.read(Magic, 4);
	if (strncmp(Magic, "IMPI", 4) != 0)
		throw new ModuleLoaderError(E_BAD_IT);
	FileName = new char[13];
	fd.read(FileName, 12);
	if (FileName[11] != 0)
		FileName[12] = 0;
	fd.read(&Const, 1);
	fd.read(&Flags, 1);
	fd.read(&LoopBegin, 1);
	fd.read(&LoopEnd, 1);
	fd.read(&SusLoopBegin, 1);
	fd.read(&SusLoopEnd, 1);
	fd.read(DontCare, 2);
	fd.read(&FadeOut, 2);
	fd.read(&NNA, 1);
	fd.read(&DNC, 1);
	fd.read(&TrackerVersion, 2);
	fd.read(&nSamples, 1);
	fd.read(DontCare, 1);
	Name = new char[27];
	fd.read(Name, 26);
	if (Name[25] != 0)
		Name[26] = 0;
	fd.read(DontCare, 6);
	fd.read(SampleMapping, 240);

	if (Const != 0 || NNA > 3 || DNC > 1)
		throw new ModuleLoaderError(E_BAD_IT);

	Envelope = new ModuleEnvelope(file, Flags, LoopBegin, LoopEnd, SusLoopBegin, SusLoopEnd);
}

ModuleOldInstrument::~ModuleOldInstrument()
{
	delete Envelope;
	delete [] Name;
	delete [] FileName;
}

uint8_t ModuleOldInstrument::Map(uint8_t /*Note*/)
{
	return 0;
}

uint16_t ModuleOldInstrument::GetFadeOut() const
{
	return 0;
}

bool ModuleOldInstrument::GetEnvEnabled(uint8_t /*env*/) const
{
	return false;
}

bool ModuleOldInstrument::GetEnvLooped(uint8_t /*env*/) const
{
	return false;
}

ModuleEnvelope *ModuleOldInstrument::GetEnvelope(uint8_t /*env*/) const
{
	return NULL;
}

bool ModuleOldInstrument::IsPanned() const
{
	return false;
}

bool ModuleOldInstrument::HasVolume() const
{
	return false;
}

uint8_t ModuleOldInstrument::GetPanning() const
{
	return 0;
}

uint8_t ModuleOldInstrument::GetVolume() const
{
	return 0;
}

uint8_t ModuleOldInstrument::GetNNA() const
{
	return NNA;
}

uint8_t ModuleOldInstrument::GetDCT() const
{
	return DCT_OFF;
}

uint8_t ModuleOldInstrument::GetDNA() const
{
	return DNA_NOTECUT;
}

ModuleNewInstrument::ModuleNewInstrument(const modIT_t &file, uint32_t i) : ModuleInstrument(i)
{
	uint8_t Const;
	char Magic[4], DontCare[6];
	const fd_t &fd = file.fd();

	fd.read(Magic, 4);
	if (strncmp(Magic, "IMPI", 4) != 0)
		throw new ModuleLoaderError(E_BAD_IT);
	FileName = new char[13];
	fd.read(FileName, 12);
	if (FileName[11] != 0)
		FileName[12] = 0;
	fd.read(&Const, 1);
	fd.read(&NNA, 1);
	fd.read(&DCT, 1);
	fd.read(&DNA, 1);
	fd.read(&FadeOut, 2);
	fd.read(&PPS, 1);
	fd.read(&PPC, 1);
	fd.read(&Volume, 1);
	fd.read(&Panning, 1);
	fd.read(&RandVolume, 1);
	fd.read(&RandPanning, 1);
	fd.read(&TrackerVersion, 2);
	fd.read(&nSamples, 1);
	fd.read(DontCare, 1);
	Name = new char[27];
	fd.read(Name, 26);
	if (Name[25] != 0)
		Name[26] = 0;
	fd.read(DontCare, 6);
	fd.read(SampleMapping, 240);

	if (Const != 0 || NNA > 3 || DCT > 3 || DNA > 2)
		throw new ModuleLoaderError(E_BAD_IT);

	FadeOut <<= 6;
	Envelopes = new ModuleEnvelope *[3];
	for (uint8_t env = 0; env < 3; env++)
		Envelopes[env] = new ModuleEnvelope(file, env);
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
	uint8_t sample = 0;
	for (uint16_t i = 0; i < 120; i++)
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

bool ModuleNewInstrument::IsPanned() const
{
	return (Panning & 128) == 0;
}

bool ModuleNewInstrument::HasVolume() const
{
	return true;
}

uint8_t ModuleNewInstrument::GetPanning() const
{
	return (Panning & 0x7F) << 1;
}

uint8_t ModuleNewInstrument::GetVolume() const
{
	return Volume & 0x7F;
}

uint8_t ModuleNewInstrument::GetNNA() const
{
	return NNA;
}

uint8_t ModuleNewInstrument::GetDCT() const
{
	return DCT;
}

uint8_t ModuleNewInstrument::GetDNA() const
{
	return DNA;
}

ModuleEnvelope::ModuleEnvelope(const modIT_t &file, uint8_t env) : Type(env)
{
	uint8_t DontCare;
	const fd_t &fd = file.fd();

	fd.read(&Flags, 1);
	fd.read(&nNodes, 1);
	fd.read(&LoopBegin, 1);
	fd.read(&LoopEnd, 1);
	fd.read(&SusLoopBegin, 1);
	fd.read(&SusLoopEnd, 1);
	fd.read(Nodes, 75);
	fd.read(&DontCare, 1);

	if (LoopBegin > nNodes || LoopEnd > nNodes)
		throw new ModuleLoaderError(E_BAD_IT);

	if (env != ENVELOPE_VOLUME)
	{
		// This requires signed/unsigned conversion
		for (uint8_t i = 0; i < nNodes; i++)
			Nodes[i].Value ^= 0x80;
	}
}

ModuleEnvelope::ModuleEnvelope(const modIT_t &, const uint8_t flags, const uint8_t loopBegin,
	const uint8_t loopEnd, const uint8_t susLoopBegin, const uint8_t susLoopEnd) noexcept :
	Type(0), Flags(flags), LoopBegin(loopBegin), LoopEnd(loopEnd), SusLoopBegin(susLoopBegin), SusLoopEnd(susLoopEnd)
{
}

uint8_t ModuleEnvelope::Apply(uint16_t Tick)
{
	uint8_t pt, n1, n2, ret;
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
		int16_t val = Tick - n1;
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

bool ModuleEnvelope::GetSustained() const
{
	return (Flags & 0x04) != 0;
}

bool ModuleEnvelope::HasNodes() const
{
	return nNodes != 0;
}

bool ModuleEnvelope::IsAtEnd(uint16_t Tick) const
{
	return Tick > GetLastTick();
}

bool ModuleEnvelope::IsZeroLoop() const
{
	return LoopEnd == LoopBegin;
}

uint16_t ModuleEnvelope::GetLoopEnd() const
{
	return Nodes[LoopEnd].Tick;
}

uint16_t ModuleEnvelope::GetLoopBegin() const
{
	return Nodes[LoopBegin].Tick;
}

uint16_t ModuleEnvelope::GetSustainEnd() const
{
	return Nodes[SusLoopBegin].Tick;
}

uint16_t ModuleEnvelope::GetSustainBegin() const
{
	return Nodes[SusLoopBegin].Tick;
}

uint16_t ModuleEnvelope::GetLastTick() const
{
	return Nodes[nNodes - 1].Tick;
}
