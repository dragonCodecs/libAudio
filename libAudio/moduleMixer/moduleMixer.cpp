// SPDX-License-Identifier: BSD-3-Clause
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#include "../libAudio.h"
#include "../libAudio.hxx"
#include "../genericModule/genericModule.h"

#include "waveTables.h"
#include "moduleMixer.h"
#include "mixFunctions.h"
#include "mixFunctionTables.h"
#include "frequencyTables.h"

uint32_t Convert32to16(void *_out, int32_t *_in, uint32_t SampleCount)
{
	uint32_t i;
	int16_t *out = (int16_t *)_out;
	for (i = 0; i < SampleCount; i++)
	{
		int32_t samp = _in[i]/* + (1 << 11)*/;
		clipInt<int32_t>(samp, 0xF8000001, 0x07FFFFFF);
		out[i] = samp >> 12U;
	}
	return SampleCount << 1U;
}

int64_t moduleFile_t::fillBuffer(void *const bufferPtr, const uint32_t length)
{
	const auto buffer = static_cast<uint8_t *>(bufferPtr);
	return ctx->mod->Mix(buffer, length);
}

void ModuleFile::InitMixer(fileInfo_t &info)
{
	MixSampleRate = info.bitRate;
	MixChannels = info.channels;
	MixBitsPerSample = info.bitsPerSample;
	MusicSpeed = p_Header->InitialSpeed;
	MusicTempo = p_Header->InitialTempo;
	TickCount = MusicSpeed;
	if (ModuleType != MODULE_IT)
		globalVolume = p_Header->GlobalVolume << 1U;
	else
		globalVolume = p_Header->GlobalVolume;
	SamplesPerTick = (MixSampleRate * 640U) / (MusicTempo << 8U);
	// If we have the possibility of NNAs, allocate a full set of channels.
	if (p_Instruments != nullptr)
	{
		Channels = new channel_t[128]();
		MixerChannels = new uint32_t[128];
	}
	// Otherwise just allocate the number in the song as that's all we can process in this case.
	else
	{
		Channels = new channel_t[p_Header->nChannels]();
		MixerChannels = new uint32_t[p_Header->nChannels];
	}

	for (uint8_t i = 0; i < p_Header->nChannels; ++i)
	{
		if (i >= 64U)
			break;
		Channels[i].channelVolume = p_Header->Volumes[i];
	}

	Rows = 2;
	ResetChannelPanning();
}

channel_t::channel_t() noexcept : SampleData{nullptr}, NewSampleData{nullptr}, Note{}, RampLength{},
	NewNote{}, NewSample{}, LoopStart{}, LoopEnd{}, Length{}, RawVolume{}, volume{},
	_sampleVolumeSlide{}, _fineSampleVolumeSlide{}, channelVolume{64}, sampleVolume{},
	autoVibratoDepth{}, autoVibratoPos{}, Sample{nullptr}, Instrument{nullptr}, FineTune{},
	_panningSlide{}, RawPanning{}, panning{}, RowNote{}, RowSample{}, RowVolEffect{}, Flags{},
	Period{}, C4Speed{}, Pos{}, PosLo{}, startTick{}, increment{}, portamentoTarget{},
	portamento{}, portamentoSlide{}, Arpeggio{}, extendedCommand{}, tremor{}, tremorCount{},
	leftVol{}, rightVol{}, NewLeftVol{}, NewRightVol{}, LeftRamp{}, RightRamp{}, patternLoopCount{},
	patternLoopStart{}, Filter_Y1{}, Filter_Y2{}, Filter_Y3{}, Filter_Y4{}, Filter_A0{},
	Filter_B0{}, Filter_B1{}, Filter_HP{}, tremoloDepth{}, tremoloSpeed{}, tremoloPos{}, tremoloType{},
	vibratoDepth{}, vibratoSpeed{}, vibratoPosition{}, vibratoType{}, panbrelloDepth{}, panbrelloSpeed{},
	panbrelloPosition{}, panbrelloType{}, EnvVolumePos{}, EnvPanningPos{}, EnvPitchPos{}, FadeOutVol{},
	DCOffsL{}, DCOffsR{} { }

void ModuleFile::DeinitMixer()
{
	delete [] Channels;
	delete [] MixerChannels;
}

void ModuleFile::ResetChannelPanning()
{
	if (ModuleType == MODULE_MOD || ModuleType == MODULE_AON)
	{
		for (uint8_t i = 0; i < p_Header->nChannels; i++)
		{
			uint8_t j = i % 4U;
			if (j == 0 || j == 3U)
				Channels[i].RawPanning = 64U;
			else
				Channels[i].RawPanning = 192U;
		}
	}
	else if (ModuleType == MODULE_S3M || ModuleType == MODULE_IT)
	{
		if (p_Header->Panning == nullptr)
		{
			for (uint8_t i = 0; i < p_Header->nChannels; i++)
				Channels[i].RawPanning = 128U;
		}
		else
		{
			for (uint8_t i = 0; i < p_Header->nChannels; i++)
			{
				Channels[i].RawPanning = p_Header->Panning[i];
				Channels[i].Flags |= (p_Header->PanSurround[i] ? CHN_SURROUND : 0);
			}
		}
	}
	else if (ModuleType == MODULE_STM)
	{
		for (uint8_t i = 0; i < p_Header->nChannels; i++)
		{
			if (i % 2U)
				Channels[i].RawPanning = 64U;
			else
				Channels[i].RawPanning = 192U;
		}
	}
}

void ModuleFile::ReloadSample(channel_t &channel)
{
	ModuleSample &sample = *channel.Sample;
	//if (channel.Instrument == nullptr)
	channel.RawVolume = sample.GetVolume();
	channel.NewSample = 0;
	channel.Length = sample.GetLength();
	channel.Flags &= ~(CHN_LOOP | CHN_SUSTAINLOOP | CHN_LPINGPONG);
	if (sample.GetSustainLooped())
	{
		channel.Flags |= CHN_LOOP | CHN_SUSTAINLOOP;
		channel.LoopStart = sample.GetSustainLoopBegin();
		channel.LoopEnd = sample.GetSustainLoopEnd();
		if (sample.GetBidiLoop())
			channel.Flags |= CHN_LPINGPONG;
	}
	else if (sample.GetLooped())
	{
		channel.Flags |= CHN_LOOP;
		channel.LoopStart = (sample.GetLoopStart() < sample.GetLength() ? sample.GetLoopStart() : sample.GetLength());
		channel.LoopEnd = sample.GetLoopEnd();
	}
	else
	{
		channel.LoopStart = 0;
		channel.LoopEnd = channel.Length;
	}
	channel.NewSampleData = p_PCM[sample.id()];
	channel.FineTune = sample.GetFineTune();
	channel.C4Speed = sample.GetC4Speed();
	if (channel.LoopEnd > channel.Length)
		channel.LoopEnd = channel.Length;
	if (channel.Length > channel.LoopEnd)
		channel.Length = channel.LoopEnd;
	channel.autoVibratoPos = 0;
}

