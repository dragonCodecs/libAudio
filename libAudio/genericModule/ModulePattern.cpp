#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"
#include <stdlib.h>

static const uint16_t Periods[60] =
{
	1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017, 961, 907,
	856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
	428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
	107, 101, 95, 90, 85, 80, 76, 71, 67, 64, 60, 57
};

ModulePattern::ModulePattern(MOD_Intern *p_MF, uint32_t nChannels) : Channels(nChannels), Rows(64)
{
	uint32_t channel, row;
	Commands = new ModuleCommand *[nChannels];
	for (row = 0; row < 64; row++)
	{
		for (channel = 0; channel < nChannels; channel++)
		{
			// Read 4 bytes of data and unpack it into the structure.
			uint8_t Data[4];
			if (row == 0)
				Commands[channel] = new ModuleCommand[64];
			fread(Data, 4, 1, p_MF->f_Module);
			Commands[channel][row].SetMODData(Data);
		}
	}
}

#define checkLength(cnt, Length) \
	if (cnt + 1 >= Length) \
		break

ModulePattern::ModulePattern(S3M_Intern *p_SF, uint32_t nChannels) : Channels(nChannels), Rows(64)
{
	uint32_t j, Length;
	uint8_t row;
	FILE *f_S3M = p_SF->f_Module;
	Commands = new ModuleCommand *[nChannels];
	for (j = 0; j < nChannels; j++)
		Commands[j] = new ModuleCommand[64];
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

ModulePattern::ModulePattern(STM_Intern *p_SF) : Channels(4), Rows(64)
{
	uint8_t row, channel;
	FILE *f_STM = p_SF->f_Module;

	Commands = new ModuleCommand *[4];
	for (row = 0; row < 64; row++)
	{
		for (channel = 0; channel < 4; channel++)
		{
			uint8_t Note;
			uint8_t Param;
			uint8_t Volume;
			uint8_t Effect;

			if (row == 0)
				Commands[channel] = new ModuleCommand[64];
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

ModulePattern::ModulePattern(AON_Intern *p_AF, uint32_t nChannels) : Channels(nChannels), Rows(64)
{
	uint8_t row, channel;
	FILE *f_AON = p_AF->f_Module;

	Commands = new ModuleCommand *[nChannels];
	for (row = 0; row < 64; row++)
	{
		for (channel = 0; channel < nChannels; channel++)
		{
			uint8_t Note, Samp;
			uint8_t Effect, Param;
			uint8_t ArpIndex = 0;

			if (row == 0)
				Commands[channel] = new ModuleCommand[64];
			fread(&Note, 1, 1, f_AON);
			Commands[channel][row].SetAONNote(Note);
			fread(&Samp, 1, 1, f_AON);
			ArpIndex |= (Samp >> 6) & 0x03;
			Commands[channel][row].SetSample(Samp & 0x3F);
			fread(&Effect, 1, 1, f_AON);
			ArpIndex |= (Effect >> 4) & 0x0C;
			Commands[channel][row].SetAONArpIndex(ArpIndex);
			fread(&Param, 1, 1, f_AON);
			Commands[channel][row].SetAONEffect(Effect & 0x3F, Param);
		}
	}
}

inline bool readInc(uint8_t &var, uint16_t &i, const uint16_t len, const fd_t &fd)
{
	if (i > len || !fd.read(var))
		return true;
	++i;
	return false;
}

ModulePattern::ModulePattern(const modIT_t &file, uint32_t nChannels) : Channels(nChannels)
{
	std::array<char, 4> DontCare;
	uint8_t b, ChannelMask[64];
	uint16_t len, row, channel, j;
	ModuleCommand LastCommand[64];
	const fd_t &fd = file.fd();

	fd.read(len);
	fd.read(&Rows, 2);
	Commands = new ModuleCommand *[nChannels];
	for (channel = 0; channel < nChannels; channel++)
		Commands[channel] = new ModuleCommand[Rows];
	fd.read(DontCare);

	row = 0;
	j = 0;
	while (row < Rows)
	{
		channel = 0;
		if (readInc(b, j, len, fd))
			break;
		if (b == 0)
		{
			row++;
			continue;
		}
		channel = b & 0x7F;
		if (channel != 0)
			channel = (channel - 1) & 0x3F;
		if ((b & 0x80) != 0 && readInc(ChannelMask[channel], j, len, fd))
			break;
		if (channel < nChannels)
			Commands[channel][row].SetITRepVal(ChannelMask[channel], LastCommand[channel]);
		if ((ChannelMask[channel] & 0x01) != 0)
		{
			uint8_t note;
			if (readInc(note, j, len, fd))
				break;
			if (channel < nChannels)
			{
				Commands[channel][row].SetITNote(note);
				LastCommand[channel].SetITNote(note);
			}
		}
		if ((ChannelMask[channel] & 0x02) != 0)
		{
			uint8_t sample;
			if (readInc(sample, j, len, fd))
				break;
			if (channel < nChannels)
			{
				Commands[channel][row].SetSample(sample);
				LastCommand[channel].SetSample(sample);
			}
		}
		if ((ChannelMask[channel] & 0x04) != 0)
		{
			uint8_t volume;
			if (readInc(volume, j, len, fd))
				break;
			if (channel < nChannels)
			{
				Commands[channel][row].SetITVolume(volume);
				LastCommand[channel].SetITVolume(volume);
			}
		}
		if ((ChannelMask[channel] & 0x08) != 0)
		{
			uint8_t effect, param;
			if (readInc(effect, j, len, fd) || readInc(param, j, len, fd))
				break;
			if (channel < nChannels)
			{
				Commands[channel][row].SetITEffect(effect, param);
				LastCommand[channel].SetITEffect(effect, param);
			}
		}
	}
}

ModulePattern::~ModulePattern()
{
	uint8_t channel;
	for (channel = 0; channel < Channels; channel++)
		delete [] Commands[channel];
	delete [] Commands;
}

ModuleCommand **ModulePattern::GetCommands() const
{
	return Commands;
}

uint16_t ModulePattern::GetRows() const
{
	return Rows;
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

void ModuleCommand::SetAONNote(uint8_t note)
{
	Note = note;
}

void ModuleCommand::SetAONArpIndex(uint8_t Index)
{
	ArpIndex = Index;
}

void ModuleCommand::SetITRepVal(uint8_t ChannelMask, ModuleCommand &LastCommand)
{
	if ((ChannelMask & 0x10) != 0)
		Note = LastCommand.Note;
	if ((ChannelMask & 0x20) != 0)
		Sample = LastCommand.Sample;
	if ((ChannelMask & 0x40) != 0)
	{
		VolEffect = LastCommand.VolEffect;
		VolParam = LastCommand.VolParam;
	}
	if ((ChannelMask & 0x80) != 0)
	{
		Effect = LastCommand.Effect;
		Param = LastCommand.Param;
	}
}

void ModuleCommand::SetITNote(uint8_t note)
{
	if (note < 0x80)
		note++;
	Note = note;
}

void ModuleCommand::SetITVolume(uint8_t volume)
{
	if (volume <= 64)
	{
		VolEffect = VOLCMD_VOLUME;
		VolParam = volume;
	}
	else if (volume <= 74)
	{
		VolEffect = VOLCMD_FINEVOLUP;
		VolParam = volume - 65;
	}
	else if (volume <= 84)
	{
		VolEffect = VOLCMD_FINEVOLDOWN;
		VolParam = volume - 75;
	}
	else if (volume <= 94)
	{
		VolEffect = VOLCMD_VOLSLIDEUP;
		VolParam = volume - 85;
	}
	else if (volume <= 104)
	{
		VolEffect = VOLCMD_VOLSLIDEDOWN;
		VolParam = volume - 95;
	}
	else if (volume <= 114)
	{
		VolEffect = VOLCMD_PORTADOWN;
		VolParam = volume - 105;
	}
	else if (volume <= 124)
	{
		VolEffect = VOLCMD_PORTAUP;
		VolParam = volume - 115;
	}
	else if (volume >= 128 && volume <= 192)
	{
		VolEffect = VOLCMD_PANNING;
		VolParam = volume - 128;
	}
	else if (volume >= 193 && volume <= 202)
	{
		VolEffect = VOLCMD_PORTAMENTO;
		VolParam = volume - 193;
	}
	else if (volume >= 203 && volume <= 212)
	{
		VolEffect = VOLCMD_VIBRATO;
		VolParam = volume - 203;
	}
	else
	{
		VolEffect = VOLCMD_NONE;
		VolParam = volume;
	}
}
