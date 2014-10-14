#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "../genericModule/genericModule.h"
#include "../fixedPoint/fixedPoint.h"

#include "waveTables.h"
#include "moduleMixer.h"
#include "mixFunctions.h"
#include "mixFunctionTables.h"
#include "frequencyTables.h"

uint32_t __CDECL__ Convert32to16(void *_out, int32_t *_in, uint32_t SampleCount)
{
	uint32_t i;
	signed short *out = (signed short *)_out;
	for (i = 0; i < SampleCount; i++)
	{
		int32_t samp = _in[i]/* + (1 << 11)*/;
		CLIPINT(samp, (int32_t)0xF8000001, 0x07FFFFFF);
		out[i] = samp >> 12;
	}
	return SampleCount << 1;
}

void ModuleFile::InitMixer(FileInfo *p_FI)
{
	MixSampleRate = p_FI->BitRate;
	MixChannels = p_FI->Channels;
	MixBitsPerSample = p_FI->BitsPerSample;
	MusicSpeed = p_Header->InitialSpeed;
	MusicTempo = p_Header->InitialTempo;
	TickCount = MusicSpeed;
	if (ModuleType != MODULE_IT)
		GlobalVolume = p_Header->GlobalVolume << 1;
	else
		GlobalVolume = p_Header->GlobalVolume;
	SamplesPerTick = (MixSampleRate * 640) / (MusicTempo << 8);
	Channels = new Channel[p_Header->nChannels]();
	MixerChannels = new uint32_t[p_Header->nChannels];
	Rows = 2;
	ResetChannelPanning();
	InitialiseTables();
}

Channel::Channel()
{
	ChannelVolume = 64;
}

void ModuleFile::DeinitMixer()
{
	DeinitialiseTables();

	delete [] Channels;
	delete [] MixerChannels;
}

void ModuleFile::ResetChannelPanning()
{
	uint8_t i;
	if (ModuleType == MODULE_MOD)
	{
		for (i = 0; i < p_Header->nChannels; i++)
		{
			uint8_t j = i % 4;
			if (j == 0 || j == 3)
				Channels[i].Panning = 0;
			else
				Channels[i].Panning = 128;
		}
	}
	else if (ModuleType == MODULE_S3M || ModuleType == MODULE_IT)
	{
		if (p_Header->Panning == NULL)
		{
			for (i = 0; i < p_Header->nChannels; i++)
				Channels[i].Panning = 64;
		}
		else
		{
			for (i = 0; i < p_Header->nChannels; i++)
				Channels[i].Panning = p_Header->Panning[i];
		}
	}
	else if (ModuleType == MODULE_STM)
	{
		for (i = 0; i < p_Header->nChannels; i++)
		{
			if ((i % 2) != 0)
				Channels[i].Panning = 32;
			else
				Channels[i].Panning = 96;
		}
	}
}

void ModuleFile::ReloadSample(Channel *channel)
{
	ModuleSample *sample = channel->Sample;
	channel->RawVolume = sample->GetVolume();
	channel->NewSample = 0;
	channel->Length = sample->GetLength();
	channel->LoopStart = (sample->GetLoopStart() < channel->Length ? sample->GetLoopStart() : channel->Length);
	channel->LoopEnd = sample->GetLoopEnd();
	if (sample->GetLooped())
		channel->Flags |= CHN_LOOP;
	else
		channel->Flags &= ~CHN_LOOP;
	if (sample->GetBidiLoop())
		channel->Flags |= CHN_LPINGPONG;
	else
		channel->Flags &= ~CHN_LPINGPONG;
	channel->NewSampleData = p_PCM[sample->ID];
	channel->FineTune = sample->GetFineTune();
	channel->C4Speed = sample->GetC4Speed();
	if (channel->LoopEnd > channel->Length)
		channel->LoopEnd = channel->Length;
	if (channel->Length > channel->LoopEnd)
		channel->Length = channel->LoopEnd;
	channel->NewSample = 0;
	channel->AutoVibratoPos = 0;
}

void ModuleFile::SampleChange(Channel *channel, uint32_t nSample)
{
	if (p_Instruments != NULL)
	{
		ModuleInstrument *instr = p_Instruments[nSample - 1];
		uint8_t nSample = instr->Map(channel->Note - 1);
		channel->EnvVolumePos = 0;
		if (nSample == 0)
		{
			channel->Sample = NULL;
			channel->NewSampleData = NULL;
			channel->Length = 0;
			return;
		}
		channel->Instrument = instr;
	}
	channel->Sample = p_Samples[nSample - 1];
	ReloadSample(channel);
}

uint32_t ModuleFile::GetPeriodFromNote(uint8_t Note, uint8_t FineTune, uint32_t C4Speed)
{
	if (Note == 0 || Note > 0xF0)
		return 0;
	Note--;
	if (ModuleType == MODULE_S3M || ModuleType == MODULE_IT || ModuleType == MODULE_STM)
	{
		if ((p_Header->Flags & FILE_FLAGS_AMIGA_SLIDES) == 0)
			return (S3MPeriods[Note % 12] << 5) >> (Note / 12);
		else
		{
			if (C4Speed == 0)
				C4Speed = 8363;
			return muldiv(8363, (S3MPeriods[Note % 12] << 5), C4Speed << (Note / 12));
		}
	}
	else
	{
		if (FineTune != 0 || Note < 36 || Note >= 108)
			return (MODTunedPeriods[(FineTune * 12) + (Note % 12)] << 5) >> (Note / 12);
		else
			return MODPeriods[Note - 36] << 2;
	}
}

uint32_t ModuleFile::GetFreqFromPeriod(uint32_t Period, uint32_t C4Speed, int32_t PeriodFrac)
{
	if (Period == 0)
		return 0;
	if (ModuleType == MODULE_MOD)
		return 14187580UL / Period;
	else// if (ModuleType == MODULE_S3M)
	{
		if ((p_Header->Flags & FILE_FLAGS_AMIGA_SLIDES) == 0)
		{
			if (C4Speed == 0)
				C4Speed = 8363;
			return muldiv(C4Speed, 438272UL, (Period << 8) + PeriodFrac);
		}
		else
			return muldiv(8363, 438272UL, (Period << 8) + PeriodFrac);
	}
}