void ModuleFile::SampleChange(channel_t &channel, const uint32_t sampleIndex, const bool doPortamento)
{
	if ((p_Instruments && sampleIndex > totalInstruments()) ||
		(!p_Instruments && sampleIndex > totalSamples()))
		return;
	auto *const instr = p_Instruments ? p_Instruments[sampleIndex - 1] : nullptr;
	auto *sample = this->sample(sampleIndex);
	auto note = channel.Note;
	bool instrumentChanged = false;
	if (instr && note && note <= 128U)
	{
		uint8_t _sampleIndex{};
		std::tie(note, _sampleIndex) = instr->mapNote(note);
		if (note >= 0xFEU)
			return;
		sample = this->sample(_sampleIndex);
	}
	else if (p_Instruments)
	{
		if (note >= 0xFEU)
			return;
	}
	channel.RawVolume = sample ? sample->GetVolume() : 0;
	if (channel.Instrument != instr)
	{
		instrumentChanged = true;
		channel.Instrument = instr;
	}
	channel.NewSample = 0;
	if (sample)
	{
		if (instr)
		{
			if (instr->HasVolume())
				channel.sampleVolume = uint32_t(instr->GetVolume() * sample->GetSampleVolume()) >> 6U;
			else
				channel.sampleVolume = sample->GetSampleVolume();
			if (instr->IsPanned())
				channel.RawPanning = instr->GetPanning();
		}
		else
			channel.sampleVolume = sample->GetSampleVolume();
		if (sample->GetPanned())
			channel.RawPanning = sample->GetPanning();
	}
	if (!doPortamento || !typeIs<MODULE_IT>() || !channel.Length ||
		((channel.Flags & CHN_NOTEFADE) && !channel.FadeOutVol))
	{
		channel.Flags |= CHN_FASTVOLRAMP;
		if (typeIs<MODULE_IT>() && !instrumentChanged && instr && !(channel.Flags & (CHN_NOTEOFF | CHN_NOTEFADE)))
		{
			if (!instr->GetEnvCarried(ENVELOPE_VOLUME))
				channel.EnvVolumePos = 0;
			if (!instr->GetEnvCarried(ENVELOPE_PANNING))
				channel.EnvPanningPos = 0;
			if (!instr->GetEnvCarried(ENVELOPE_PITCH))
				channel.EnvPitchPos = 0;
		}
		else
		{
			channel.EnvVolumePos = 0;
			channel.EnvPanningPos = 0;
			channel.EnvPitchPos = 0;
		}
		channel.autoVibratoDepth = 0;
		channel.autoVibratoPos = 0;
	}
	else if (instr && !instr->GetEnvEnabled(ENVELOPE_VOLUME))
	{
		channel.EnvVolumePos = 0;
		channel.autoVibratoDepth = 0;
		channel.autoVibratoPos = 0;
	}
	if (!sample)
	{
		channel.Instrument = nullptr;
		channel.Sample = nullptr;
		channel.NewSampleData = nullptr;
		channel.sampleVolume = 0;
		return;
	}
	if (doPortamento && sample == channel.Sample)
	{
		if (typeIs<MODULE_S3M, MODULE_IT>())
			return;
		channel.Flags &= ~(CHN_NOTEOFF | CHN_NOTEFADE | CHN_LOOP | CHN_SUSTAINLOOP | CHN_LPINGPONG | CHN_FPINGPONG);
		channel.Flags |= (sample->GetSustainLooped() || sample->GetLooped() ? CHN_LOOP : 0) |
			(sample->GetSustainLooped() && sample->GetBidiLoop() ? CHN_LPINGPONG : 0) |
			(sample->GetSustainLooped() ? CHN_SUSTAINLOOP : 0);
	}
	else
	{
		channel.Flags &= ~(CHN_NOTEOFF | CHN_NOTEFADE | CHN_LOOP | CHN_SUSTAINLOOP | CHN_LPINGPONG);
		channel.Flags |= (sample->GetSustainLooped() || sample->GetLooped() ? CHN_LOOP : 0) |
			(sample->GetSustainLooped() && sample->GetBidiLoop() ? CHN_LPINGPONG : 0) |
			(sample->GetSustainLooped() ? CHN_SUSTAINLOOP : 0);
		// TODO: Handle volume and panning swing..
	}
	channel.Sample = sample;
	channel.Length = sample->GetLength();
	if (sample->GetSustainLooped())
	{
		channel.LoopStart = sample->GetSustainLoopBegin();
		channel.LoopEnd = sample->GetSustainLoopEnd();
	}
	else if (sample->GetLooped())
	{
		channel.LoopStart = sample->GetLoopStart();
		channel.LoopEnd = sample->GetLoopEnd();
	}
	else
	{
		channel.LoopStart = 0;
		channel.LoopEnd = channel.Length;
	}
	if (channel.LoopEnd > channel.Length)
		channel.LoopEnd = channel.Length;
	if (channel.Length > channel.LoopEnd)
		channel.Length = channel.LoopEnd;
	channel.C4Speed = sample->GetC4Speed();
	channel.FineTune = sample->GetFineTune();
	channel.NewSampleData = p_PCM[sample->id()];
}

uint32_t ModuleFile::GetPeriodFromNote(uint8_t Note, uint8_t fineTune, uint32_t C4Speed)
{
	if (!Note || Note > 0xF0U)
		return 0;
	Note--;
	if (typeIs<MODULE_IT, MODULE_S3M, MODULE_STM>())
	{
		if (hasLinearSlides())
			return (S3MPeriods[Note % 12U] << 5U) >> (Note / 12U);
		else
		{
			if (C4Speed == 0)
				C4Speed = 8363U;
			return muldiv_t<uint32_t>{}(8363U, (S3MPeriods[Note % 12U] << 5U), C4Speed << (Note / 12U));
		}
	}
	else
	{
		if (fineTune || Note < 36U || Note >= 108U)
			return (MODTunedPeriods[(fineTune * 12U) + (Note % 12U)] << 5U) >> (Note / 12U);
		else
			return MODPeriods[Note - 36U] << 2U;
	}
}

uint32_t ModuleFile::GetFreqFromPeriod(uint32_t Period, uint32_t C4Speed, int8_t PeriodFrac)
{
	if (!Period)
		return 0;
	if (typeIs<MODULE_MOD>())
		return 14187580UL / Period;
	else
	{
		if (hasLinearSlides())
		{
			if (!C4Speed)
				C4Speed = 8363;
			return muldiv_t<uint32_t>{}(C4Speed, 438272UL, (Period << 8U) + PeriodFrac);
		}
		else
			return muldiv_t<uint32_t>{}(8363, 438272UL, (Period << 8U) + PeriodFrac);
	}
}

