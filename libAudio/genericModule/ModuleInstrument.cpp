#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"

ModuleInstrument *ModuleInstrument::LoadInstrument(const modIT_t &file, const uint32_t i, const uint16_t FormatVersion)
{
	if (FormatVersion < 0x0200)
		return new ModuleOldInstrument(file, i);
	else
		return new ModuleNewInstrument(file, i);
}

ModuleOldInstrument::ModuleOldInstrument(const modIT_t &file, const uint32_t i) : ModuleInstrument(i)
{
	uint8_t LoopBegin, LoopEnd;
	uint8_t SusLoopBegin, SusLoopEnd;
	uint8_t Const;
	char DontCare[6];
	std::array<char, 4> magic;
	const fd_t &fd = file.fd();

	if (!fd.read(magic) ||
		strncmp(magic.data(), "IMPI", 4) != 0)
		throw ModuleLoaderError(E_BAD_IT);

	FileName = makeUnique<char []>(13);
	Name = makeUnique<char []>(27);

	if (!FileName || !Name ||
		!fd.read(FileName, 12) ||
		!fd.read(&Const, 1) ||
		!fd.read(&Flags, 1) ||
		!fd.read(&LoopBegin, 1) ||
		!fd.read(&LoopEnd, 1) ||
		!fd.read(&SusLoopBegin, 1) ||
		!fd.read(&SusLoopEnd, 1) ||
		!fd.read(DontCare, 2) ||
		!fd.read(&FadeOut, 2) ||
		!fd.read(&NNA, 1) ||
		!fd.read(&DNC, 1) ||
		!fd.read(&TrackerVersion, 2) ||
		!fd.read(&nSamples, 1) ||
		!fd.read(DontCare, 1) ||
		!fd.read(Name, 26) ||
		!fd.read(DontCare, 6) ||
		!fd.read(SampleMapping, 240))
		throw ModuleLoaderError(E_BAD_IT);

	if (FileName[11] != 0)
		FileName[12] = 0;
	if (Name[25] != 0)
		Name[26] = 0;

	if (Const != 0 || NNA > 3 || DNC > 1)
		throw ModuleLoaderError(E_BAD_IT);

	Envelope = makeUnique<ModuleEnvelope>(file, Flags, LoopBegin, LoopEnd, SusLoopBegin, SusLoopEnd);
}

uint8_t ModuleOldInstrument::Map(uint8_t /*Note*/) noexcept
{
	return 0;
}

uint16_t ModuleOldInstrument::GetFadeOut() const noexcept { return FadeOut; }
bool ModuleOldInstrument::GetEnvEnabled(uint8_t env) const noexcept
	{ return !env ? (Flags & 0x01) : false; }
bool ModuleOldInstrument::GetEnvLooped(uint8_t env) const noexcept
	{ return !env ? (Flags & 0x02) : false; }
ModuleEnvelope *ModuleOldInstrument::GetEnvelope(uint8_t env) const noexcept
	{ return !env ? Envelope.get() : nullptr; }
bool ModuleOldInstrument::IsPanned() const noexcept { return false; }
bool ModuleOldInstrument::HasVolume() const noexcept { return false; }
uint8_t ModuleOldInstrument::GetPanning() const noexcept { return 0; }
uint8_t ModuleOldInstrument::GetVolume() const noexcept { return 0; }
uint8_t ModuleOldInstrument::GetNNA() const noexcept { return NNA; }
uint8_t ModuleOldInstrument::GetDCT() const noexcept { return DCT_OFF; }
uint8_t ModuleOldInstrument::GetDNA() const noexcept { return DNA_NOTECUT; }

ModuleNewInstrument::ModuleNewInstrument(const modIT_t &file, const uint32_t i) : ModuleInstrument(i)
{
	uint8_t Const;
	char DontCare[6];
	std::array<char, 4> magic;
	const fd_t &fd = file.fd();

	if (!fd.read(magic) ||
		strncmp(magic.data(), "IMPI", 4) != 0)
		throw ModuleLoaderError(E_BAD_IT);

	FileName = makeUnique<char []>(13);
	Name = makeUnique<char []>(27);

	if (!FileName || !Name ||
		!fd.read(FileName, 12) ||
		!fd.read(&Const, 1) ||
		!fd.read(&NNA, 1) ||
		!fd.read(&DCT, 1) ||
		!fd.read(&DNA, 1) ||
		!fd.read(&FadeOut, 2) ||
		!fd.read(&PPS, 1) ||
		!fd.read(&PPC, 1) ||
		!fd.read(&Volume, 1) ||
		!fd.read(&Panning, 1) ||
		!fd.read(&RandVolume, 1) ||
		!fd.read(&RandPanning, 1) ||
		!fd.read(&TrackerVersion, 2) ||
		!fd.read(&nSamples, 1) ||
		!fd.read(DontCare, 1) ||
		!fd.read(Name, 26) ||
		!fd.read(DontCare, 6) ||
		!fd.read(SampleMapping, 240))
		throw ModuleLoaderError(E_BAD_IT);

	if (FileName[11] != 0)
		FileName[12] = 0;
	if (Name[25] != 0)
		Name[26] = 0;

	if (Const != 0 || NNA > 3 || DCT > 3 || DNA > 2)
		throw ModuleLoaderError(E_BAD_IT);

	FadeOut <<= 6;
	for (uint8_t env = 0; env < Envelopes.size(); ++env)
		Envelopes[env] = makeUnique<ModuleEnvelope>(file, env);
}

