#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"
#include <stdlib.h>

uint16_t Periods[60] =
{
	1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017, 961, 907,
	856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
	428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
	107, 101, 95, 90, 85, 80, 76, 71, 67, 64, 60, 57
};

ModulePattern::ModulePattern(MOD_Intern *p_MF, uint32_t nChannels)
{
	uint32_t channel, row;
	Commands = new ModuleCommand[nChannels][64];
	for (row = 0; row < 64; row++)
	{
		for (channel = 0; channel < nChannels; channel++)
		{
			// Read 4 bytes of data and unpack it into the structure.
			uint8_t Data[4];
			fread(Data, 4, 1, p_MF->f_MOD);
			Commands[channel][row].SetMODData(Data);
		}
	}
}

#define checkLength(cnt, Length) \
	if (cnt + 1 >= Length) \
		break

ModulePattern::ModulePattern(S3M_Intern *p_SF, uint32_t nChannels)
{
	uint32_t j, Length;
	uint8_t row;
	FILE *f_S3M = p_SF->f_S3M;
	Commands = new ModuleCommand[nChannels][64];
	fread(&Length, sizeof(uint16_t), 1, f_S3M);
	for (j = 0, row = 0; row < 64;)
	{
		/* Begin: temp */
		uint8_t Note;
		uint8_t Sample;
		uint8_t Volume;
		uint8_t Effect;
		uint8_t Param;
		/* End: temp */

		uint8_t byte, channel;
		checkLength(j, Length);
		fread(&byte, 1, 1, f_S3M);
		if (byte == 0)
		{
			row++;
			continue;
		}
		channel = (byte & 0x1F);
		if ((byte & 0x20) != 0)
		{
			fread(&Note, 1, 1, f_S3M);
			fread(&Sample, 1, 1, f_S3M);
			if (channel < nChannels)
				Commands[channel][row].SetS3MNote(Note, Sample);
			j += 2;
			checkLength(j, Length);
		}
		if ((byte & 0x40) != 0)
		{
			fread(&Volume, 1, 1, f_S3M);
			if (channel < nChannels)
				Commands[channel][row].SetS3MVolume(Volume);
			j++;
			checkLength(j, Length);
		}
		if ((byte & 0x80) != 0)
		{
			fread(&Effect, 1, 1, f_S3M);
			fread(&Param, 1, 1, f_S3M);
			if (channel < nChannels)
				Commands[channel][row].SetS3MEffect(Effect, Param);
			j += 2;
			checkLength(j, Length);
		}
	}
}

#undef checkLength

ModulePattern::ModulePattern(STM_Intern *p_SF)
{
	uint8_t row, channel;
	FILE *f_STM = p_SF->f_STM;

	Commands = new ModuleCommand[4][64];
	for (row = 0; row < 64; row++)
	{
		for (channel = 0; channel < 4; channel++)
		{
			uint8_t Note;
			uint8_t Param;
			uint8_t Volume;
			uint8_t Effect;

			fread(&Note, 1, 1, f_STM);
			Commands[channel][row].SetSTMNote(Note);
			fread(&Param, 1, 1, f_STM);
			Volume = Param & 0x07;
			Commands[channel][row].SetSample(Param >> 3);
			fread(&Param, 1, 1, f_STM);
			Volume += Param >> 1;
			Commands[channel][row].SetVolume(Volume);
			Effect = Param & 0x0F;
			fread(&Param, 1, 1, f_STM);
			Commands[channel][row].SetSTMEffect(Effect, Param);
		}
	}
}

ModulePattern::~ModulePattern()
{
	delete [] Commands;
}

ModuleCommand **ModulePattern::GetCommands()
{
	return (ModuleCommand **)Commands;
}

void ModuleCommand::SetSample(uint8_t sample)
{
	Sample = sample;
}

void ModuleCommand::SetVolume(uint8_t volume)
{
	VolEffect = VOLCMD_VOLUME;
	VolParam = volume;
}

uint8_t ModuleCommand::MODPeriodToNoteIndex(uint16_t Period)
{
	uint8_t i, min = 0, max = 59;
	if (Period == 0)
		return 0;
	do
	{
		i = min + ((max - min) / 2);
		if (Periods[i] == Period)
			return 37 + i;
		else if (Periods[i] < Period)
		{
			if (i > 0)
			{
				uint32_t Dist1 = Period - Periods[i];
				uint32_t Dist2 = abs(Periods[i - 1] - Period);
				if (Dist1 < Dist2)
					return 37 + i;
			}
			max = i - 1;
		}
		else if (Periods[i] > Period)
		{
			if (i < 59)
			{
				uint32_t Dist1 = Periods[i] - Period;
				uint32_t Dist2 = abs(Period - Periods[i + 1]);
				if (Dist1 < Dist2)
					return 37 + i;
			}
			min = i + 1;
		}
	}
	while (min < max);
	if (min == max)
		return 37 + min;
	else
		return 0;
}

void ModuleCommand::SetMODData(uint8_t Data[4])
{
	Sample = (Data[0] & 0xF0) | (Data[2] >> 4);
	Note = MODPeriodToNoteIndex((((uint16_t)(Data[0] & 0x0F)) << 8) | Data[1]);
	TranslateMODEffect(Data[2] & 0x0F, Data[3]);
}

void ModuleCommand::SetS3MNote(uint8_t note, uint8_t sample)
{
	Sample = sample;
	if (note < 0xF0)
		Note = (note & 0x0F) + (12 * (note >> 4)) + 13;
	else if (note == 0xFF)
		Note = 0;
	else
		Note = note;
}

void ModuleCommand::SetS3MVolume(uint8_t volume)
{
	if (volume >= 128 && volume <= 192)
	{
		VolEffect = VOLCMD_PANNING;
		VolParam = volume - 128;
	}
	else
	{
		VolEffect = VOLCMD_VOLUME;
		VolParam = (volume > 64 ? 64 : volume);
	}
}

void ModuleCommand::SetSTMNote(uint8_t note)
{
	if (note > 250)
	{
		if (note == 254 || note == 252)
			Note = 254;
	}
	else
	{
		uint8_t Octave = (note & 0xF0) >> 4;
		uint8_t Pitch = note & 0x0F;
		Pitch %= 12;
		Note = (Octave * 12) + Pitch + 37;
	}
}