void channel_t::noteChange(ModuleFile &module, uint8_t note, bool handlePorta)
{
	uint32_t period;
	ModuleSample *sample = Sample;

	if (!note)
		return;
	else if (Instrument)
	{
		uint8_t sampleIndex{};
		std::tie(note, sampleIndex) = Instrument->mapNote(note);
		if (sampleIndex && sampleIndex <= module.totalSamples())
			sample = module.sample(sampleIndex);
		if (sample)
		{
			FineTune = sample->GetFineTune();
			C4Speed = sample->GetC4Speed();
		}
	}
	if (note >= 0x80U)
	{
		if (note == 0xFFU || !module.typeIs<MODULE_IT>())
			noteOff();
		else
			Flags |= CHN_NOTEFADE;

		if (note == 0xFEU)
		{
			Flags |= CHN_NOTEFADE | CHN_FASTVOLRAMP;
			if (!module.typeIs<MODULE_IT>() || module.totalInstruments())
				RawVolume = 0;
			FadeOutVol = 0;
		}
		return;
	}
	else if (!sample) // MPT has this above the clip-n-period code.. OpenMPT has this below..
		return;
	clipInt<uint8_t>(note, 1, 132);
	Note = note;
	if (!handlePorta || module.typeIs<MODULE_S3M, MODULE_IT>())
		NewSample = 0;
	period = module.GetPeriodFromNote(note, FineTune, C4Speed);
	if (period)
	{
		if (!handlePorta || !Period)
			Period = period;
		portamentoTarget = period;
		if (!handlePorta || (!Length && !module.typeIs<MODULE_S3M>()))
		{
			Sample = sample;
			NewSampleData = module.p_PCM[sample->id()];
			Length = sample->GetLength();
			Flags &= ~(CHN_LOOP | CHN_LPINGPONG);
			if (sample->GetSustainLooped())
			{
				Flags |= CHN_LOOP | CHN_SUSTAINLOOP;
				LoopStart = sample->GetSustainLoopBegin();
				LoopEnd = sample->GetSustainLoopEnd();
				if (sample->GetBidiLoop())
					Flags |= CHN_LPINGPONG;
			}
			else if (sample->GetLooped())
			{
				Flags |= CHN_LOOP;
				LoopStart = (sample->GetLoopStart() < sample->GetLength() ? sample->GetLoopStart() : sample->GetLength());
				LoopEnd = sample->GetLoopEnd();
			}
			else
			{
				LoopStart = 0;
				LoopEnd = Length;
			}
			Pos = 0;
			PosLo = 0;
			if (vibratoType < 4)
				vibratoPosition = module.typeIs<MODULE_IT>() && module.useOldEffects() ? 0x10 : 0;
			//if ((channel->tremoloType & 0x03) != 0)
			if (tremoloType < 4)
				tremoloPos = 0;
		}
		if (Pos > Length)
			Pos = LoopStart;
	}
	else
		handlePorta = false;
	if (!handlePorta || !module.typeIs<MODULE_IT>() || ((Flags & CHN_NOTEFADE) && !FadeOutVol))
	{
		if (module.typeIs<MODULE_IT>() && (Flags & CHN_NOTEFADE) && !FadeOutVol)
		{
			EnvVolumePos = 0;
			EnvPanningPos = 0;
			EnvPitchPos = 0;
			autoVibratoDepth = 0;
			autoVibratoPos = 0;
			Flags &= ~CHN_NOTEFADE;
			FadeOutVol = 0xFFFFU;
		}
		else if (!handlePorta)
		{
			Flags &= ~CHN_NOTEFADE;
			FadeOutVol = 0xFFFFU;
		}
	}
	Flags &= ~CHN_NOTEOFF;
	if (!handlePorta)
	{
		Flags |= CHN_FASTVOLRAMP;
		tremorCount = 0;
		// if (resetEnvironment)
		leftVol = 0;
		rightVol = 0;
	}
}

uint8_t ModuleFile::FindFreeNNAChannel() const
{
	uint8_t res = 0;
	uint32_t volWeight = 64U << 16U;
	uint16_t envVolumePos = 0xFFFFU;
	for (uint8_t i = p_Header->nChannels; i < 128U; i++)
	{
		const channel_t &chn = Channels[i];
		uint32_t weight = chn.RawVolume;
		if (chn.FadeOutVol == 0)
			return i;
		if ((chn.Flags & CHN_NOTEFADE) != 0)
			weight *= chn.FadeOutVol;
		else
			weight <<= 16U;
		if ((chn.Flags & CHN_LOOP) != 0)
			weight >>= 1U;
		if (weight < volWeight || (weight == volWeight && chn.EnvVolumePos > envVolumePos))
		{
			envVolumePos = chn.EnvVolumePos;
			volWeight = weight;
			res = i;
		}
	}
	return res;
}

void ModuleFile::HandleNNA(channel_t *channel, uint32_t instrument, uint8_t note)
{
	if (note > 0x80U || note < 1)
		return;
	// If not Impulse Tracker, or we have no actual instruments, always cut on NNA
	if (!typeIs<MODULE_IT>() || !p_Instruments)
	{
		if (!channel->Length || !channel->leftVol || !channel->rightVol || !p_Instruments)
			return;
		const uint8_t newChannel = FindFreeNNAChannel();
		if (!newChannel)
			return;
		channel_t &nnaChannel = Channels[newChannel];
		nnaChannel = *channel;
		nnaChannel.Flags &= ~(CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO | CHN_PORTAMENTO);
		nnaChannel.RowEffect = CMD_NONE;
		nnaChannel.FadeOutVol = 0;
		nnaChannel.Flags |= CHN_NOTEFADE | CHN_FASTVOLRAMP;
		channel->Length = 0;
		channel->Pos = 0;
		channel->PosLo = 0;
		channel->DCOffsL = 0;
		channel->DCOffsR = 0;
		channel->leftVol = 0;
		channel->rightVol = 0;
		return;
	}
	if (instrument >= totalInstruments())
		instrument = 0;
	auto *sample = channel->Sample;
	auto *instr = channel->Instrument;
	if (instrument)
	{
		instr = p_Instruments[instrument - 1];
		if (instr)
		{
			uint8_t sampleIndex{};
			std::tie(note, sampleIndex) = instr->mapNote(note);
			if (sampleIndex && sampleIndex <= totalSamples())
				sample = this->sample(sampleIndex);
		}
		else
			sample = nullptr;
	}
	else
		return;

	for (uint8_t i = 0; i < 128; i++)
	{
		channel_t *dnaChannel = &Channels[i];
		if (dnaChannel->Instrument && (i >= p_Header->nChannels || dnaChannel == channel))
		{
			bool duplicate = false;
			switch (dnaChannel->Instrument->GetDCT())
			{
				case DCT_NOTE:
					if (note && dnaChannel->Note == note && instr == dnaChannel->Instrument)
						duplicate = true;
					break;
				case DCT_SAMPLE:
					if (sample && sample == dnaChannel->Sample)
						duplicate = true;
					break;
				case DCT_INSTRUMENT:
					if (instr == dnaChannel->Instrument)
						duplicate = true;
					break;
			}

			if (duplicate)
			{
				switch (dnaChannel->Instrument->GetDNA())
				{
					case DNA_NOTECUT:
						dnaChannel->noteOff();
						dnaChannel->RawVolume = 0;
						break;
					case DNA_NOTEOFF:
						dnaChannel->noteOff();
						break;
					case DNA_NOTEFADE:
						dnaChannel->Flags |= CHN_NOTEFADE;
						break;
				}
				if (!dnaChannel->RawVolume)
				{
					dnaChannel->FadeOutVol = 0;
					dnaChannel->Flags |= CHN_NOTEFADE | CHN_FASTVOLRAMP;
				}
			}
		}
	}

	if (channel->RawVolume && channel->Length)
	{
		const uint8_t newChannel = FindFreeNNAChannel();
		if (newChannel)
		{
			// With the new channel, duplicate it and clear certain effects
			channel_t &nnaChannel = Channels[newChannel];
			nnaChannel = *channel;
			nnaChannel.Flags &= ~(CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO | CHN_PORTAMENTO);
			nnaChannel.RowEffect = CMD_NONE;
			// Then check what the NNA is supposed to be
			switch (instr->GetNNA())
			{
				case NNA_NOTEOFF:
					nnaChannel.noteOff();
					break;
				case NNA_NOTECUT:
					nnaChannel.FadeOutVol = 0;
					[[clang::fallthrough]]; // Falls through
				case NNA_NOTEFADE:
					nnaChannel.Flags |= CHN_NOTEFADE;
					break;
			}
			// NNA_CONTINUE is done implicitly in just duplicating the channel here, so only cull already silent samples.
			if (!nnaChannel.RawVolume)
			{
				nnaChannel.FadeOutVol = 0;
				nnaChannel.Flags |= CHN_NOTEFADE | CHN_FASTVOLRAMP;
			}
			// And clean up on the original channel so it can be used in the main effects processing.
			channel->Length = 0;
			channel->Pos = 0;
			channel->PosLo = 0;
			channel->DCOffsL = 0;
			channel->DCOffsR = 0;
		}
	}
}