void ModuleFile::NoteChange(Channel * const channel, uint8_t note, uint8_t cmd)
{
	uint32_t period;
	ModuleSample *sample = channel->Sample;

	if (note < 1)
		return;
	if (note >= 0x80)
	{
		if (ModuleType == MODULE_IT)
		{
			if (note == 0xFF)
				channel->NoteOff();
			else
				channel->Flags |= CHN_NOTEFADE;
		}
		else
			channel->NoteOff();

		if (note == 0xFE)
			channel->NoteCut(true);
		return;
	}
	channel->Note = note;
	if (channel->Instrument != NULL)
	{
		uint8_t nSample = channel->Instrument->Map(channel->Note - 1);
		if (nSample == 0)
			sample = NULL;
		else
			sample = p_Samples[nSample - 1];
		channel->EnvVolumePos = 0;
		channel->EnvPitchPos = 0;
		channel->Sample = sample;
		if (sample != NULL)
			ReloadSample(channel);
		else
			channel->NewSampleData = NULL;
	}
	channel->NewSample = 0;
	period = GetPeriodFromNote(note, channel->FineTune, channel->C4Speed);
	if (sample == NULL)
		return;
	if (period != 0)
	{
		if (cmd != CMD_TONEPORTAMENTO && cmd != CMD_TONEPORTAVOL)
			channel->Period = period;
		channel->PortamentoDest = period;
		if (channel->PortamentoDest == channel->Period || (channel->Length == 0 && ModuleType != MODULE_S3M))
		{
			channel->Sample = sample;
			channel->NewSampleData = p_PCM[sample->ID];
			channel->Length = sample->GetLength();
			if (sample->GetLooped())
			{
				channel->Flags |= CHN_LOOP;
				channel->LoopStart = (sample->GetLoopStart() < sample->GetLength() ? sample->GetLoopStart() : sample->GetLength());
				channel->LoopEnd = sample->GetLoopEnd();
			}
			else
			{
				channel->Flags &= ~CHN_LOOP;
				channel->LoopStart = 0;
				channel->LoopEnd = sample->GetLength();
			}
			if (sample->GetBidiLoop())
				channel->Flags |= CHN_LPINGPONG;
			else
				channel->Flags &= ~CHN_LPINGPONG;
			channel->Pos = 0;
			if ((channel->TremoloType & 0x04) != 0)
				channel->TremoloPos = 0;
		}
		if (channel->Pos > channel->Length)
			channel->Pos = channel->Length;
		channel->PosLo = 0;
	}
	if (period == 0 || ModuleType != MODULE_IT || ((channel->Flags & CHN_NOTEFADE) != 0 && channel->FadeOutVol == 0))
	{
		if (ModuleType == MODULE_IT && (channel->Flags & CHN_NOTEFADE) != 0 && channel->FadeOutVol == 0)
		{
			// Reset envelopes and auto-vibrato*
			channel->Flags &= ~CHN_NOTEFADE;
			channel->FadeOutVol = 0xFFFF;
		}
		else
		{
			channel->Flags &= ~CHN_NOTEFADE;
			channel->FadeOutVol = 0xFFFF;
		}
	}
	channel->Flags &= ~CHN_NOTEOFF;
	if (channel->PortamentoDest == channel->Period)
		channel->Flags |= CHN_FASTVOLRAMP;
	channel->LeftVol = channel->RightVol = 0;
}