uint8_t ModuleNewInstrument::Map(uint8_t Note) noexcept
{
	uint8_t sample = 0;
	for (uint16_t i = 0; i < 120; i++)
	{
		if (SampleMapping[i << 1] <= Note)
			sample = SampleMapping[(i << 1) + 1];
	}
	return sample;
}

uint16_t ModuleNewInstrument::GetFadeOut() const noexcept { return FadeOut; }
bool ModuleNewInstrument::GetEnvEnabled(uint8_t env) const
	{ return Envelopes[env]->GetEnabled(); }
bool ModuleNewInstrument::GetEnvLooped(uint8_t env) const
	{ return Envelopes[env]->GetLooped(); }
ModuleEnvelope *ModuleNewInstrument::GetEnvelope(uint8_t env) const noexcept
	{ return env < Envelopes.size() ? Envelopes[env].get() : nullptr; }
bool ModuleNewInstrument::IsPanned() const noexcept { return !(Panning & 128); }
bool ModuleNewInstrument::HasVolume() const noexcept { return true; }
uint8_t ModuleNewInstrument::GetPanning() const noexcept { return (Panning & 0x7F) << 1; }
uint8_t ModuleNewInstrument::GetVolume() const noexcept { return Volume & 0x7F; }
uint8_t ModuleNewInstrument::GetNNA() const noexcept { return NNA; }
uint8_t ModuleNewInstrument::GetDCT() const noexcept { return DCT; }
uint8_t ModuleNewInstrument::GetDNA() const noexcept { return DNA; }

ModuleEnvelope::ModuleEnvelope(const modIT_t &file, uint8_t env) : Type(env)
{
	uint8_t DontCare;
	const fd_t &fd = file.fd();

	if (!fd.read(&Flags, 1) ||
		!fd.read(&nNodes, 1) ||
		!fd.read(&LoopBegin, 1) ||
		!fd.read(&LoopEnd, 1) ||
		!fd.read(&SusLoopBegin, 1) ||
		!fd.read(&SusLoopEnd, 1) ||
		!fd.read(Nodes) ||
		!fd.read(&DontCare, 1))
		throw ModuleLoaderError(E_BAD_IT);

	if (LoopBegin > nNodes || LoopEnd > nNodes)
		throw ModuleLoaderError(E_BAD_IT);

	if (env != ENVELOPE_VOLUME)
	{
		// This requires signed/unsigned conversion
		for (uint8_t i = 0; i < nNodes; i++)
			Nodes[i].Value ^= 0x80;
	}
}

ModuleEnvelope::ModuleEnvelope(const modIT_t &, const uint8_t flags, const uint8_t loopBegin,
	const uint8_t loopEnd, const uint8_t susLoopBegin, const uint8_t susLoopEnd) noexcept :
	Type(0), Flags(flags), LoopBegin(loopBegin), LoopEnd(loopEnd), SusLoopBegin(susLoopBegin), SusLoopEnd(susLoopEnd) { }

uint8_t ModuleEnvelope::Apply(const uint16_t currentTick) noexcept
{
	uint8_t pt, n1 = 0, ret = 0;
	for (pt = 0; pt < (nNodes - 1); ++pt)
	{
		if (currentTick <= Nodes[pt].Tick)
			break;
	}
	if (currentTick >= Nodes[pt].Tick)
		return Nodes[pt].Value;
	if (pt)
	{
		n1 = Nodes[pt - 1].Tick;
		ret = Nodes[pt - 1].Value;
	}
	uint8_t n2 = Nodes[pt].Tick;
	if (n2 > n1 && currentTick > n1)
	{
		int16_t val = currentTick - n1;
		val *= int16_t(Nodes[pt].Value) - ret;
		n2 -= n1;
		return ret + (val / n2);
	}
	return ret;
}

bool ModuleEnvelope::GetEnabled() const noexcept { return (Flags & 0x01); }
bool ModuleEnvelope::GetLooped() const noexcept { return (Flags & 0x02); }
bool ModuleEnvelope::GetSustained() const noexcept { return (Flags & 0x04); }
bool ModuleEnvelope::HasNodes() const noexcept { return nNodes; }
bool ModuleEnvelope::IsAtEnd(const uint16_t currentTick) const noexcept
	{ return currentTick > GetLastTick(); }
bool ModuleEnvelope::IsZeroLoop() const noexcept { return LoopEnd == LoopBegin; }
uint16_t ModuleEnvelope::GetLoopEnd() const noexcept { return Nodes[LoopEnd].Tick; }
uint16_t ModuleEnvelope::GetLoopBegin() const noexcept { return Nodes[LoopBegin].Tick; }
uint16_t ModuleEnvelope::GetSustainEnd() const noexcept { return Nodes[SusLoopBegin].Tick; }
uint16_t ModuleEnvelope::GetSustainBegin() const noexcept { return Nodes[SusLoopBegin].Tick; }
uint16_t ModuleEnvelope::GetLastTick() const noexcept { return Nodes[nNodes - 1].Tick; }