void ModuleFile::ProcessMODExtended(channel_t *channel)
{
	uint8_t param = channel->RowParam;
	uint8_t cmd = (param & 0xF0U) >> 4U;
	param &= 0x0FU;

	switch (cmd)
	{
		case CMD_MODEX_FILTER:
			// Could implement a 7Khz lowpass filter
			// For this, or just do what MPT does - ignore it.
			// XXX: Currently we ignore this.
			break;
		case CMD_MODEX_FINEPORTAUP:
			if (TickCount == channel->startTick)
			{
				if (channel->Period != 0 && param != 0)
				{
					channel->Period -= param << 2U;
					if (channel->Period < 56U)
						channel->Period = 56U;
				}
			}
			break;
		case CMD_MODEX_FINEPORTADOWN:
			if (TickCount == channel->startTick)
			{
				if (channel->Period != 0 && param != 0)
				{
					channel->Period += param << 2U;
					if (channel->Period > 7040U)
						channel->Period = 7040U;
				}
			}
			break;
		case CMD_MODEX_GLISSANDO:
			if (param == 0)
				channel->Flags &= ~CHN_GLISSANDO;
			else if (param == 1)
				channel->Flags |= CHN_GLISSANDO;
			break;
		case CMD_MODEX_VIBRATOWAVE:
			channel->vibratoType = param & 0x07U;
			break;
		case CMD_MODEX_FINETUNE:
			if (TickCount != channel->startTick)
				break;
			channel->C4Speed = S3MSpeedTable[param];
			channel->FineTune = param;
			if (channel->Period != 0)
				channel->Period = GetPeriodFromNote(channel->Note, channel->FineTune, channel->C4Speed);
			break;
		case CMD_MODEX_TREMOLOWAVE:
			channel->tremoloType = param & 0x07U;
			break;
		case CMD_MODEX_RETRIGER:
			if (param != 0 && (TickCount % param) == 0)
				channel->noteChange(*this, channel->NewNote, 0);
			break;
		case CMD_MODEX_FINEVOLUP:
			if (param != 0 && TickCount == channel->startTick)
			{
				param <<= 1U; // << 1?
				if (128 - param > channel->RawVolume)
					channel->RawVolume += param;
				else
					channel->RawVolume = 128;
			}
			break;
		case CMD_MODEX_FINEVOLDOWN:
			if (param != 0 && TickCount == channel->startTick)
			{
				param <<= 1U; // << 1?
				if (channel->RawVolume > param)
					channel->RawVolume -= param;
				else
					channel->RawVolume = 0;
			}
			break;
		case CMD_MODEX_CUT:
			if (TickCount == param)
			{
				channel->RawVolume = 0;
				channel->Flags |= CHN_FASTVOLRAMP;
			}
			break;
		case CMD_MODEX_INVERTLOOP:
			if (ModuleType == MODULE_MOD)
				channel->increment.iValue = -channel->increment.iValue;
			break;
	}
}

void channel_t::ChannelEffect(uint8_t param)
{
	switch (param)
	{
		case 0x00:
			Flags &= ~CHN_SURROUND;
			break;
		case 0x01:
			Flags |= CHN_SURROUND;
			RawPanning = 128U;
			break;
		// There are also some (Open)MPT extended modes we might want to suport here hence this structure.
	}
}

void ModuleFile::ProcessS3MExtended(channel_t *channel)
{
	uint8_t param = channel->RowParam;
	uint8_t cmd = (param & 0xF0U) >> 4U;
	param &= 0x0FU;

	switch (cmd)
	{
		case CMD_S3MEX_FILTER:
			break;
		case CMD_S3MEX_GLISSANDO:
			if (param == 0)
				channel->Flags &= ~CHN_GLISSANDO;
			else
				channel->Flags |= CHN_GLISSANDO;
			break;
		case CMD_S3MEX_FINETUNE:
			if (TickCount != channel->startTick)
				break;
			channel->C4Speed = S3MSpeedTable[param];
			channel->FineTune = param;
			if (channel->Period != 0)
				channel->Period = GetPeriodFromNote(channel->Note, channel->FineTune, channel->C4Speed);
			break;
		case CMD_S3MEX_VIBRATOWAVE:
			if (ModuleType == MODULE_S3M)
				channel->vibratoType = param & 0x03U;
			else
				channel->vibratoType = param & 0x07U;
			break;
		case CMD_S3MEX_TREMOLOWAVE:
			if (ModuleType == MODULE_S3M)
				channel->tremoloType = param & 0x03U;
			else
				channel->tremoloType = param & 0x07U;
			break;
		case CMD_S3MEX_PANWAVE:
			channel->panbrelloType = param & 0x07U;
			break;
		case CMD_S3MEX_FRAMEDELAY:
			FrameDelay = param;
			break;
		case CMD_S3MEX_ENVELOPE:
			if (TickCount == channel->startTick)
			{
			}
			break;
		case CMD_S3MEX_PANNING:
			if (TickCount == channel->startTick)
			{
				channel->RawPanning = param | (param << 4U);
				channel->Flags |= CHN_FASTVOLRAMP;
			}
			break;
		case CMD_S3MEX_CHNEFFECT:
			if (TickCount == channel->startTick)
				channel->ChannelEffect(param & 0x0FU);
			break;
		case CMD_S3MEX_OFFSET:
			if (TickCount == channel->startTick && channel->RowNote != 0 && channel->RowNote < 0x80U)
			{
				int pos = param << 16U;
				if (pos < (int)channel->Length)
					channel->Pos = pos;
			}
			break;
		case CMD_S3MEX_NOTECUT:
			channel->noteCut(*this, param);
			break;
		case CMD_S3MEX_DELAYPAT:
			PatternDelay = param;
			break;
	}
}

inline void ModuleFile::applyGlobalVolumeSlide(uint8_t param)
{
	if (!param)
		param = globalVolumeSlide;
	else
		globalVolumeSlide = param;

	uint16_t volume = globalVolume;
	const uint8_t slideLo = param & 0x0FU;
	const uint8_t slideHi = param & 0xF0U;
	if (slideHi == 0xF0U && slideLo)
	{
		if (!ticks())
			volume -= slideLo/* << 1U*/;
	}
	else if (slideLo == 0x0FU && slideHi)
	{
		if (!ticks())
			volume += slideHi >> /*3U*/4U;
	}
	else if (ticks())
	{
		if (slideLo)
			volume -= slideLo/* << 1U*/;
		else
			volume += slideHi >> /*3U*/4U;
	}

	if (volume != globalVolume)
	{
		clipInt<uint16_t>(volume, 0, 128);
		globalVolume = volume;
	}
}

