#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "../genericModule/genericModule.h"

#include "waveTables.h"
#include "moduleMixer.h"
#include "mixFunctions.h"
#include "mixFunctionTables.h"
#include "frequencyTables.h"

void ModuleFile::InitMixer(FileInfo *p_FI)
{
	MixSampleRate = p_FI->BitRate;
	MixChannels = p_FI->Channels;
	MixBitsPerSample = p_FI->BitsPerSample;
	MusicSpeed = p_Header->InitialSpeed;
	MusicTempo = p_Header->InitialTempo;
	TickCount = MusicSpeed;
	SamplesPerTick = (MixSampleRate * 640) / (MusicTempo << 8);
	Channels = new Channel[p_Header->nChannels];
	MixerChannels = new uint32_t[p_Header->nChannels];
	ResetChannelPanning();
	InitialiseTables();
}

void ModuleFile::ResetChannelPanning()
{
	uint8_t i;
	if (ModuleType == MODULE_MOD)
	{
		for (i = 0; i < p_Header->nChannels; i++)
			Channels[i].Panning = ((i % 2) == 0 ? 0 : 128);
	}
	else if (ModuleType == MODULE_S3M)
	{
		if (p_Header->Panning == NULL)
		{
			for (i = 0; i < p_Header->nChannels; i++)
				Channels[i].Panning = 32;
		}
		else
		{
			for (i = 0; i < p_Header->nChannels; i++)
				Channels[i].Panning = p_Header->Panning[i];
		}
	}
}

void ModuleFile::SampleChange(Channel *channel, uint32_t sample_)
{
	ModuleSample *sample;
	uint32_t note;

	sample = p_Samples[sample_ - 1];
	note = channel->NewNote;
	channel->Volume = sample->GetVolume();
	channel->NewSample = 0;
	channel->Sample = sample;
	channel->Length = sample->GetLength();
	channel->LoopStart = (sample->GetLoopStart() < sample->GetLength() ? sample->GetLoopStart() : sample->GetLength());
	channel->LoopEnd = (sample->GetLoopLen() > 2 ? channel->LoopStart + sample->GetLoopLen() : 0);
	if (channel->LoopEnd != 0)
		channel->Flags |= CHN_LOOP;
	else
		channel->Flags &= ~CHN_LOOP;
	channel->NewSampleData = p_PCM[sample->ID];
	channel->FineTune = sample->GetFineTune();
	channel->C4Speed = sample->GetC4Speed();
	if (channel->LoopEnd > channel->Length)
		channel->LoopEnd = channel->Length;
}

