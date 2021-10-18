// SPDX-License-Identifier: BSD-3-Clause
#include "genericModule.h"

ModuleInstrument *ModuleInstrument::LoadInstrument(const modIT_t &file, const uint32_t i, const uint16_t FormatVersion)
{
	if (FormatVersion < 0x0200)
		return new ModuleOldInstrument(file, i);
	else
		return new ModuleNewInstrument(file, i);
}

std::pair<uint8_t, uint8_t> ModuleInstrument::mapNote(const uint8_t note) noexcept
{
	if (note >= 254) // 0xFE and 0xFF have special meaning.
		return {note, 0};
	else if (!note || note > 120)
		return {250, 0}; // If we get an out of range value, return an invalid note.
	const uint8_t entry = (note - 1) << 1U;
	uint8_t mappedNote = SampleMapping[entry];
	const uint8_t mappedSample = SampleMapping[entry + 1];
	if (mappedNote >= 128 && mappedNote < 254)
		mappedNote = 250;
	else if (mappedNote < 128)
		++mappedNote;
	return {mappedNote, mappedSample};
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

uint16_t ModuleOldInstrument::GetFadeOut() const noexcept { return FadeOut; }
bool ModuleOldInstrument::GetEnvEnabled(const envelopeType_t env) const noexcept
	{ return static_cast<uint8_t>(env) ? false : (Flags & 0x01U); }
bool ModuleOldInstrument::GetEnvLooped(const envelopeType_t env) const noexcept
	{ return static_cast<uint8_t>(env) ? false : (Flags & 0x02U); }
bool ModuleOldInstrument::GetEnvCarried(const envelopeType_t env) const noexcept
	{ return static_cast<uint8_t>(env) ? false : Envelope->GetCarried(); }
bool ModuleOldInstrument::IsPanned() const noexcept { return false; }
bool ModuleOldInstrument::HasVolume() const noexcept { return false; }
uint8_t ModuleOldInstrument::GetPanning() const noexcept { return 0; }
uint8_t ModuleOldInstrument::GetVolume() const noexcept { return 0; }
uint8_t ModuleOldInstrument::GetNNA() const noexcept { return NNA; }
uint8_t ModuleOldInstrument::GetDCT() const noexcept { return DCT_OFF; }
uint8_t ModuleOldInstrument::GetDNA() const noexcept { return DNA_NOTECUT; }

ModuleEnvelope &ModuleOldInstrument::GetEnvelope(const envelopeType_t env) const
{
	if (static_cast<uint8_t>(env))
		throw std::range_error{"instrument envelope index out of range"};
	return *Envelope;
}

ModuleNewInstrument::ModuleNewInstrument(const modIT_t &file, const uint32_t i) : ModuleInstrument(i)
{
	uint8_t Const;
	char DontCare[6];
	std::array<char, 4> magic;
	const fd_t &fd = file.fd();

	if (!fd.read(magic) || memcmp(magic.data(), "IMPI", 4))
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

	if (FileName[11])
		FileName[12] = 0;
	if (Name[25])
		Name[26] = 0;

	if (Const || NNA > 3 || DCT > 3 || DNA > 2 || Volume > 128)
		throw ModuleLoaderError(E_BAD_IT);

	FadeOut <<= 6U;
	Volume >>= 1U; // XXX: This seems like a bug..
	for (uint8_t env{0}; env < static_cast<uint8_t>(envelopeType_t::count); ++env)
		Envelopes[env] = makeUnique<ModuleEnvelope>(file, static_cast<envelopeType_t>(env));
}

uint16_t ModuleNewInstrument::GetFadeOut() const noexcept { return FadeOut; }
bool ModuleNewInstrument::IsPanned() const noexcept { return !(Panning & 128U); }
bool ModuleNewInstrument::HasVolume() const noexcept { return true; }
uint8_t ModuleNewInstrument::GetPanning() const noexcept { return (Panning & 0x7FU) << 1U; }
uint8_t ModuleNewInstrument::GetVolume() const noexcept { return Volume & 0x7FU; }
uint8_t ModuleNewInstrument::GetNNA() const noexcept { return NNA; }
uint8_t ModuleNewInstrument::GetDCT() const noexcept { return DCT; }
uint8_t ModuleNewInstrument::GetDNA() const noexcept { return DNA; }

bool ModuleNewInstrument::GetEnvEnabled(const envelopeType_t env) const noexcept
{
	if (env >= envelopeType_t::count)
		return false;
	return Envelopes[static_cast<size_t>(env)]->GetEnabled();
}

bool ModuleNewInstrument::GetEnvLooped(const envelopeType_t env) const noexcept
{
	if (env >= envelopeType_t::count)
		return false;
	return Envelopes[static_cast<size_t>(env)]->GetLooped();
}

bool ModuleNewInstrument::GetEnvCarried(const envelopeType_t env) const noexcept
{
	if (env >= envelopeType_t::count)
		return false;
	return Envelopes[static_cast<size_t>(env)]->GetCarried();
}

ModuleEnvelope &ModuleNewInstrument::GetEnvelope(const envelopeType_t env) const
{
	if (env >= envelopeType_t::count)
		throw std::range_error{"instrument envelope index out of range"};
	return *Envelopes[static_cast<size_t>(env)];
}

ModuleEnvelope::ModuleEnvelope(const modIT_t &file, const envelopeType_t env) : Type{env}
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

	if (env != envelopeType_t::volume)
	{
		// This requires signed/unsigned conversion
		for (uint8_t i = 0; i < nNodes; i++)
			Nodes[i].Value ^= 0x80U;
	}
}

ModuleEnvelope::ModuleEnvelope(const modIT_t &, const uint8_t flags, const uint8_t loopBegin,
	const uint8_t loopEnd, const uint8_t susLoopBegin, const uint8_t susLoopEnd) noexcept :
	Type{envelopeType_t::volume}, Flags{flags}, LoopBegin{loopBegin}, LoopEnd{loopEnd},
	SusLoopBegin{susLoopBegin}, SusLoopEnd{susLoopEnd} { }

uint8_t ModuleEnvelope::Apply(const uint16_t currentTick) noexcept
{
	uint8_t pt = 0;
	uint8_t ret = 0;
	uint16_t n1 = 0;
	for (; pt < (nNodes - 1); ++pt)
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
	uint16_t n2 = Nodes[pt].Tick;
	if (n2 > n1 && currentTick > n1)
	{
		int32_t val = currentTick - n1;
		val *= int16_t(Nodes[pt].Value) - ret;
		n2 -= n1;
		return ret + (val / n2);
	}
	return ret;
}