bool ModuleFile::ProcessEffects()
{
	int16_t positionJump = -1;
	int16_t breakRow = -1;
	int32_t patternLoopRow = -1;
	for (uint16_t i = 0; i < p_Header->nChannels; ++i)
	{
		channel_t *channel = &Channels[i];
		uint8_t sample = channel->RowSample;
		const uint8_t cmd = channel->RowEffect;
		uint8_t param = channel->RowParam;
		bool doPortamento = cmd == CMD_TONEPORTAMENTO || cmd == CMD_TONEPORTAVOL ||
			channel->RowVolEffect == VOLCMD_PORTAMENTO;

		channel->Flags &= ~CHN_FASTVOLRAMP;
		if (cmd == CMD_MOD_EXTENDED || cmd == CMD_S3M_EXTENDED)
		{
			static_assert(CMD_MODEX_DELAYSAMP == CMD_S3MEX_DELAYSAMP, "Note/sample delay constants incorrectly defined");
			static_assert(CMD_MODEX_DELAYPAT == CMD_S3MEX_DELAYPAT, "Pattern delay constants incorrectly defined");
			if (!param && typeIs<MODULE_S3M, MODULE_IT>())
				param = channel->extendedCommand;
			else
				channel->extendedCommand = param;
			uint8_t excmd = (param & 0xF0U) >> 4U;
			if (!TickCount) // Only process extended commands on the first tick of a row.
			{
				if (excmd == CMD_MODEX_DELAYSAMP)
					channel->startTick = param & 0x0FU;
				else if ((cmd == CMD_MOD_EXTENDED && excmd == CMD_MODEX_LOOP) ||
					(cmd == CMD_S3M_EXTENDED && excmd == CMD_S3MEX_LOOP))
				{
					const auto loop = channel->patternLoop(param & 0x0FU, Row);
					if (loop >= 0)
						patternLoopRow = loop;
				}
				else if (excmd == CMD_MODEX_DELAYPAT)
					PatternDelay = param & 0x0FU;
			}
		}

		if (TickCount == channel->startTick)
		{
			uint8_t note = channel->RowNote;
			if (sample != 0)
				channel->NewSample = sample;
			if (ModuleType == MODULE_MOD && note == 0xFFU)
			{
				channel->Flags |= CHN_FASTVOLRAMP;
				channel->RawVolume = 0;
				note = sample = 0;
			}
			if (!note && sample)
			{
				if (p_Instruments)
				{
					if (channel->Instrument)
						channel->RawVolume = channel->Instrument->GetVolume();
				}
				else
					channel->RawVolume = p_Samples[sample - 1]->GetVolume();
			}
			if (note >= 0xFEU)
				sample = 0;
			if (note && note <= 128U)
			{
				channel->NewNote = note;
				if (!doPortamento)
					HandleNNA(channel, sample, note);
			}
			if (sample)
			{
				auto *const currentSample = channel->Sample;
				SampleChange(*channel, sample, doPortamento);
				channel->NewSample = 0;
				// Special case - if IT or S3M song tries to change note and do a portamento, ignore the portamento.
				if (typeIs<MODULE_S3M, MODULE_IT>() && currentSample != channel->Sample && note && note < 128)
					doPortamento = false;
			}
			if (note)
			{
				if (!sample && channel->NewSample && note < 0x80U)
				{
					SampleChange(*channel, channel->NewSample, doPortamento);
					channel->NewSample = 0;
				}
				channel->noteChange(*this, note, doPortamento);
			}
			if (channel->RowVolEffect == VOLCMD_VOLUME)
			{
				channel->RawVolume = channel->RowVolParam << 1U;
				channel->Flags |= CHN_FASTVOLRAMP;
			}
			else if (channel->RowVolEffect == VOLCMD_PANNING)
			{
				channel->RawPanning = channel->RowVolParam << 2U;
				channel->Flags |= CHN_FASTVOLRAMP;
			}
		}
		processEffects(*channel, param, breakRow, positionJump);
	}
	return handleNavigationEffects(patternLoopRow, breakRow, positionJump);
}

void ModuleFile::processEffects(channel_t &channel, uint8_t param, int16_t &breakRow, int16_t &positionJump)
{
	switch (channel.RowEffect)
	{
		case CMD_NONE:
			break;
		case CMD_ARPEGGIO:
			if (TickCount == channel.startTick && channel.Period && channel.Note != 0xFF)
			{
				if (param == 0 && ModuleType != MODULE_S3M)
					break;
				channel.Flags |= CHN_ARPEGGIO;
				if (param != 0)
					channel.Arpeggio = param;
			}
			break;
		case CMD_PORTAMENTOUP:
			channel.portamentoUp(*this, param);
			break;
		case CMD_PORTAMENTODOWN:
			channel.portamentoDown(*this, param);
			break;
		case CMD_TONEPORTAMENTO:
			channel.tonePortamento(*this, param);
			break;
		case CMD_VIBRATO:
			channel.vibrato(param, 4);
			break;
		case CMD_TONEPORTAVOL:
			if (param != 0) // In theory, this if does nothing as VolumeSlide() is protected too.
				channel.sampleVolumeSlide(*this, param);
			channel.tonePortamento(*this, 0);
			break;
		case CMD_TONEPORTAVOLUP:
			// param contains volume in high nibble
			// Volume low nibble as 0 is "up"
			channel.sampleVolumeSlide(*this, param & 0xF0U);
			channel.tonePortamento(*this, param & 0x0FU);
			break;
		case CMD_TONEPORTAVOLDOWN:
			// Volume high nibble as 0 is "down"
			channel.sampleVolumeSlide(*this, param >> 4U);
			channel.tonePortamento(*this, param & 0x0FU);
			break;
		case CMD_VIBRATOVOL:
			if (param != 0)
				channel.sampleVolumeSlide(*this, param);
			channel.Flags |= CHN_VIBRATO;
			break;
		case CMD_TREMOLO:
			if (param & 0x0FU)
				channel.tremoloDepth = param & 0x0FU;
			if (param & 0xF0U)
				channel.tremoloSpeed = param >> 4U;
			channel.Flags |= CHN_TREMOLO;
			break;
		case CMD_OFFSET:
			if (TickCount == channel.startTick)
			{
				channel.Pos = param << 8U;
				channel.PosLo = 0;
				if (channel.Pos > channel.Length)
					channel.Pos = channel.Length;
			}
			break;
		case CMD_VOLUMESLIDE:
			if (param != 0 || ModuleType != MODULE_MOD)
				channel.sampleVolumeSlide(*this, param);
			break;
		case CMD_CHANNELVOLSLIDE:
			channel.channelVolumeSlide(*this, param);
			break;
		case CMD_GLOBALVOLSLIDE:
			applyGlobalVolumeSlide(param);
			break;
		case CMD_POSITIONJUMP:
			positionJump = param;
			if (positionJump > p_Header->nOrders)
				positionJump = 0;
			break;
		case CMD_PATTERNBREAK:
			breakRow = ((param >> 4U) * 10) + (param & 0x0FU);
			if (breakRow > Rows - 1)
				breakRow = Rows - 1;
			break;
		case CMD_SPEED:
			MusicSpeed = param;
			break;
		case CMD_MOD_EXTENDED:
			ProcessMODExtended(&channel);
			break;
		case CMD_S3M_EXTENDED:
			ProcessS3MExtended(&channel);
			break;
		case CMD_RETRIGER:
			if (param != 0 && (TickCount % param) == 0)
				channel.noteChange(*this, channel.NewNote, false);
			break;
		case CMD_TEMPO:
			MusicTempo = param;
			break;
		case CMD_VOLUME:
			if (TickCount == 0)//channel->startTick)
			{
				uint8_t NewVolume = param;
				if (NewVolume > 64)
					NewVolume = 64;
				channel.RawVolume = NewVolume << 1U;
				channel.Flags |= CHN_FASTVOLRAMP;
			}
			break;
		case CMD_CHANNELVOLUME:
			if (TickCount == 0 && param < 65)
			{
				channel.channelVolume = param;
				channel.Flags |= CHN_FASTVOLRAMP;
			}
			break;
		case CMD_GLOBALVOLUME:
			if (TickCount == channel.startTick)
			{
				if (ModuleType == MODULE_IT && param > 128)
					break;
				if (param > 128)
					param = 128;
				globalVolume = param;
			}
			break;
		case CMD_PANNING:
			if (TickCount > channel.startTick)
				break;
			if (ModuleType == MODULE_MOD || ModuleType == MODULE_IT)
				channel.RawPanning = param;
			else if (param <= 128)
				channel.RawPanning = param << 1U;
			else if (param == 164)
			{
				channel.RawPanning = 128;
				channel.Flags |= CHN_SURROUND;
			}
			channel.Flags |= CHN_FASTVOLRAMP;
			break;
		case CMD_PANNINGSLIDE:
			channel.panningSlide(*this, param);
			break;
		case CMD_FINEVIBRATO:
			channel.vibrato(param, 1);
			break;
		case CMD_TREMOR:
			if (TickCount != 0)
				break;
			if (param != 0)
				channel.tremor = param;
			channel.Flags |= CHN_TREMOR;
			break;
		case CMD_PANBRELLO:
			channel.panbrello(param);
			break;
		default:
			break;
	}
}