uint32_t ModuleFile::GetPeriodFromNote(uint8_t Note, uint8_t FineTune, uint32_t C4Speed)
{
	if (Note == 0 || Note > 0xF0)
		return 0;
	Note--;
	if (ModuleType == MODULE_S3M)
	{
		if ((p_Header->Flags & 4) == 0)
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
		if ((p_Header->Flags & 4) == 0)
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

	channel->Note = note;
	channel->NewSample = 0;
	period = GetPeriodFromNote(note, channel->FineTune, channel->C4Speed);
	if (sample == NULL)
		return;
	if (period != 0)
	{
		if (cmd != CMD_TONEPORTAMENTO && cmd != CMD_TONEPORTAVOL)
			channel->Period = period;
		channel->PortamentoDest = period;
		if (channel->PortamentoDest == channel->Period || channel->Length == 0)
		{
			channel->Sample = sample;
			channel->NewSampleData = p_PCM[sample->ID];
			channel->Length = sample->GetLength();
			if (sample->GetLoopLen() > 2)
			{
				channel->Flags |= CHN_LOOP;
				channel->LoopStart = (sample->GetLoopStart() < sample->GetLength() ? sample->GetLoopStart() : sample->GetLength());
				channel->LoopEnd = channel->LoopStart + sample->GetLoopLen();
			}
			else
			{
				channel->Flags &= ~CHN_LOOP;
				channel->LoopStart = 0;
				channel->LoopEnd = sample->GetLength();
			}
			channel->Pos = 0;
			if ((channel->TremoloType & 0x04) != 0)
				channel->TremoloPos = 0;
		}
		if (channel->Pos > channel->Length)
			channel->Pos = channel->Length;
		channel->PosLo = 0;
	}
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
			if (TickCount == 0)
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
			if (TickCount == 0)
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
			if (TickCount != 0)
				break;
			channel->FineTune = param;
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
			if (param != 0 && TickCount == 0)
				channel->Volume += param << 1;
			break;
		case CMD_MODEX_FINEVOLDOWN:
			if (param != 0 && TickCount == 0)
				channel->Volume -= param << 1;
			break;
		case CMD_MODEX_CUT:
			if (TickCount == param)
			{
				channel->Volume = 0;
				channel->Flags |= CHN_FASTVOLRAMP;
			}
			break;
		case CMD_MODEX_INVERTLOOP:
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
			if (TickCount != 0)
				break;
			channel->FineTune = S3MSpeedTable[param];
			channel->Period = GetPeriodFromNote(channel->Note, channel->FineTune, channel->C4Speed);
			break;
		case CMD_S3MEX_VIBRATOWAVE:
			channel->VibratoType = param & 0x07;
			break;
		case CMD_S3MEX_TREMOLOWAVE:
			channel->TremoloType = param & 0x07;
			break;
		case CMD_S3MEX_PANWAVE:
			break;
		case CMD_S3MEX_FRAMEDELAY:
			FrameDelay = param;
			break;
		case CMD_S3MEX_ENVELOPE:
			if (TickCount != 0)
				break;
			break;
		case CMD_S3MEX_PANNING:
			if (TickCount != 0)
				break;
			channel->Panning = param << 3;
			channel->Flags |= CHN_FASTVOLRAMP;
			break;
		case CMD_S3MEX_SURROUND:
			break;
		case CMD_S3MEX_OFFSET:
			if (TickCount != 0)
				break;
			if (channel->RowNote != 0 && channel->RowNote < 0x80)
			{
				int pos = param << 16;
				if (pos < channel->Length)
					channel->Pos = pos;
			}
			break;
		case CMD_S3MEX_NOTECUT:
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
	// TODO: Recode to take into account S3M FineVolume slides
	if (TickCount != 0)
	{
		short NewVolume = channel->Volume;
		if ((param & 0xF0) != 0)
			NewVolume += (param & 0xF0) >> 8;
		else
			NewVolume -= param & 0x0F;
		channel->Flags |= CHN_FASTVOLRAMP;
		if (NewVolume < 0)
			NewVolume = 0;
		else if (NewVolume > 64)
			NewVolume = 64;
		channel->Volume = NewVolume & 0xFF;
	}
}

inline void ModuleFile::PortamentoUp(Channel *channel, uint8_t param)
{
	if (param != 0)
		channel->Portamento = param;
	else
		param = channel->Portamento;
	if (ModuleType == MODULE_S3M && (param & 0xF0) >= 0xE0)
	{
		if ((param & 0x0F) != 0)
		{
			if ((param & 0xF0) == 0xF0)
				FinePortamentoUp(channel, param & 0x0F);
			else if ((param & 0xF0) == 0xE0)
				ExtraFinePortamentoUp(channel, param & 0x0F);
		}
		return;
	}
	if (TickCount != 0 || MusicSpeed == 1)
	{
		if (ModuleType == MODULE_S3M && (p_Header->Flags & 4) == 0)
		{
			uint32_t OldPeriod = channel->Period;
			channel->Period = ((OldPeriod * LinearSlideDownTable[param]) + 32768) / 65536;
			if (channel->Period == OldPeriod)
				channel->Period--;
		}
		else
		{
			channel->Period -= param << 2;
			if (channel->Period < MinPeriod)
				channel->Period = MinPeriod;
		}
	}
}

inline void ModuleFile::PortamentoDown(Channel *channel, uint8_t param)
{
	if (param != 0)
		channel->Portamento = param;
	else
		param = channel->Portamento;
	if (ModuleType == MODULE_S3M && (param & 0xF0) >= 0xE0)
	{
		if ((param & 0x0F) != 0)
		{
			if ((param & 0xF0) == 0xF0)
				FinePortamentoDown(channel, param & 0x0F);
			else if ((param & 0xF0) == 0xE0)
				ExtraFinePortamentoDown(channel, param & 0x0F);
		}
		return;
	}
	if (TickCount != 0 || MusicSpeed == 1)
	{
		if (ModuleType == MODULE_S3M && (p_Header->Flags & 4) == 0)
		{
			uint32_t OldPeriod = channel->Period;
			channel->Period = ((OldPeriod * LinearSlideUpTable[param]) + 32768) / 65536;
			if (channel->Period == OldPeriod)
				channel->Period++;
		}
		else
		{
			channel->Period += param << 2;
			if (channel->Period > MaxPeriod)
				channel->Period = MaxPeriod;
		}
	}
}

inline void ModuleFile::FinePortamentoUp(Channel *channel, uint8_t param)
{
	if (TickCount == 0)
	{
		if  ((p_Header->Flags & 4) == 0)
			channel->Period = ((channel->Period * LinearSlideDownTable[param]) + 32768) / 65536;
		else
			channel->Period -= param << 2;
		if (channel->Period < MinPeriod)
			channel->Period = MinPeriod;
	}
}

inline void ModuleFile::FinePortamentoDown(Channel *channel, uint8_t param)
{
	if (TickCount == 0)
	{
		if  ((p_Header->Flags & 4) == 0)
			channel->Period = ((channel->Period * LinearSlideUpTable[param]) + 32768) / 65536;
		else
			channel->Period += param << 2;
		if (channel->Period > MaxPeriod)
			channel->Period = MaxPeriod;
	}
}

inline void ModuleFile::ExtraFinePortamentoUp(Channel *channel, uint8_t param)
{
	if (TickCount == 0)
	{
		if  ((p_Header->Flags & 4) == 0)
		{
		}
		else
			channel->Period -= param;
		if (channel->Period < MinPeriod)
			channel->Period = MinPeriod;
	}
}

inline void ModuleFile::ExtraFinePortamentoDown(Channel *channel, uint8_t param)
{
	if (TickCount == 0)
	{
		if  ((p_Header->Flags & 4) == 0)
		{
		}
		else
			channel->Period += param;
		if (channel->Period > MaxPeriod)
			channel->Period = MaxPeriod;
	}
}

inline void ModuleFile::TonePortamento(Channel *channel, uint8_t param)
{
	if (param != 0)
		channel->PortamentoSlide = param << 2;
	channel->Flags |= CHN_PORTAMENTO;
	if (channel->Period != 0 && channel->PortamentoDest != 0 && (MusicSpeed == 1 || TickCount != 0))
	{
		if (channel->Period < channel->PortamentoDest)
		{
			int Delta;
			if ((channel->Flags & CHN_GLISSANDO) != 0)
			{
				uint8_t Slide = (uint8_t)(channel->PortamentoSlide >> 2);
				// TODO: Replace LinearSlideUpTable lookup with
				// (int)(65536 * 2^(Slide/192)) which is the formula to create the table
				Delta = (((channel->Period * LinearSlideUpTable[Slide]) + 32768) / 65536) - channel->Period;
				//Delta = muladddiv(channel->Period, LinearSlideUpTable[Slide], 65536) - channel->Period;
				if (Delta < 1)
					Delta = 1;
			}
			else
				Delta = channel->PortamentoSlide;
			channel->Period += Delta;
			if (channel->Period > channel->PortamentoDest)
				channel->Period = channel->PortamentoDest;
		}
		else if (channel->Period > channel->PortamentoDest)
		{
			int Delta;
			if ((channel->Flags & CHN_GLISSANDO) != 0)
			{
				uint8_t Slide = (uint8_t)(channel->PortamentoSlide >> 2);
				Delta = (((channel->Period * LinearSlideDownTable[Slide]) + 32768) / 65536) - channel->Period;
				//Delta = muladddiv(channel->Period, LinearSlideDownTable[Slide], 65536) - channel->Period;
				if (Delta > -1)
					Delta = -1;
			}
			else
				Delta = -channel->PortamentoSlide;
			channel->Period += Delta;
			if (channel->Period < channel->PortamentoDest)
				channel->Period = channel->PortamentoDest;
		}
	}
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
		uint32_t StartTick = 0;

		channel->Flags &= ~CHN_FASTVOLRAMP;
		if (cmd == CMD_MOD_EXTENDED || cmd == CMD_S3M_EXTENDED)
		{
			uint8_t excmd = (param & 0xF0) >> 4;
			if (excmd == CMD_MODEX_DELAYSAMP)
				StartTick = param & 0x0F;
			else if (TickCount == 0)
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

		if (TickCount == StartTick)
		{
			uint8_t note = channel->RowNote;
			if (sample != 0)
				channel->NewSample = sample;
			if (ModuleType == MODULE_MOD && note == 0xFF)
			{
				channel->Flags |= CHN_FASTVOLRAMP;
				channel->Volume = 0;
				note = sample = 0;
			}
			if (note == 0 && sample != 0)
				channel->Volume = p_Samples[sample - 1]->GetVolume();
			if (note >= 0xFE)
				sample = 0;
			if (note != 0 && note <= 128)
				channel->NewNote = note;
			if (sample != 0)
			{
				SampleChange(channel, sample);
				channel->NewSample = 0;
			}
			if (note != 0)
			{
				if (sample == 0 && channel->NewSample != 0 && note < 0x80)
				{
					SampleChange(channel, channel->NewSample);
					channel->NewSample = 0;
				}
				NoteChange(channel, note, cmd);
			}
			if (channel->RowVolEffect == VOLCMD_VOLUME)
			{
				channel->Volume = channel->RowVolParam << 1;
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
				if (TickCount != 0 || channel->Period == 0 || channel->Note == 0xFF)
					break;
				channel->Flags |= CHN_ARPEGGIO;
				channel->Arpeggio = param;
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
				if ((param & 0x0F) != 0)
					channel->VibratoDepth = (param & 0x0F) * 4;
				if ((param & 0xF0) != 0)
					channel->VibratoSpeed = param >> 4;
				channel->Flags |= CHN_VIBRATO;
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
				if (TickCount != 0)
					break;
				channel->Pos = ((uint32_t)param) << 9;
				channel->PosLo = 0;
				if (channel->Pos > channel->Length)
					channel->Pos = channel->Length;
				break;
			case CMD_VOLUMESLIDE:
				if (param != 0)
					VolumeSlide(channel, param);
				break;
			case CMD_POSITIONJUMP:
				PositionJump = param;
				if (PositionJump > p_Header->nOrders)
					PositionJump = 0;
				break;
			case CMD_PATTERNBREAK:
				BreakRow = param;
				BreakRow = ((BreakRow >> 8) * 10) + (BreakRow & 0x0F);
				if (BreakRow > 63)
					BreakRow = 63;
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
				if (TickCount == 0)
				{
					uint8_t NewVolume = param;
					if (NewVolume > 64)
						NewVolume = 64;
					channel->Volume = NewVolume << 1;
					channel->Flags |= CHN_FASTVOLRAMP;
				}
				break;
			case CMD_PANNING:
				if (TickCount != 0)
					break;
				channel->Panning = param;
				channel->Flags |= CHN_FASTVOLRAMP;
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
			BOOL Jump = TRUE;
			if (PositionJump < 0)
				PositionJump = NewPattern + 1;
			if (BreakRow < 0)
				BreakRow = 0;
			if ((uint32_t)PositionJump < NewPattern || ((uint32_t)PositionJump == NewPattern && (uint32_t)BreakRow <= Row))
				Jump = FALSE;
			if (Jump == TRUE && (PositionJump != NewPattern || BreakRow != Row))
			{
				if (PositionJump != NewPattern)
					PatternLoopCount = PatternLoopStart = 0;
				NextPattern = PositionJump;
				NextRow = BreakRow;
			}
		}
	}
	return true;
}

void ModuleFile::SetChannelRowData(uint32_t i, ModuleCommand *Command)
{
	Channel *channel = &Channels[i];
	channel->RowNote = Command->Note;
	if (channel->RowNote == 0)
		channel->RowNote = -1;
	channel->RowSample = Command->Sample;
	if (channel->RowSample > p_Header->nSamples)
		channel->RowSample = 0;
	channel->RowVolEffect = Command->VolEffect;
	channel->RowVolParam = Command->VolParam;
	channel->RowEffect = Command->Effect;
	channel->RowParam = Command->Param;
	channel->Flags &= ~(CHN_TREMOLO | CHN_ARPEGGIO | CHN_VIBRATO | CHN_PORTAMENTO | CHN_GLISSANDO);
}

bool ModuleFile::ProcessRow()
{
	TickCount++;
	if (TickCount >= (MusicSpeed * (PatternDelay + 1)) + FrameDelay)
	{
		uint32_t i;
		ModuleCommand (*Commands)[64];
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
		if (Row >= 64)
			Row = 0;
		NextRow = Row + 1;
		if (NextRow >= 64)
		{
			NextPattern = NewPattern + 1;
			NextRow = 0;
		}
		Commands = (ModuleCommand (*)[64])p_Patterns[Pattern]->GetCommands();
		for (i = 0; i < p_Header->nChannels; i++)
			SetChannelRowData(i, &Commands[i][Row]);
	}
	if (MusicSpeed == 0)
		MusicSpeed = 1;
	return ProcessEffects();
}

bool ModuleFile::AdvanceRow()
{
	uint32_t i;

	if (ProcessRow() == false)
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
			short vol = channel->Volume;
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
						vol += (RandomTable[TremoloPos] * channel->TremoloDepth) >> 8;
					else
						vol += (SinusTable[TremoloPos] * TremoloDepth) >> 8;
				}
				if (TickCount != 0)
					channel->TremoloPos = (TremoloPos + channel->TremoloSpeed) & 0x3F;
			}
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
			if ((p_Header->Flags & 16) != 0)
			{
				CLIPINT(period, 452, 3424);
			}
			if ((channel->Flags & CHN_VIBRATO) != 0)
			{
				char Delta;
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
				period += (short)(((int)Delta * (int)channel->VibratoDepth) >> 7);
				channel->VibratoPos = (VibratoPos + channel->VibratoSpeed) & 0x3F;
			}
			if (period < MinPeriod && ModuleType == MODULE_S3M)
				channel->Length = 0;
			CLIPINT(period, MinPeriod, MaxPeriod);
			freq = GetFreqFromPeriod(period, channel->C4Speed, 0);
			inc = muldiv(freq, 0x10000, MixSampleRate);
			channel->Increment.iValue = (inc + 1) & ~3;
		}
		if (channel->Volume != 0 || channel->LeftVol != 0 || channel->RightVol != 0)
			channel->Flags |= CHN_VOLUMERAMP;
		else
			channel->Flags &= ~CHN_VOLUMERAMP;
		channel->NewLeftVol = channel->NewRightVol = 0;
		if (channel->Increment.Value.Hi + 1 >= (int)(channel->LoopEnd - channel->LoopStart))
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
					CLIPINT(RampLength, 4, 512);
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
		if (Increment.iValue < 0)
		{
			Increment.iValue = -Increment.iValue;
			channel->Increment.iValue = Increment.iValue;
		}
		channel->Pos += LoopStart - channel->Length;
		if (channel->Pos < LoopStart)
			channel->Pos = LoopStart;
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
#ifdef _WINDOWS
		__asm
		{
			sar OffsL, 31
			sar OffsR, 31
		}