void ModuleFile::ProcessMODExtended(Channel *channel)
{
	uint8_t param = channel->RowParam;
	uint8_t cmd = (param & 0xF0) >> 4;
	param &= 0x0F;

	switch (cmd)
	{
		case CMD_MODEX_FILTER:
			// Could implement a 7Khz lowpass filter
			// For this, or just do what MPT does - ignore it.
			// XXX: Currently we ignore this.
			break;
		case CMD_MODEX_FINEPORTAUP:
			if (TickCount == channel->StartTick)
			{
				if (channel->Period != 0 && param != 0)
				{
					channel->Period -= param << 2;
					if (channel->Period < 56)
						channel->Period = 56;
				}
			}
			break;
		case CMD_MODEX_FINEPORTADOWN:
			if (TickCount == channel->StartTick)
			{
				if (channel->Period != 0 && param != 0)
				{
					channel->Period += param << 2;
					if (channel->Period > 7040)
						channel->Period = 7040;
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
			channel->VibratoType = param & 0x07;
			break;
		case CMD_MODEX_FINETUNE:
			if (TickCount != channel->StartTick)
				break;
			channel->C4Speed = S3MSpeedTable[param];
			channel->FineTune = param;
			if (channel->Period != 0)
				channel->Period = GetPeriodFromNote(channel->Note, channel->FineTune, channel->C4Speed);
			break;
		case CMD_MODEX_TREMOLOWAVE:
			channel->TremoloType = param & 0x07;
			break;
		case CMD_MODEX_RETRIGER:
			if (param != 0 && (TickCount % param) == 0)
				NoteChange(channel, channel->NewNote, 0);
			break;
		case CMD_MODEX_FINEVOLUP:
			if (param != 0 && TickCount == channel->StartTick)
			{
				param <<= 1; // << 1?
				if (128 - param > channel->RawVolume)
					channel->RawVolume += param;
				else
					channel->RawVolume = 128;
			}
			break;
		case CMD_MODEX_FINEVOLDOWN:
			if (param != 0 && TickCount == channel->StartTick)
			{
				param <<= 1; // << 1?
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
				channel->Increment.iValue = -channel->Increment.iValue;
			break;
	}
}

void ModuleFile::ProcessS3MExtended(Channel *channel)
{
	uint8_t param = channel->RowParam;
	uint8_t cmd = (param & 0xF0) >> 4;
	param &= 0x0F;

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
			if (TickCount != channel->StartTick)
				break;
			channel->C4Speed = S3MSpeedTable[param];
			channel->FineTune =  param;
			if (channel->Period != 0)
				channel->Period = GetPeriodFromNote(channel->Note, channel->FineTune, channel->C4Speed);
			break;
		case CMD_S3MEX_VIBRATOWAVE:
			channel->VibratoType = param & 0x07;
			break;
		case CMD_S3MEX_TREMOLOWAVE:
			channel->TremoloType = param & 0x07;
			break;
		case CMD_S3MEX_PANWAVE:
			channel->PanbrelloType = param & 0x07;
			break;
		case CMD_S3MEX_FRAMEDELAY:
			FrameDelay = param;
			break;
		case CMD_S3MEX_ENVELOPE:
			if (TickCount == channel->StartTick)
			{
			}
			break;
		case CMD_S3MEX_PANNING:
			if (TickCount == channel->StartTick)
			{
				channel->Panning = (param + (param << 4)) >> 1;
				channel->Flags |= CHN_FASTVOLRAMP;
			}
			break;
		case CMD_S3MEX_SURROUND:
			break;
		case CMD_S3MEX_OFFSET:
			if (TickCount == channel->StartTick && channel->RowNote != 0 && channel->RowNote < 0x80)
			{
				int pos = param << 16;
				if (pos < (int)channel->Length)
					channel->Pos = pos;
			}
			break;
		case CMD_S3MEX_NOTECUT:
			channel->NoteCut(TickCount == param);
			break;
	}
}

inline int ModuleFile::PatternLoop(uint32_t param)
{
	if (param != 0)
	{
		if (PatternLoopCount != 0)
		{
			PatternLoopCount--;
			if (PatternLoopCount == 0)
			{
				// Reset the default start position for the next
				// CMDEX_LOOP
				PatternLoopStart = 0;
				return -1;
			}
		}
		else
			PatternLoopCount = param;
		return PatternLoopStart;
	}
	else
		PatternLoopStart = Row;
	return -1;
}

inline void ModuleFile::VolumeSlide(Channel *channel, uint8_t param)
{
	short NewVolume;

	if (param == 0)
		param = channel->VolumeSlide;
	else
		channel->VolumeSlide = param;
	NewVolume = channel->RawVolume;

	// TODO: Complete recode to take into account S3M FineVolume slides
	if (ModuleType == MODULE_S3M || ModuleType == MODULE_STM)
	{
		if ((param & 0x0F) == 0x0F)
		{
			if ((param & 0xF0) != 0)
				return; // FineVolumeUp(channel, param >> 4);
			else if (TickCount > channel->StartTick)
				NewVolume -= 0x1E; //0x0F * 2;
		}
		else if ((param & 0xF0) == 0xF0)
		{
			if ((param & 0x0F) != 0)
				return; // FineVolumeDown(channel, param & 0x0F);
			else if (TickCount > channel->StartTick)
				NewVolume += 0x1E; //0x0F * 2;
		}
	}

	if (TickCount > channel->StartTick)
	{
		if ((param & 0xF0) != 0)
			NewVolume += (param & 0xF0) >> 3;
		else
			NewVolume -= (param & 0x0F) << 1;
		if (ModuleType == MODULE_MOD)
			channel->Flags |= CHN_FASTVOLRAMP;
		CLIPINT(NewVolume, 0, 128);
		channel->RawVolume = NewVolume & 0xFF;
	}
}

// TODO: Write these next two functions as one template as both
// do the exact same thing, just on different variables.
inline void ModuleFile::ChannelVolumeSlide(Channel *channel, uint8_t param)
{
	bool FirstTick = (TickCount == 0);
	int16_t SlideDest = channel->ChannelVolume;

	if (param == 0)
		param = channel->ChannelVolumeSlide;
	else
		channel->ChannelVolumeSlide = param;

	if ((param & 0xF0) == 0xF0 && (param & 0x0F) != 0)
	{
		if (FirstTick)
			SlideDest -= param & 0x0F;
	}
	else if ((param & 0x0F) == 0x0F && (param & 0xF0) != 0)
	{
		if (FirstTick)
			SlideDest += (param & 0xF0) >> 4;
	}
	else if (!FirstTick)
	{
		if ((param & 0x0F) != 0)
			SlideDest -= param & 0x0F;
		else
			SlideDest += (param & 0xF0) >> 4;
	}

	if (SlideDest != channel->ChannelVolume)
	{
		CLIPINT(SlideDest, 0, 64);
		channel->ChannelVolume = SlideDest;
	}
}

inline void ModuleFile::GlobalVolumeSlide(uint8_t param)
{
	bool FirstTick = (TickCount == 0);
	int16_t SlideDest = GlobalVolume;

	if (param == 0)
		param = GlobalVolSlide;
	else
		GlobalVolSlide = param;

	if ((param & 0xF0) == 0xF0 && (param & 0x0F) != 0)
	{
		if (FirstTick)
			SlideDest -= (param & 0x0F) << 1;
	}
	else if ((param & 0x0F) == 0x0F && (param & 0xF0) != 0)
	{
		if (FirstTick)
			SlideDest += (param & 0xF0) >> 3;
	}
	else if (!FirstTick)
	{
		if ((param & 0x0F) != 0)
			SlideDest -= (param & 0x0F) << 1;
		else if ((param & 0xF0) != 0)
			SlideDest += (param & 0xF0) >> 3;
	}

	if (SlideDest != GlobalVolume)
	{
		CLIPINT(SlideDest, 0, 128);
		GlobalVolume = SlideDest;
	}
}

// Returns ((period * 65536 * 2^(slide / 192)) + 32768) / 65536 using fixed-point maths
inline uint32_t LinearSlideUp(uint32_t period, uint8_t slide)
{
	const fixed64_t c192(192);
	const fixed64_t c32768(32768);
	const fixed64_t c65536(65536);
	return (((fixed64_t(period) * (fixed64_t(slide) / c192).pow2() * c65536) + c32768) / c65536).operator int();
}

// Returns ((period * 65535 * 2^(-slide / 192)) + 32768) / 65536 using fixed-point maths
inline uint32_t LinearSlideDown(uint32_t period, uint8_t slide)
{
	const fixed64_t c192(192);
	const fixed64_t c32768(32768);
	const fixed64_t c65535(65535);
	const fixed64_t c65536(65536);
	return (((fixed64_t(period) * (fixed64_t(slide, 0, -1) / c192).pow2() * c65535) + c32768) / c65536).operator int();
}

// Returns ((period * 65536 * 2^((slide / 4) / 192)) + 32768) / 65536 using fixed-point maths
inline uint32_t FineLinearSlideUp(uint32_t period, uint8_t slide)
{
	const fixed64_t c4(4);
	const fixed64_t c192(192);
	const fixed64_t c32768(32768);
	const fixed64_t c65536(65536);
	return (((fixed64_t(period) * ((fixed64_t(slide) / c4) / c192).pow2() * c65536) + c32768) / c65536).operator int();
}

// Returns ((period * 65535 * 2^((-slide / 4) / 192)) + 32768) / 65536 using fixed-point maths
inline uint32_t FineLinearSlideDown(uint32_t period, uint8_t slide)
{
	const fixed64_t c4(4);
	const fixed64_t c192(192);
	const fixed64_t c32768(32768);
	const fixed64_t c65535(65535);
	const fixed64_t c65536(65536);
	return (((fixed64_t(period) * ((fixed64_t(slide, 0, -1) / c4) / c192).pow2() * c65535) + c32768) / c65536).operator int();
}

inline void ModuleFile::PortamentoUp(Channel *channel, uint8_t param)
{
	if (param != 0)
		channel->Portamento = param;
	else
		param = channel->Portamento;
	if (channel->Period == 0)
		return;
	if ((ModuleType == MODULE_S3M || ModuleType == MODULE_STM || ModuleType == MODULE_IT) && (param & 0xE0) == 0xE0)
	{
		if ((param & 0x0F) != 0)
		{
			if ((param & 0x10) == 0x10)
				FinePortamentoUp(channel, param & 0x0F);
			else
				ExtraFinePortamentoUp(channel, param & 0x0F);
		}
		return;
	}
	if (TickCount > channel->StartTick || MusicSpeed == 1)
	{
		if ((p_Header->Flags & FILE_FLAGS_LINEAR_SLIDES) != 0)// && ModuleType != MODULE_XM)
		{
			uint32_t OldPeriod = channel->Period;
			if (param != 0)
			{
				channel->Period = LinearSlideDown(OldPeriod, param);
				if (channel->Period == OldPeriod)
					channel->Period--;
			}
		}
		else
			channel->Period -= param << 2;
		if (channel->Period > MaxPeriod)
		{
			channel->Period = MaxPeriod;
			if (ModuleType == MODULE_IT)
			{
				channel->Flags |= CHN_NOTEFADE;
				channel->FadeOutVol = 0;
			}
		}
	}
}

inline void ModuleFile::PortamentoDown(Channel *channel, uint8_t param)
{
	if (param != 0)
		channel->Portamento = param;
	else
		param = channel->Portamento;
	if (channel->Period == 0)
		return;
	if ((ModuleType == MODULE_S3M || ModuleType == MODULE_STM || ModuleType == MODULE_IT) && (param & 0xE0) == 0xE0)
	{
		if ((param & 0x0F) != 0)
		{
			if ((param & 0x10) == 0x10)
				FinePortamentoDown(channel, param & 0x0F);
			else
				ExtraFinePortamentoDown(channel, param & 0x0F);
		}
		return;
	}
	if (TickCount > channel->StartTick || MusicSpeed == 1)
	{
		if ((p_Header->Flags & FILE_FLAGS_LINEAR_SLIDES) != 0)// && ModuleType != MODULE_XM)
		{
			uint32_t OldPeriod = channel->Period;
			if (param != 0)
			{
				channel->Period = LinearSlideUp(OldPeriod, param);
				if (channel->Period == OldPeriod)
					channel->Period++;
			}
		}
		else
			channel->Period += param << 2;
		if (channel->Period > MaxPeriod)
		{
			channel->Period = MaxPeriod;
			if (ModuleType == MODULE_IT)
			{
				channel->Flags |= CHN_NOTEFADE;
				channel->FadeOutVol = 0;
			}
		}
	}
}

inline void ModuleFile::FinePortamentoUp(Channel *channel, uint8_t param)
{
	if (TickCount == channel->StartTick && channel->Period != 0 && param != 0)
	{
		if ((p_Header->Flags & FILE_FLAGS_LINEAR_SLIDES) != 0)
			channel->Period = LinearSlideDown(channel->Period, param);
		else
			channel->Period -= param << 2;
		if (channel->Period < MinPeriod)
			channel->Period = MinPeriod;
	}
}

inline void ModuleFile::FinePortamentoDown(Channel *channel, uint8_t param)
{
	if (TickCount == channel->StartTick && channel->Period != 0 && param != 0)
	{
		if ((p_Header->Flags & FILE_FLAGS_LINEAR_SLIDES) != 0)
			channel->Period = LinearSlideUp(channel->Period, param);
		else
			channel->Period += param << 2;
		if (channel->Period > MaxPeriod)
			channel->Period = MaxPeriod;
	}
}

inline void ModuleFile::ExtraFinePortamentoUp(Channel *channel, uint8_t param)
{
	if (TickCount == channel->StartTick && channel->Period != 0 && param != 0)
	{
		if ((p_Header->Flags & FILE_FLAGS_LINEAR_SLIDES) != 0)
			channel->Period = FineLinearSlideDown(channel->Period, param);
		else
			channel->Period -= param;
		if (channel->Period < MinPeriod)
			channel->Period = MinPeriod;
	}
}

inline void ModuleFile::ExtraFinePortamentoDown(Channel *channel, uint8_t param)
{
	if (TickCount == channel->StartTick && channel->Period != 0 && param != 0)
	{
		if ((p_Header->Flags & FILE_FLAGS_LINEAR_SLIDES) != 0)
			channel->Period = FineLinearSlideUp(channel->Period, param);
		else
			channel->Period += param;
		if (channel->Period > MaxPeriod)
			channel->Period = MaxPeriod;
	}
}

inline void ModuleFile::TonePortamento(Channel *channel, uint8_t param)
{
	if (param != 0)
		channel->PortamentoSlide = ((uint16_t)param) << 2;
	channel->Flags |= CHN_PORTAMENTO;
	if (channel->Period != 0 && channel->PortamentoDest != 0)
	{
		if (channel->Period < channel->PortamentoDest)
		{
			int Delta;
			if ((channel->Flags & CHN_GLISSANDO) != 0)
			{
				uint8_t Slide = (uint8_t)(channel->PortamentoSlide >> 2);
				Delta = LinearSlideUp(channel->Period, Slide) - channel->Period;
				if (Delta < 1)
					Delta = 1;
			}
			else
				Delta = channel->PortamentoSlide;
			if (channel->PortamentoDest - channel->Period < (uint32_t)Delta)
				Delta = channel->PortamentoDest - channel->Period;
			channel->Period += Delta;
		}
		else if (channel->Period > channel->PortamentoDest)
		{
			int Delta;
			if ((channel->Flags & CHN_GLISSANDO) != 0)
			{
				uint8_t Slide = (uint8_t)(channel->PortamentoSlide >> 2);
				Delta = LinearSlideDown(channel->Period, Slide) - channel->Period;
				if (Delta > -1)
					Delta = -1;
			}
			else
				Delta = -((int)channel->PortamentoSlide);
			if (channel->PortamentoDest - channel->Period > ((uint32_t)Delta))
				Delta = channel->PortamentoDest - channel->Period;
			channel->Period += Delta;
		}
	}
}

void Channel::NoteCut(bool Triggered)
{
	if (Triggered)
	{
		RawVolume = 0;
		FadeOutVol = 0;
		Flags |= CHN_FASTVOLRAMP | CHN_NOTEFADE;
	}
}

void Channel::NoteOff()
{
	bool NoteOn = !(Flags & CHN_NOTEOFF);
	Flags |= CHN_NOTEOFF;
	if (Instrument != NULL && Instrument->GetEnvEnabled(ENVELOPE_VOLUME))
		Flags |= CHN_NOTEFADE;
	if (Length == 0)
		return;
	if (false && Sample != NULL && NoteOn)
	{
		if (LoopEnd != 0)
		{
			if (Sample->GetBidiLoop())
				Flags |= CHN_LPINGPONG;
			else
				Flags &= ~CHN_LPINGPONG;
			Flags |= CHN_LOOP;
			Length = Sample->GetLength();
			LoopStart = Sample->GetLoopStart();
			LoopEnd = Sample->GetLoopEnd();
			if (LoopEnd > Length)
				LoopEnd = Length;
			if (Length > LoopEnd)
				Length = LoopEnd;
		}
		else
		{
			Flags &= ~(CHN_LOOP | CHN_LPINGPONG);
			Length = Sample->GetLength();
		}
	}
	if (Instrument != NULL)
	{
		if (Instrument->GetEnvLooped(ENVELOPE_VOLUME) && Instrument->GetFadeOut() != 0)
			Flags |= CHN_NOTEFADE;
	}
}

void Channel::Vibrato(uint8_t param, uint8_t Multiplier)
{
	if ((param & 0x0F) != 0)
		VibratoDepth = (param & 0x0F) * Multiplier;
	if ((param & 0xF0) != 0)
		VibratoSpeed = param >> 4;
	Flags |= CHN_VIBRATO;
}

void Channel::Panbrello(uint8_t param)
{
	if ((param & 0x0F) != 0)
		PanbrelloDepth = param & 0x0F;
	if ((param & 0xF0) != 0)
		PanbrelloSpeed = param >> 4;
	Flags |= CHN_PANBRELLO;
}

bool ModuleFile::ProcessEffects()
{
	int PositionJump = -1, BreakRow = -1, PatternLoopRow = -1;
	uint32_t i;
	for (i = 0; i < p_Header->nChannels; i++)
	{
		Channel *channel = &Channels[i];
		uint8_t sample = channel->RowSample;
		uint8_t cmd = channel->RowEffect;
		uint8_t param = channel->RowParam;

		channel->Flags &= ~CHN_FASTVOLRAMP;
		if (cmd == CMD_MOD_EXTENDED || cmd == CMD_S3M_EXTENDED)
		{
			uint8_t excmd = (param & 0xF0) >> 4;
			if (channel->StartTick == 0 && TickCount == 0)
			{
				if ((cmd == CMD_MOD_EXTENDED && excmd == CMD_MODEX_LOOP) ||
					(cmd == CMD_S3M_EXTENDED && excmd == CMD_S3MEX_LOOP))
				{
					int loop = PatternLoop(param & 0x0F);
					if (loop >= 0)
						PatternLoopRow = loop;
				}
				else if (excmd == CMD_MODEX_DELAYPAT)
					PatternDelay = param & 0x0F;
			}
		}

		if (TickCount == channel->StartTick)
		{
			uint8_t note = channel->RowNote;
			if (sample != 0)
				channel->NewSample = sample;
			if (ModuleType == MODULE_MOD && note == 0xFF)
			{
				channel->Flags |= CHN_FASTVOLRAMP;
				channel->RawVolume = 0;
				note = sample = 0;
			}
			if (note == 0 && sample != 0)
				channel->RawVolume = p_Samples[sample - 1]->GetVolume();
			if (note >= 0xFE)
				sample = 0;
			if (note != 0 && note <= 128)
				channel->NewNote = note;
			if (sample != 0)
				SampleChange(channel, sample);
			if (note != 0)
			{
				if (sample == 0 && channel->NewSample != 0 && note <= 0x80)
					SampleChange(channel, channel->NewSample);
				NoteChange(channel, note, cmd);
			}
			if (channel->RowVolEffect == VOLCMD_VOLUME)
			{
				channel->RawVolume = channel->RowVolParam << 1;
				channel->Flags |= CHN_FASTVOLRAMP;
			}
			else if (channel->RowVolEffect == VOLCMD_PANNING)
			{
				channel->Panning = channel->RowVolParam;
				channel->Flags |= CHN_FASTVOLRAMP;
			}
		}
		switch (cmd)
		{
			case CMD_NONE:
				break;
			case CMD_ARPEGGIO:
				if (TickCount == channel->StartTick && channel->Period != 0 && channel->Note != 0xFF)
				{
					if (param == 0 && ModuleType != MODULE_S3M)
						break;
					channel->Flags |= CHN_ARPEGGIO;
					if (param != 0)
						channel->Arpeggio = param;
				}
				break;
			case CMD_PORTAMENTOUP:
				PortamentoUp(channel, param);
				break;
			case CMD_PORTAMENTODOWN:
				PortamentoDown(channel, param);
				break;
			case CMD_TONEPORTAMENTO:
				TonePortamento(channel, param);
				break;
			case CMD_VIBRATO:
				channel->Vibrato(param, 4);
				break;
			case CMD_TONEPORTAVOL:
				if (param != 0)
					VolumeSlide(channel, param);
				TonePortamento(channel, 0);
				break;
			case CMD_VIBRATOVOL:
				if (param != 0)
					VolumeSlide(channel, param);
				channel->Flags |= CHN_VIBRATO;
				break;
			case CMD_TREMOLO:
				if ((param & 0x0F) != 0)
					channel->TremoloDepth = param & 0x0F;
				if ((param & 0xF0) != 0)
					channel->TremoloSpeed = param >> 4;
				channel->Flags |= CHN_TREMOLO;
				break;
			case CMD_OFFSET:
				if (TickCount > channel->StartTick)
					break;
				channel->Pos = ((uint32_t)param) << 8;
				channel->PosLo = 0;
				if (channel->Pos > channel->Length)
					channel->Pos = channel->Length;
				break;
			case CMD_VOLUMESLIDE:
				if (param != 0 || ModuleType != MODULE_MOD)
					VolumeSlide(channel, param);
				break;
			case CMD_CHANNELVOLSLIDE:
				ChannelVolumeSlide(channel, param);
				break;
			case CMD_GLOBALVOLSLIDE:
				GlobalVolumeSlide(param);
				break;
			case CMD_POSITIONJUMP:
				PositionJump = param;
				if (PositionJump > p_Header->nOrders)
					PositionJump = 0;
				break;
			case CMD_PATTERNBREAK:
				BreakRow = ((param >> 4) * 10) + (param & 0x0F);
				if (BreakRow > (Rows - 1))
					BreakRow = (Rows - 1);
				break;
			case CMD_SPEED:
				MusicSpeed = param;
				break;
			case CMD_MOD_EXTENDED:
				ProcessMODExtended(channel);
				break;
			case CMD_S3M_EXTENDED:
				ProcessS3MExtended(channel);
				break;
			case CMD_RETRIGER:
				if (param != 0 && (TickCount % param) == 0)
					NoteChange(channel, channel->NewNote, 0);
				break;
			case CMD_TEMPO:
				MusicTempo = param;
				break;
			case CMD_VOLUME:
				if (TickCount == 0)//channel->StartTick)
				{
					uint8_t NewVolume = param;
					if (NewVolume > 64)
						NewVolume = 64;
					channel->RawVolume = NewVolume << 1;
					channel->Flags |= CHN_FASTVOLRAMP;
				}
				break;
			case CMD_CHANNELVOLUME:
				if (TickCount == 0 && param < 65)
				{
					channel->ChannelVolume = param;
					channel->Flags |= CHN_FASTVOLRAMP;
				}
				break;
			case CMD_GLOBALVOLUME:
				if (TickCount == 0)
				{
					if (param > 128)
						param = 128;
					GlobalVolume = param;
				}
				break;
			case CMD_PANNING:
				if (TickCount > channel->StartTick)
					break;
				channel->Panning = param;
				channel->Flags |= CHN_FASTVOLRAMP;
				break;
			case CMD_FINEVIBRATO:
				channel->Vibrato(param, 1);
				break;
			case CMD_TREMOR:
				if (TickCount != 0)
					break;
				if (param != 0)
					channel->Tremor = param;
				channel->Flags |= CHN_TREMOR;
				break;
			case CMD_PANBRELLO:
				channel->Panbrello(param);
				break;
			default:
				break;
		}
	}
	if (TickCount == 0)
	{
		if (PatternLoopRow >= 0)
		{
			NextPattern = NewPattern;
			NextRow = PatternLoopRow;
			if (PatternDelay != 0)
				NextRow++;
		}
		else if (BreakRow >= 0 || PositionJump >= 0)
		{
			if (PositionJump < 0)
				PositionJump = NewPattern + 1;
			if (BreakRow < 0)
				BreakRow = 0;
			if ((uint32_t)PositionJump < NewPattern)// || ((uint32_t)PositionJump == NewPattern && (uint32_t)BreakRow <= Row))
			{
				NextPattern = p_Header->nOrders;
				return false;
			}
			else if (((uint32_t)PositionJump != NewPattern || (uint32_t)BreakRow != Row))
			{
				if ((uint32_t)PositionJump != NewPattern)
					PatternLoopCount = PatternLoopStart = 0;
				NextPattern = PositionJump;
				NextRow = BreakRow;
			}
		}
	}
	return true;
}

void Channel::SetData(ModuleCommand *Command, ModuleHeader *p_Header)
{
	uint8_t excmd;
	RowNote = Command->Note;
	RowSample = Command->Sample;
	if (RowSample > p_Header->nSamples)
		RowSample = 0;
	RowVolEffect = Command->VolEffect;
	RowVolParam = Command->VolParam;
	RowEffect = Command->Effect;
	RowParam = Command->Param;
	excmd = (RowParam & 0xF0) >> 4;
	if ((RowEffect == CMD_MOD_EXTENDED && excmd == CMD_MODEX_DELAYSAMP) ||
		(RowEffect == CMD_S3M_EXTENDED && excmd == CMD_S3MEX_DELAYSAMP))
		StartTick = RowParam & 0x0F;
	else
		StartTick = 0;
	Flags &= ~(CHN_TREMOLO | CHN_ARPEGGIO | CHN_VIBRATO | CHN_PORTAMENTO | CHN_GLISSANDO | CHN_TREMOR | CHN_PANBRELLO);
}

bool ModuleFile::Tick()
{
	TickCount++;
	if (TickCount >= (MusicSpeed * (PatternDelay + 1)) + FrameDelay)
	{
		uint32_t i;
		ModuleCommand **Commands;
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
		if (p_Patterns[Pattern] == NULL)
			return false;
		Commands = p_Patterns[Pattern]->GetCommands();
		Rows = p_Patterns[Pattern]->GetRows();
		for (i = 0; i < p_Header->nChannels; i++)
			Channels[i].SetData(&Commands[i][Row], p_Header);
	}
	if (MusicSpeed == 0)
		MusicSpeed = 1;
	return ProcessEffects();
}

bool ModuleFile::AdvanceTick()
{
	uint32_t i;

	if (Tick() == false)
		return false;
	if (MusicTempo == 0)
		return false;
	SamplesToMix = (MixSampleRate * 640) / (MusicTempo << 8);
	SamplesPerTick = SamplesToMix;
	nMixerChannels = 0;
	for (i = 0; i < p_Header->nChannels; i++)
	{
		Channel *channel = &Channels[i];
 		channel->Increment.iValue = 0;
		if (channel->Period != 0 && channel->Length != 0)
		{
			int inc, period, freq;
			short vol = channel->RawVolume;
			if ((channel->Flags & CHN_TREMOLO) != 0)
			{
				uint8_t TremoloPos = channel->TremoloPos;
				if (vol > 0)
				{
					uint8_t TremoloType = channel->TremoloType & 0x03;
					uint8_t TremoloDepth = channel->TremoloDepth << 4;
					if (TremoloType == 1)
						vol += (RampDownTable[TremoloPos] * TremoloDepth) >> 8;
					else if (TremoloType == 2)
						vol += (SquareTable[TremoloPos] * TremoloDepth) >> 8;
					else if (TremoloType == 3)
						vol += (RandomTable[TremoloPos] * TremoloDepth) >> 8;
					else
						vol += (SinusTable[TremoloPos] * TremoloDepth) >> 8;
				}
				if (TickCount > channel->StartTick)
					channel->TremoloPos = (TremoloPos + channel->TremoloSpeed) & 0x3F;
			}
			if ((channel->Flags & CHN_TREMOR) != 0)
			{
				uint8_t Duration = (channel->Tremor >> 4) + (channel->Tremor & 0x0F) + 2;
				uint8_t OnTime = (channel->Tremor >> 4) + 1;
				uint8_t Count = channel->TremorCount;
				if (Count > Duration)
					Count = 0;
				if (TickCount != 0 || ModuleType == MODULE_S3M)
				{
					if (Count > OnTime)
						vol = 0;
					channel->TremorCount = Count + 1;
				}
				channel->Flags |= CHN_FASTVOLRAMP;
			}
			/*CLIPINT(vol, 0, 128);
			//vol <<= 7;*/

			if (channel->Instrument != NULL)
			{
				ModuleInstrument *instr = channel->Instrument;
				ModuleEnvelope *env = instr->GetEnvelope(ENVELOPE_VOLUME);
				if ((channel->Flags & CHN_NOTEFADE) != 0)
				{
					uint16_t FadeOut = instr->GetFadeOut();
					if (FadeOut != 0)
					{
						if (channel->FadeOutVol < (FadeOut << 1))
							channel->FadeOutVol = 0;
						else
							channel->FadeOutVol -= FadeOut << 1;
						vol = (vol * channel->FadeOutVol) >> 16;
					}
					else if (channel->FadeOutVol == 0)
						vol = 0;
				}
				if (env->GetEnabled() && env->HasNodes())
				{
					uint8_t volValue = env->Apply(channel->EnvVolumePos);
					vol = muldiv(vol, volValue, 1 << 6);
					CLIPINT(vol, 0, 128);
					channel->EnvVolumePos++;
					if (env->GetLooped())
					{
						uint16_t endTick = env->GetLoopEnd();
						if (channel->EnvVolumePos == ++endTick)
						{
							channel->EnvVolumePos = env->GetLoopBegin();
							if (env->IsZeroLoop() && env->Apply(channel->EnvVolumePos) == 0)
							{
								channel->Flags |= CHN_NOTEFADE;
								channel->FadeOutVol = 0;
							}
						}
					}
					if (env->GetSustained() && (channel->Flags & CHN_NOTEOFF) == 0)
					{
						uint16_t endTick = env->GetSustainEnd();
						if (channel->EnvVolumePos == ++endTick)
							channel->EnvVolumePos = env->GetSustainBegin();
					}
					else if (env->IsAtEnd(channel->EnvVolumePos))
					{
						if (ModuleType == MODULE_IT || (channel->Flags & CHN_NOTEOFF) != 0)
							channel->Flags |= CHN_NOTEFADE;
						channel->EnvVolumePos = env->GetLastTick();
						if (env->Apply(channel->EnvVolumePos) == 0)
						{
							channel->Flags |= CHN_NOTEFADE;
							channel->FadeOutVol = 0;
							vol = 0;
						}
					}
				}
				// TODO: Process panning envelopes here.
			}
			else if ((channel->Flags & CHN_NOTEFADE) != 0)
			{
				channel->FadeOutVol = 0;
				vol = 0;
//				channel->Flags &= ~CHN_NOTEFADE;
			}

			vol = muldiv(vol * GlobalVolume, channel->ChannelVolume, 1 << 13);
			CLIPINT(vol, 0, 128);
			channel->Volume = vol;
			CLIPINT(channel->Period, MinPeriod, MaxPeriod);
			period = channel->Period;
			if ((channel->Flags & (CHN_GLISSANDO | CHN_PORTAMENTO)) == (CHN_GLISSANDO | CHN_PORTAMENTO))
				period = GetPeriodFromNote(/*GetNoteFromPeriod(period)*/channel->Note, channel->FineTune, channel->C4Speed);
			if ((channel->Flags & CHN_ARPEGGIO) != 0)
			{
				uint8_t n = TickCount % 3;
				if (n == 1)
					period = GetPeriodFromNote(channel->Note + (channel->Arpeggio >> 4), channel->FineTune, channel->C4Speed);
				else if (n == 2)
					period = GetPeriodFromNote(channel->Note + (channel->Arpeggio & 0x0F), channel->FineTune, channel->C4Speed);
			}
			if ((p_Header->Flags & FILE_FLAGS_AMIGA_LIMITS) != 0)
			{
				CLIPINT(period, 452, 3424);
			}
			if (channel->Instrument != NULL)
			{
				ModuleInstrument *instr = channel->Instrument;
				ModuleEnvelope *env = instr->GetEnvelope(ENVELOPE_PITCH);
				if (env->GetEnabled() && env->HasNodes())
				{
					int8_t pitchValue = env->Apply(channel->EnvPitchPos) - 32;
					if (pitchValue < 0)
					{
						uint8_t adjust = ((uint8_t)-pitchValue) << 3;
						period = LinearSlideUp(period, adjust);
					}
					else
					{
						uint8_t adjust = ((uint8_t)pitchValue) << 3;
						period = LinearSlideDown(period, adjust);
					}
					channel->EnvPitchPos++;
					if (env->GetLooped())
					{
						uint16_t endTick = env->GetLoopEnd();
						if (channel->EnvPitchPos == ++endTick)
							channel->EnvPitchPos = env->GetLoopBegin();
					}
				}
			}
			if ((channel->Flags & CHN_VIBRATO) != 0)
			{
				int8_t Delta;
				uint8_t VibratoPos = channel->VibratoPos;
				uint8_t VibratoType = channel->VibratoType & 0x03;
				if (VibratoType == 1)
					Delta = RampDownTable[VibratoPos];
				else if (VibratoType == 2)
					Delta = SquareTable[VibratoPos];
				else if (VibratoType == 3)
					Delta = RandomTable[VibratoPos];
				else
					Delta = SinusTable[VibratoPos];
				period += (short)(((int)Delta * channel->VibratoDepth) >> 7);
				channel->VibratoPos = (VibratoPos + channel->VibratoSpeed) & 0x3F;
			}
			if ((channel->Flags & CHN_PANBRELLO) != 0)
			{
				int8_t Delta;
				uint8_t PanPos = (((uint16_t)channel->PanbrelloPos + 16) >> 2) & 0x3F;
				uint8_t PanType = channel->PanbrelloType & 0x03;
				int16_t Pan = channel->Panning;
				if (PanType == 1)
					Delta = RampDownTable[PanPos];
				else if (PanType == 2)
					Delta = SquareTable[PanPos];
				else if (PanType == 3)
					Delta = RandomTable[PanPos];
				else
					Delta = SinusTable[PanPos];
				Pan += (Delta * (int)channel->PanbrelloDepth) >> 4;
				CLIPINT(Pan, 0, 128);
				channel->Panning = Pan;
				channel->PanbrelloPos += channel->PanbrelloSpeed;
			}
			if (channel->Sample != NULL && channel->Sample->GetVibratoDepth() != 0)
			{
				int8_t Delta;
				uint8_t VibratoPos;
				ModuleSample *sample = channel->Sample;
				uint8_t VibratoType = sample->GetVibratoType();
				//channel->AutoVibratoRate = sample->GetVibratoRate();
				VibratoPos = (channel->AutoVibratoPos + sample->GetVibratoSpeed()) & 0x03;
				if (VibratoType == 1)
					Delta = RampDownTable[VibratoPos];
				else if (VibratoType == 2)
					Delta = SquareTable[VibratoPos];
				else if (VibratoType == 3)
					Delta = RandomTable[VibratoPos];
				else
					Delta = SinusTable[VibratoPos];
				period += (short)(((int)Delta * sample->GetVibratoDepth()) >> 7);
				channel->AutoVibratoPos = VibratoPos;
			}
			if (period < (int)MinPeriod && ModuleType == MODULE_S3M)
				channel->Length = 0;
			CLIPINT(period, (int)MinPeriod, (int)MaxPeriod);
			// Calculate the increment from the frequency from the period
			freq = GetFreqFromPeriod(period, channel->C4Speed, 0);
			inc = muldiv(freq, 0x10000, MixSampleRate);
			channel->Increment.iValue = (inc + 1) & ~3;
		}
		if (channel->Volume != 0 || channel->LeftVol != 0 || channel->RightVol != 0)
			channel->Flags |= CHN_VOLUMERAMP;
		else
			channel->Flags &= ~CHN_VOLUMERAMP;
		channel->NewLeftVol = channel->NewRightVol = 0;
		//if (channel->Increment.Value.Hi + 1 >= (int)(channel->LoopEnd - channel->LoopStart))
		if ((channel->Increment.Value.Hi + 1) >= (int)channel->LoopEnd)
			channel->Flags &= ~CHN_LOOP;
		channel->SampleData = ((channel->NewSampleData != NULL && channel->Length != 0 && channel->Increment.iValue != 0) ? channel->NewSampleData : NULL);
		if (channel->SampleData != NULL)
		{
			if (MixChannels == 2)
			{
				channel->NewLeftVol = (channel->Volume * channel->Panning) >> 7;
				channel->NewRightVol = (channel->Volume * (128 - channel->Panning)) >> 7;
			}
			else
				channel->NewLeftVol = channel->NewRightVol = channel->Volume;
			channel->RightRamp = channel->LeftRamp = 0;
			// Do we need to ramp the volume up or down?
			if ((channel->Flags & CHN_VOLUMERAMP) != 0 && (channel->LeftVol != channel->NewLeftVol || channel->RightVol != channel->NewRightVol))
			{
				int LeftDelta, RightDelta;
				uint32_t RampLength = 1;
				// Calculate Volume deltas
				LeftDelta = channel->NewLeftVol - channel->LeftVol;
				RightDelta = channel->NewRightVol - channel->RightVol;
				// Check if we need to calculate the RampLength, and do so if need be
				if ((channel->LeftVol | channel->RightVol) != 0 && (channel->NewLeftVol | channel->NewRightVol) != 0 && (channel->Flags & CHN_FASTVOLRAMP) != 0)
				{
					RampLength = SamplesToMix;
					// Clipping:
					CLIPINT(RampLength, 2, 256);
				}
				// Calculate value to add to the volume to get it closer to the new volume during ramping
				channel->LeftRamp = LeftDelta / RampLength;
				channel->RightRamp = RightDelta / RampLength;
				// Normalise the current volume so that the ramping won't under or over shoot
				channel->LeftVol = channel->NewLeftVol - (channel->LeftRamp * RampLength);
				channel->RightVol = channel->NewRightVol - (channel->RightRamp * RampLength);
				// If the ramp values aren't 0 (ramping already done?)
				if ((channel->LeftRamp | channel->RightRamp) != 0)
					channel->RampLength = RampLength;
				else
				{
					// Otherwise the ramping is done, so don't need to make the mixer functions do it for us
					channel->Flags &= ~CHN_VOLUMERAMP;
					channel->LeftVol = channel->NewLeftVol;
					channel->RightVol = channel->NewRightVol;
				}
			}
			// No? ok, scratch the ramping.
			else
			{
				channel->Flags &= ~CHN_VOLUMERAMP;
				channel->LeftVol = channel->NewLeftVol;
				channel->RightVol = channel->NewRightVol;
			}
			// DEBUG: Uncomment to see the channel's main state information
			/*printf("%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %d, %u, %u, %u\n",
				channel->Flags, channel->LoopStart, channel->LoopEnd, channel->Length, channel->RawVolume, channel->RowNote, channel->RowSample,
				channel->RowEffect, channel->RowParam, channel->Period, channel->PortamentoDest, channel->FineTune, channel->Increment.Value.Hi,
				channel->Increment.Value.Lo, channel->Pos, channel->PosLo);*/
			MixerChannels[nMixerChannels++] = i;
		}
		else
			channel->LeftVol = channel->RightVol = channel->Length = 0;
	}

	return true;
}

uint32_t ModuleFile::GetSampleCount(Channel *channel, uint32_t Samples)
{
	uint32_t Pos, PosLo, SampleCount;
	uint32_t LoopStart = ((channel->Flags & CHN_LOOP) != 0 ? channel->LoopStart : 0);
	int16dot16 Increment = channel->Increment;
	if (Samples == 0 || Increment.iValue == 0 || channel->Length == 0)
		return 0;
	if (channel->Pos < LoopStart)
	{
		if (Increment.iValue < 0)
		{
			int Delta = ((LoopStart - channel->Pos) << 16) - (channel->PosLo & 0xFFFF);
			channel->Pos = LoopStart | (Delta >> 16);
			channel->PosLo = Delta & 0xFFFF;
			if (channel->Pos < LoopStart || channel->Pos >= (LoopStart + channel->Length) / 2)
			{
				channel->Pos = LoopStart;
				channel->PosLo = 0;
			}
			Increment.iValue = -Increment.iValue;
			channel->Increment.iValue = Increment.iValue;
			if ((channel->Flags & CHN_LOOP) == 0 || channel->Pos >= channel->Length)
			{
				channel->Pos = channel->Length;
				channel->PosLo = 0;
				return 0;
			}
		}
		else if (channel->Pos < 0)
			channel->Pos = 0;
	}
	else if (channel->Pos >= channel->Length)
	{
		if ((channel->Flags & CHN_LOOP) == 0)
			return 0;
		if ((channel->Flags & CHN_LPINGPONG) != 0)
		{
			if (Increment.iValue > 0)
			{
				Increment.iValue = -Increment.iValue;
				channel->Increment.iValue = Increment.iValue;
			}
			channel->Pos -= channel->Pos - channel->Length;
			if (channel->Pos <= channel->LoopStart || channel->Pos >= channel->Length)
				channel->Pos = channel->Length - 1;
		}
		else
		{
			if (Increment.iValue < 0)
			{
				Increment.iValue = -Increment.iValue;
				channel->Increment.iValue = Increment.iValue;
			}
			channel->Pos += LoopStart - channel->Length;
			if (channel->Pos < LoopStart)
				channel->Pos = LoopStart;
		}
		// The following fixes 3 or 4 bugs and allows
		// for loops to run correctly. DO NOT REMOVE!
		if (channel->Length > channel->LoopEnd)
			channel->Length = channel->LoopEnd;
	}
	Pos = channel->Pos;
	if (Pos < LoopStart)
	{
		if (Pos < 0 || Increment.iValue < 0)
			return 0;
	}
	if (Pos >= channel->Length)
		return 0;
	PosLo = channel->PosLo;
	SampleCount = Samples;
	if (Increment.iValue < 0)
	{
		int16dot16 Inv = Increment;
		uint32_t MaxSamples;
		uint32_t DeltaHi, DeltaLo, PosDest;
		Inv.iValue = -Inv.iValue;
		MaxSamples = 16384 / (Inv.Value.Hi + 1);
		if (MaxSamples < 2)
			MaxSamples = 2;
		if (Samples > MaxSamples)
			Samples = MaxSamples;
		DeltaHi = Inv.Value.Hi * Samples;
		DeltaLo = Inv.Value.Lo * Samples;
		PosDest = Pos - DeltaHi + ((PosLo - DeltaLo) >> 16);
		if (PosDest < LoopStart)
			SampleCount = ((((Pos - LoopStart) << 16) + PosLo - 1) / Inv.iValue);
	}
	else
	{
		uint32_t MaxSamples = 16384 / (Increment.Value.Hi + 1);
		uint32_t DeltaHi, DeltaLo, PosDest;
		if (MaxSamples < 2)
			MaxSamples = 2;
		if (Samples > MaxSamples)
			Samples = MaxSamples;
		DeltaHi = Increment.Value.Hi * Samples;
		DeltaLo = Increment.Value.Lo * Samples;
		PosDest = Pos + DeltaHi + ((PosLo + DeltaLo) >> 16);
		if (PosDest >= channel->Length)
//			SampleCount = channel->Length - channel->Pos;
			SampleCount = (((channel->Length - Pos) << 16) - PosLo) / Increment.iValue;
	}
	if (SampleCount <= 1)
		return 1;
	if (SampleCount > Samples)
		return Samples;
	return SampleCount;
}

inline void ModuleFile::FixDCOffset(int *p_DCOffsL, int *p_DCOffsR, int *buff, uint32_t samples)
{
	int DCOffsL = *p_DCOffsL;
	int DCOffsR = *p_DCOffsR;
	while (samples != 0 && (DCOffsR | DCOffsL) != 0)
	{
		int OffsL = -DCOffsL;
		int OffsR = -DCOffsR;

		OffsL >>= 31;
		OffsR >>= 31;
		OffsL &= 0xFF;
		OffsR &= 0xFF;
		OffsL += DCOffsL;
		OffsR += DCOffsR;
		OffsL >>= 8;
		OffsR >>= 8;
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
	uint32_t i, /*Flags, */rampSamples;
	if (count == 0)
		return;
	/*Flags = GetResamplingFlag();*/
	for (i = 0; i < nMixerChannels; i++)
	{
		uint32_t samples = count;
		int *buff = MixBuffer;
		Channel * const channel = &Channels[MixerChannels[i]];
		if (channel->SampleData == NULL)
			continue;
		do
		{
			rampSamples = samples;
			if (channel->RampLength > 0)
			{
				if (rampSamples > channel->RampLength)
					rampSamples = channel->RampLength;
			}
			SampleCount = GetSampleCount(channel, rampSamples);
			if (SampleCount <= 0)
			{
				FixDCOffset(&channel->DCOffsL, &channel->DCOffsR, buff, samples);
				DCOffsL += channel->DCOffsL;
				DCOffsR += channel->DCOffsR;
				channel->DCOffsL = channel->DCOffsR = 0;
				samples = 0;
				continue;
			}
			if (channel->RampLength == 0 && (channel->LeftVol | channel->RightVol) == 0)
				buff += SampleCount * 2;
			else
			{
				MixInterface MixFunc = MixFunctionTable[/*Flags*/MIX_NOSRC | (channel->RampLength != 0 ? MIX_RAMP : 0) |
					(channel->Sample->Get16Bit() == true ? MIX_16BIT : 0) | (channel->Sample->GetStereo() == true ? MIX_STEREO : 0)];
				int *BuffMax = buff + (SampleCount * 2);
				channel->DCOffsR = -((BuffMax - 2)[0]);
				channel->DCOffsL = -((BuffMax - 2)[1]);
				MixFunc(channel, buff, BuffMax);
				channel->DCOffsR += ((BuffMax - 2)[0]);
				channel->DCOffsL += ((BuffMax - 2)[1]);
				buff = BuffMax;
			}
			samples -= SampleCount;
			if (channel->RampLength != 0)
			{
				channel->RampLength -= SampleCount;
				if (channel->RampLength <= 0)
				{
					channel->RampLength = 0;
					channel->LeftVol = channel->NewLeftVol;
					channel->RightVol = channel->NewRightVol;
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
		MixBuffer[i] = MixBuffer[i << 1];
}

int32_t ModuleFile::Mix(uint8_t *Buffer, uint32_t BuffLen)
{
	uint32_t Count, SampleCount, Mixed = 0;
	uint32_t SampleSize = MixBitsPerSample / 8 * MixChannels, Max = BuffLen / SampleSize;

	if (Max == 0)
		return -2;
	if (NextPattern >= p_Header->nOrders)
		return (Mixed == 0 ? -2 : Mixed * (MixBitsPerSample / 8) * MixChannels);
	while (Mixed < Max)
	{
		if (SamplesToMix == 0)
		{
			// TODO: Deal with song fading
			if (AdvanceTick() == false)
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
	return (Mixed == 0 ? -2 : Mixed * (MixBitsPerSample / 8) * MixChannels);
}