bool ModuleFile::handleNavigationEffects(const int32_t patternLoopRow, const int16_t breakRow,
	const int16_t positionJump) noexcept
{
	if (!TickCount)
	{
		if (patternLoopRow >= 0)
		{
			NextPattern = NewPattern;
			NextRow = uint16_t(patternLoopRow);
			if (PatternDelay)
				++NextRow;
		}
		else if (breakRow >= 0 || positionJump >= 0)
		{
			const uint16_t _positionJump = positionJump < 0 ? NewPattern + 1 : uint32_t(positionJump);
			const uint16_t _breakRow = breakRow < 0 ? 0 : uint32_t(breakRow);
			if (_positionJump < NewPattern)// || (_positionJump == NewPattern && _breakRow <= Row))
			{
				NextPattern = p_Header->nOrders;
				return false;
			}
			else if ((_positionJump != NewPattern || _breakRow != Row))
			{
				if (_positionJump != NewPattern)
				{
					for (uint8_t i = 0; i < p_Header->nChannels; ++i)
					{
						channel_t &channel = Channels[i];
						channel.patternLoopCount = 0;
						channel.patternLoopStart = 0;
					}
				}
				NextPattern = _positionJump;
				NextRow = _breakRow;
			}
		}
	}
	return true;
}

void channel_t::SetData(ModuleCommand *Command, ModuleHeader *p_Header)
{
	uint8_t excmd;
	RowNote = Command->Note;
	RowSample = Command->Sample;
	if ((p_Header->nInstruments && RowSample > p_Header->nInstruments) || (!p_Header->nInstruments && RowSample > p_Header->nSamples))
		RowSample = 0;
	RowVolEffect = Command->VolEffect;
	RowVolParam = Command->VolParam;
	RowEffect = Command->Effect;
	RowParam = Command->Param;
	excmd = (RowParam & 0xF0U) >> 4U;
	if ((RowEffect == CMD_MOD_EXTENDED && excmd == CMD_MODEX_DELAYSAMP) ||
		(RowEffect == CMD_S3M_EXTENDED && excmd == CMD_S3MEX_DELAYSAMP))
		startTick = RowParam & 0x0FU;
	else
		startTick = 0;
	Flags &= ~(CHN_TREMOLO | CHN_ARPEGGIO | CHN_VIBRATO | CHN_PORTAMENTO | CHN_GLISSANDO | CHN_TREMOR | CHN_PANBRELLO);
}

bool ModuleFile::Tick()
{
	TickCount++;
	if (TickCount >= (MusicSpeed * (PatternDelay + 1)) + FrameDelay)
	{
		TickCount = 0;
		PatternDelay = 0;
		FrameDelay = 0;
		Row = NextRow;
		do
		{
			if (NextPattern >= p_Header->nOrders)
				return false;
			if (NewPattern != NextPattern)
				NewPattern = NextPattern;
			Pattern = p_Header->Orders[NewPattern];
			if (Pattern >= p_Header->nPatterns)
				NextPattern++;
		}
		while (Pattern >= p_Header->nPatterns);
		NextPattern = NewPattern;
		if (Row >= Rows)
			Row = 0;
		NextRow = Row + 1;
		if (NextRow >= Rows)
		{
			NextPattern = NewPattern + 1;
			NextRow = 0;
		}
		if (!p_Patterns[Pattern])
			return false;
		const ModulePattern &pattern = *p_Patterns[Pattern];
		const auto &commands = pattern.commands();
		Rows = pattern.rows();
		for (uint32_t i = 0; i < p_Header->nChannels; ++i)
			Channels[i].SetData(&commands[i][Row], p_Header);
	}
	if (MusicSpeed == 0)
		MusicSpeed = 1;
	return ProcessEffects();
}