#else
		asm(".intel_syntax noprefix\n"
			"\tsar %%eax, 31\n"
			"\tsar %%ebx, 31\n"
			".att_syntax\n" : [OffsL] "=a" (OffsL), [OffsR] "=b" (OffsR) :
				"a" (OffsL), "b" (OffsR) : );
#endif
		OffsL &= 0xFF;
		OffsR &= 0xFF;
		OffsL += DCOffsL;
		OffsR += DCOffsR;
#ifdef _WINDOWS
		__asm
		{
			sar OffsL, 8
			sar OffsR, 8
		}
#else
		asm(".intel_syntax noprefix\n"
			"\tsar %%eax, 8\n"
			"\tsar %%ebx, 8\n"
			".att_syntax\n" : [OffsL] "=a" (OffsL), [OffsR] "=b" (OffsR) :
				"a" (OffsL), "b" (OffsR) : );
#endif
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
				MixInterface MixFunc = MixFunctionTable[/*Flags*/MIX_NOSRC | (channel->RampLength != 0 ? MIX_RAMP : 0)];
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

long ModuleFile::Mix(uint8_t *Buffer, uint32_t BuffLen)
{
	uint32_t Count, SampleCount, Mixed = 0;
	uint32_t SampleSize = MixBitsPerSample / 8 * MixChannels, Max = BuffLen / SampleSize;

	if (Max == 0)
		return -2;
	if (Pattern >= p_Header->nOrders)
		return (Mixed == 0 ? -2 : Mixed * (MixBitsPerSample / 8) * MixChannels);
	while (Mixed < Max)
	{
		if (SamplesToMix == 0)
		{
			// TODO: Deal with song fading
			if (AdvanceRow() == false)
			{
				// Song fading
			}
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
			// MonoFromStereo?
		}
		Buffer += Convert32to16(Buffer, MixBuffer, SampleCount);
		Mixed += Count;
		SamplesToMix -= Count;
	}
	return (Mixed == 0 ? -2 : Mixed * (MixBitsPerSample / 8) * MixChannels);
}