bool ModuleFile::AdvanceTick()
{
	if (!Tick() || !MusicTempo)
		return false;
	SamplesToMix = (MixSampleRate * 640U) / (MusicTempo << 8U);
	SamplesPerTick = SamplesToMix;
	nMixerChannels = 0;
	const uint8_t nChannels = p_Instruments ? 128 : p_Header->nChannels;
	for (uint8_t i = 0; i < nChannels; i++)
	{
		auto &channel = Channels[i];
		bool incNegative = channel.increment.iValue < 0;

		channel.increment.iValue = 0U;
		channel.volume = 0;
		channel.panning = channel.RawPanning;
		channel.RampLength = 0;

		if (channel.Period != 0 && channel.Length != 0)
		{
			uint16_t vol = channel.RawVolume;
			if (channel.Flags & CHN_TREMOLO)
				vol += channel.applyTremolo(*this, vol);
			if (channel.Flags & CHN_TREMOR)
				vol -= channel.applyTremor(*this, vol);
			/*clipInt<uint16_t>(vol, 0, 128);
			//vol <<= 7;*/

			if (channel.Instrument)
			{
				const auto *const instr{channel.Instrument};
				[&](ModuleEnvelope &env) noexcept
				{
					if (env.GetEnabled() && env.HasNodes())
						vol = channel.applyVolumeEnvelope(*this, vol, env);
				}(*instr->GetEnvelope(ENVELOPE_VOLUME));
				[&](ModuleEnvelope &env) noexcept
				{
					if (env.GetEnabled() && env.HasNodes())
						channel.applyPanningEnvelope(env);
				}(*instr->GetEnvelope(ENVELOPE_PANNING));
				if ((channel.Flags & CHN_NOTEFADE) != 0U)
					vol = channel.applyNoteFade(vol);
			}
			else if (channel.Flags & CHN_NOTEFADE)
			{
				channel.FadeOutVol = 0;
				vol = 0;
			}

			vol = muldiv_t<uint32_t>{}(vol * globalVolume, channel.channelVolume * channel.sampleVolume, 1U << 19U);
			clipInt<uint16_t>(vol, 0, 128);
			channel.volume = vol;
			clipInt(channel.Period, MinPeriod, MaxPeriod);
			uint32_t period = channel.Period;
			if ((channel.Flags & (CHN_GLISSANDO | CHN_PORTAMENTO)) == (CHN_GLISSANDO | CHN_PORTAMENTO))
				period = GetPeriodFromNote(/*GetNoteFromPeriod(period)*/channel.Note, channel.FineTune, channel.C4Speed);
			if ((channel.Flags & CHN_ARPEGGIO) != 0)
			{
				uint8_t n = TickCount % 3;
				if (n == 1)
					period = GetPeriodFromNote(channel.Note + (channel.Arpeggio >> 4U), channel.FineTune, channel.C4Speed);
				else if (n == 2)
					period = GetPeriodFromNote(channel.Note + (channel.Arpeggio & 0x0FU), channel.FineTune, channel.C4Speed);
			}
			if ((p_Header->Flags & FILE_FLAGS_AMIGA_LIMITS) != 0)
				clipInt<uint32_t>(period, 452, 3424);
			if (channel.Instrument != nullptr)
			{
				ModuleEnvelope *env = channel.Instrument->GetEnvelope(ENVELOPE_PITCH);
				if (env->GetEnabled() && env->HasNodes())
					period = channel.applyPitchEnvelope(period, *env);
			}
			period += channel.applyVibrato(*this, period);
			channel.applyPanbrello();
			int8_t fractionalPeriod{0};
			period += channel.applyAutoVibrato(*this, period, fractionalPeriod);
			if (period <= MinPeriod || period & 0x80000000)
			{
				if (ModuleType == MODULE_S3M)
					channel.Length = 0;
				period = MinPeriod;
			}
			else if (period > MaxPeriod)
			{
				if (ModuleType == MODULE_IT || period >= 0x100000)
				{
					channel.FadeOutVol = 0;
					channel.volume = 0;
					channel.Flags |= CHN_NOTEFADE;
				}
				period = MaxPeriod;
				fractionalPeriod = 0;
			}
			// Calculate the increment from the frequency from the period
			const uint32_t freq = GetFreqFromPeriod(period, channel.C4Speed, fractionalPeriod);
			// Silence impulse tracker notes that fall off the bottom of the reproduction spectrum
			if (ModuleType == MODULE_IT && freq < 256)
			{
				channel.FadeOutVol = 0;
				channel.volume = 0;
				channel.Flags |= CHN_NOTEFADE;
			}
			int32_t inc = muldiv_t<uint32_t>{}(freq, 0x10000U, MixSampleRate) + 1;
			if (incNegative && (channel.Flags & CHN_LPINGPONG) != 0 && channel.Pos != 0)
				inc = -inc;
			channel.increment.iValue = inc & ~3U;
		}
		if (channel.volume != 0 || channel.leftVol != 0 || channel.rightVol != 0)
			channel.Flags |= CHN_VOLUMERAMP;
		else
			channel.Flags &= ~CHN_VOLUMERAMP;
		channel.NewLeftVol = channel.NewRightVol = 0;
		if ((channel.increment.Value.Hi + 1) >= (int32_t)channel.LoopEnd)
			channel.Flags &= ~CHN_LOOP;
		channel.SampleData = ((channel.NewSampleData && channel.Length && channel.increment.iValue) ? channel.NewSampleData : nullptr);
		if (channel.SampleData != nullptr)
		{
			if (MixChannels == 2 && (channel.Flags & CHN_SURROUND) == 0)
			{
				channel.NewLeftVol = uint16_t(channel.volume * channel.panning) >> 8U;
				channel.NewRightVol = (channel.volume * (256U - channel.panning)) >> 8U;
			}
			else
				channel.NewLeftVol = channel.NewRightVol = channel.volume;

			channel.RightRamp = channel.LeftRamp = 0U;
			// TODO: Process ping-pong flag (pos = -pos)
			// Do we need to ramp the volume up or down?
			if ((channel.Flags & CHN_VOLUMERAMP) != 0 && (channel.leftVol != channel.NewLeftVol || channel.rightVol != channel.NewRightVol))
			{
				uint32_t RampLength = 1;
				// Calculate Volume deltas
				int32_t LeftDelta = channel.NewLeftVol - channel.leftVol;
				int32_t RightDelta = channel.NewRightVol - channel.rightVol;
				// Check if we need to calculate the RampLength, and do so if need be
				if ((channel.leftVol | channel.rightVol) != 0 && (channel.NewLeftVol | channel.NewRightVol) != 0 && (channel.Flags & CHN_FASTVOLRAMP) != 0)
				{
					RampLength = SamplesToMix;
					// Clipping:
					clipInt<uint32_t>(RampLength, 2U, 256U);
				}
				// Calculate value to add to the volume to get it closer to the new volume during ramping
				channel.LeftRamp = LeftDelta / RampLength;
				channel.RightRamp = RightDelta / RampLength;
				// Normalise the current volume so that the ramping won't under or over shoot
				channel.leftVol = channel.NewLeftVol - (channel.LeftRamp * RampLength);
				channel.rightVol = channel.NewRightVol - (channel.RightRamp * RampLength);
				// If the ramp values aren't 0 (ramping already done?)
				if ((channel.LeftRamp | channel.RightRamp) != 0U)
					channel.RampLength = RampLength;
				else
				{
					// Otherwise the ramping is done, so don't need to make the mixer functions do it for us
					channel.Flags &= ~CHN_VOLUMERAMP;
					channel.leftVol = channel.NewLeftVol;
					channel.rightVol = channel.NewRightVol;
				}
			}
			// No? ok, scratch the ramping.
			else
			{
				channel.Flags &= ~CHN_VOLUMERAMP;
				channel.leftVol = channel.NewLeftVol;
				channel.rightVol = channel.NewRightVol;
			}
			// DEBUG: Uncomment to see the channel's main state information
			/*printf("%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %d, %u, %u, %u\n",
				channel.Flags, channel.LoopStart, channel.LoopEnd, channel.Length, channel.RawVolume, channel.RowNote, channel.RowSample,
				channel.RowEffect, channel.RowParam, channel.Period, channel.portamentoTarget, channel.FineTune, channel.increment.Value.Hi,
				channel.increment.Value.Lo, channel.Pos, channel.PosLo);*/
			MixerChannels[nMixerChannels++] = i;
		}
		else
			channel.leftVol = channel.rightVol = channel.Length = 0;
	}

	return true;
}

uint32_t channel_t::GetSampleCount(uint32_t samples)
{
	uint32_t deltaHi, deltaLo, maxSamples, sampleCount;
	uint32_t loopStart = ((Flags & CHN_LOOP) != 0 ? LoopStart : 0);
	int16dot16 nextIncrement = increment;
	if (samples == 0 || increment.iValue == 0 || Length == 0)
		return 0;
	// The following fixes 3 or 4 bugs and allows
	// for loops to run correctly. DO NOT REMOVE!
	if (Length > LoopEnd)
		Length = LoopEnd;
	if (Pos < loopStart)
	{
		if (increment.iValue < 0)
		{
			int delta = ((loopStart - Pos) << 16U) - (PosLo & 0xFFFFU);
			Pos = loopStart + (delta >> 16U);
			PosLo = delta & 0xFFFFU;
			if (Pos < loopStart || Pos >= (loopStart + Length) / 2)
			{
				Pos = loopStart;
				PosLo = 0;
			}
			nextIncrement.iValue = -nextIncrement.iValue;
			increment.iValue = nextIncrement.iValue;
			if ((Flags & CHN_LOOP) == 0 || Pos >= Length)
			{
				Pos = Length;
				PosLo = 0;
				return 0;
			}
		}
	}
	else if (Pos >= Length)
	{
		if ((Flags & CHN_LOOP) == 0)
			return 0;
		if ((Flags & CHN_LPINGPONG) != 0)
		{
			uint32_t delta = ((~PosLo) & 0xFFFFU) + 1U;
			if (nextIncrement.iValue > 0)
			{
				nextIncrement.iValue = -nextIncrement.iValue;
				increment.iValue = nextIncrement.iValue;
			}
			Pos -= ((Pos - Length) << 1U) + (delta >> 16U);
			PosLo = delta & 0xFFFFU;
			if (Pos <= LoopStart || Pos >= Length)
				Pos = Length - 1;
		}
		else
		{
			if (increment.iValue < 0) // Theory says this is imposible..
			{
				printf("This should not happen\n");
				nextIncrement.iValue = -nextIncrement.iValue;
				increment.iValue = nextIncrement.iValue;
			}
			Pos -= Length - loopStart;
			if (Pos < loopStart)
				Pos = loopStart;
		}
	}
	if (Pos < loopStart)
	{
		if ((Pos & 0x80000000U) || increment.iValue < 0)
			return 0;
	}
	if ((Pos & 0x80000000U) || Pos >= Length)
		return 0;
	sampleCount = samples;
	if (nextIncrement.iValue < 0)
		nextIncrement.iValue = -nextIncrement.iValue;
	maxSamples = 16384U / (nextIncrement.Value.Hi + 1U);
	if (maxSamples < 2)
		maxSamples = 2;
	if (samples > maxSamples)
		samples = maxSamples;
	deltaHi = nextIncrement.Value.Hi * (samples - 1U);
	deltaLo = nextIncrement.Value.Lo * (samples - 1U);
	if (increment.iValue < 0)
	{
		uint32_t posDest = Pos - deltaHi - ((deltaLo - PosLo) >> 16U);
		if (posDest & 0x80000000U || posDest < loopStart)
		{
			sampleCount = (((Pos - loopStart) << 16U) + PosLo) / nextIncrement.iValue;
			++sampleCount;
		}
	}
	else
	{
		uint32_t posDest = Pos + deltaHi + ((deltaLo + PosLo) >> 16U);
		if (posDest >= Length)
		{
			sampleCount = (((Length - Pos) << 16U) - PosLo) / nextIncrement.iValue;
			++sampleCount;
		}
	}
	if (sampleCount <= 1U)
		return 1;
	if (sampleCount > samples)
		return samples;
	return sampleCount;
}

inline void ModuleFile::FixDCOffset(int *p_DCOffsL, int *p_DCOffsR, int *buff, uint32_t samples)
{
	int DCOffsL = *p_DCOffsL;
	int DCOffsR = *p_DCOffsR;
	while (samples != 0 && (DCOffsR | DCOffsL) != 0)
	{
		int OffsL = -DCOffsL;
		int OffsR = -DCOffsR;

		OffsL >>= 31U;
		OffsR >>= 31U;
		OffsL &= 0xFFU;
		OffsR &= 0xFFU;
		OffsL += DCOffsL;
		OffsR += DCOffsR;
		OffsL >>= 8U;
		OffsR >>= 8U;
		DCOffsL -= OffsL;
		DCOffsR -= OffsR;
		buff[0] += DCOffsR;
		buff[1] += DCOffsL;
		buff += 2;
		samples--;
	}
	*p_DCOffsL = DCOffsL;
	*p_DCOffsR = DCOffsR;
}

inline void ModuleFile::DCFixingFill(uint32_t samples)
{
	int *buff = MixBuffer;
	for (uint32_t i = 0; i < samples; i++)
	{
		buff[0] = 0;
		buff[1] = 0;
		buff += 2;
	}
	FixDCOffset(&DCOffsL, &DCOffsR, MixBuffer, samples);
}

void ModuleFile::CreateStereoMix(uint32_t count)
{
	int SampleCount;
	uint32_t /*Flags, */rampSamples;
	if (count == 0)
		return;
	/*Flags = GetResamplingFlag();*/
	for (uint32_t i = 0; i < nMixerChannels; i++)
	{
		uint32_t samples = count;
		int *buff = MixBuffer;
		channel_t * const channel = &Channels[MixerChannels[i]];
		if (channel->SampleData == nullptr)
			continue;
		do
		{
			rampSamples = samples;
			if (channel->RampLength > 0)
			{
				if (rampSamples > channel->RampLength)
					rampSamples = channel->RampLength;
			}
			SampleCount = channel->GetSampleCount(rampSamples);
			if (SampleCount <= 0)
			{
				FixDCOffset(&channel->DCOffsL, &channel->DCOffsR, buff, samples);
				DCOffsL += channel->DCOffsL;
				DCOffsR += channel->DCOffsR;
				channel->DCOffsL = channel->DCOffsR = 0;
				samples = 0;
				continue;
			}
			if (channel->RampLength == 0 && (channel->leftVol | channel->rightVol) == 0)
				buff += SampleCount * 2;
			else
			{
				MixInterface MixFunc = MixFunctionTable[/*Flags*/MIX_NOSRC | (channel->RampLength ? MIX_RAMP : 0) |
					(channel->Sample->Get16Bit() ? MIX_16BIT : 0) | (channel->Sample->GetStereo() ? MIX_STEREO : 0)];
				int *BuffMax = buff + (SampleCount * 2U);
				channel->DCOffsR = -((BuffMax - 2U)[0]);
				channel->DCOffsL = -((BuffMax - 2U)[1]);
				MixFunc(channel, buff, BuffMax);
				channel->DCOffsR += ((BuffMax - 2U)[0]);
				channel->DCOffsL += ((BuffMax - 2U)[1]);
				buff = BuffMax;
			}
			samples -= SampleCount;
			if (channel->RampLength != 0)
			{
				channel->RampLength -= SampleCount;
				if (channel->RampLength <= 0)
				{
					channel->RampLength = 0;
					channel->leftVol = channel->NewLeftVol;
					channel->rightVol = channel->NewRightVol;
					channel->LeftRamp = channel->RightRamp = 0;
					channel->Flags &= ~(CHN_FASTVOLRAMP | CHN_VOLUMERAMP);
				}
			}
		}
		while (samples > 0);
	}
}

inline void ModuleFile::MonoFromStereo(uint32_t count)
{
	for (uint32_t i = 0; i < count; i++)
		MixBuffer[i] = MixBuffer[i << 1U];
}

int32_t ModuleFile::Mix(uint8_t *Buffer, uint32_t BuffLen)
{
	uint32_t Count, SampleCount, Mixed = 0;
	uint32_t SampleSize = MixBitsPerSample / 8U * MixChannels;
	uint32_t Max = BuffLen / SampleSize;

	if (Max == 0)
		return -2;
	if (NextPattern >= p_Header->nOrders)
		return (Mixed == 0 ? -2 : Mixed * (MixBitsPerSample / 8U) * MixChannels);
	while (Mixed < Max)
	{
		if (SamplesToMix == 0)
		{
			// TODO: Deal with song fading
			if (!AdvanceTick())
				Max = Mixed;
		}
		Count = SamplesToMix;
		if (Count > MIXBUFFERSIZE)
			Count = MIXBUFFERSIZE;
		if (Count > (Max - Mixed))
			Count = (Max - Mixed);
		if (Count == 0)
			break;
		SampleCount = Count;
		// Reset the sound buffer.
		DCFixingFill(SampleCount);
		// MixOutChannels can only be one or two
		if (MixChannels == 2)
		{
			SampleCount *= 2;
			CreateStereoMix(Count);
			// Reverb processing?
		}
		else
		{
			CreateStereoMix(Count);
			// Reverb processing?
			MonoFromStereo(Count);
		}
		Buffer += Convert32to16(Buffer, MixBuffer, SampleCount);
		Mixed += Count;
		SamplesToMix -= Count;
	}
	return (Mixed == 0 ? -2 : Mixed * (MixBitsPerSample / 8U) * MixChannels);
}
