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

ModulePattern::ModulePattern(const uint32_t _channels, const uint16_t rows, const uint32_t type) :
	Channels(_channels), _commands(_channels), _rows(rows)
{
	if (!_commands.valid())
		throw ModuleLoaderError(type);
}

ModulePattern::ModulePattern(const modMOD_t &file, const uint32_t nChannels) : ModulePattern(nChannels, 64, E_BAD_MOD)
{
	const fd_t &fd = file.fd();
	for (uint32_t row = 0; row < _rows; row++)
	{
		for (uint32_t channel = 0; channel < nChannels; channel++)
		{
			// Read 4 bytes of data and unpack it into the structure.
			std::array<uint8_t, 4> Data;
			if (row == 0)
			{
				_commands[channel] = makeUnique<ModuleCommand []>(_rows);
				if (!_commands[channel])
					throw ModuleLoaderError(E_BAD_MOD);
			}
			fd.read(Data);
			_commands[channel][row].SetMODData(Data.data());
		}
	}
}

#define checkLength(cnt, length) \
	if (cnt + 1 >= length) \
		break

ModulePattern::ModulePattern(const modS3M_t &file, const uint32_t nChannels) : ModulePattern(nChannels, 64, E_BAD_S3M)
{
	uint32_t length;
	const fd_t &fd = file.fd();

	for (uint32_t i = 0; i < nChannels; ++i)
	{
		_commands[i] = makeUnique<ModuleCommand []>(_rows);
		if (!_commands[i])
			throw ModuleLoaderError(E_BAD_S3M);
	}
	if (!fd.read(&length, sizeof(uint16_t)))
		throw ModuleLoaderError(E_BAD_S3M);

	for (uint32_t j = 0, row = 0; row < _rows && j < length;)
	{
		uint8_t byte;
		checkLength(j, length);
		if (!fd.read(byte))
			throw ModuleLoaderError(E_BAD_S3M);
		else if (byte == 0)
		{
			++row;
			continue;
		}

		const uint8_t channel = byte & 0x1F;
		if (byte & 0x20)
		{
			uint8_t note, sample;
			if (!fd.read(note) ||
				!fd.read(sample))
				throw ModuleLoaderError(E_BAD_S3M);
			else if (channel < nChannels)
				_commands[channel][row].SetS3MNote(note, sample);
			j += 2;
			checkLength(j, length);
		}
		if (byte & 0x40)
		{
			uint8_t volume;
			if (!fd.read(volume))
				throw ModuleLoaderError(E_BAD_S3M);
			else if (channel < nChannels)
				_commands[channel][row].SetS3MVolume(volume);
			++j;
			checkLength(j, length);
		}
		if (byte & 0x80)
		{
			uint8_t effect, param;
			if (!fd.read(effect) ||
				!fd.read(param))
				throw ModuleLoaderError(E_BAD_S3M);
			if (channel < nChannels)
				_commands[channel][row].SetS3MEffect(effect, param);
			j += 2;
			checkLength(j, length);
		}
	}
}

#undef checkLength

ModulePattern::ModulePattern(STM_Intern *p_SF) : ModulePattern(4, 64, E_BAD_STM)
{
	uint8_t row, channel;
	FILE *f_STM = p_SF->f_Module;

	for (row = 0; row < _rows; row++)
	{
		for (channel = 0; channel < 4; channel++)
		{
			uint8_t Note;
			uint8_t Param;
			uint8_t Volume;
			uint8_t Effect;

			if (row == 0)
			{
				_commands[channel] = makeUnique<ModuleCommand []>(_rows);
				if (!_commands[channel])
					throw ModuleLoaderError(E_BAD_STM);
			}
			fread(&Note, 1, 1, f_STM);
			_commands[channel][row].SetSTMNote(Note);
			fread(&Param, 1, 1, f_STM);
			Volume = Param & 0x07;
			_commands[channel][row].SetSample(Param >> 3);
			fread(&Param, 1, 1, f_STM);
			Volume += Param >> 1;
			_commands[channel][row].SetVolume(Volume);
			Effect = Param & 0x0F;
			fread(&Param, 1, 1, f_STM);
			_commands[channel][row].SetSTMEffect(Effect, Param);
		}
	}
}

ModulePattern::ModulePattern(AON_Intern *p_AF, uint32_t nChannels) : ModulePattern(nChannels, 64, E_BAD_AON)
{
	uint8_t row, channel;
	FILE *f_AON = p_AF->f_Module;

	for (row = 0; row < _rows; row++)
	{
		for (channel = 0; channel < nChannels; channel++)
		{
			uint8_t Note, Samp;
			uint8_t Effect, Param;
			uint8_t ArpIndex = 0;

			if (row == 0)
			{
				_commands[channel] = makeUnique<ModuleCommand []>(_rows);
				if (!_commands[channel])
					throw ModuleLoaderError(E_BAD_AON);
			}
			fread(&Note, 1, 1, f_AON);
			_commands[channel][row].SetAONNote(Note);
			fread(&Samp, 1, 1, f_AON);
			ArpIndex |= (Samp >> 6) & 0x03;
			_commands[channel][row].SetSample(Samp & 0x3F);
			fread(&Effect, 1, 1, f_AON);
			ArpIndex |= (Effect >> 4) & 0x0C;
			_commands[channel][row].SetAONArpIndex(ArpIndex);
			fread(&Param, 1, 1, f_AON);
			_commands[channel][row].SetAONEffect(Effect & 0x3F, Param);
		}
	}
}

inline bool readInc(uint8_t &var, uint16_t &i, const uint16_t len, const fd_t &fd) noexcept
{
	if (i > len || !fd.read(var))
		return true;
	++i;
	return false;
}

ModulePattern::ModulePattern(const modIT_t &file, uint32_t nChannels) : Channels(nChannels)
{
	std::array<char, 4> dontCare;
	std::array<uint8_t, 64> channelMask;
	uint16_t len;
	std::array<ModuleCommand, 64> lastCmd;
	const fd_t &fd = file.fd();

	_commands = fixedVector_t<commandPtr_t>(nChannels);
	if (!_commands.valid() ||
		!fd.read(len) ||
		!fd.read(&_rows, 2) ||
		!fd.read(dontCare))
		throw ModuleLoaderError(E_BAD_IT);

	for (uint16_t channel = 0; channel < nChannels; ++channel)
	{
		_commands[channel] = makeUnique<ModuleCommand []>(_rows);
		if (!_commands[channel])
			throw ModuleLoaderError(E_BAD_IT);
	}

	uint16_t row = 0;
	uint16_t j = 0;
	while (row < _rows)
	{
		uint16_t channel = 0;
		uint8_t b;
		if (readInc(b, j, len, fd))
			break;
		if (b == 0)
		{
			++row;
			continue;
		}
		channel = b & 0x7F;
		if (channel != 0)
			channel = (channel - 1) & 0x3F;
		if ((b & 0x80) != 0 && readInc(channelMask[channel], j, len, fd))
			break;
		if (channel < nChannels)
			_commands[channel][row].SetITRepVal(channelMask[channel], lastCmd[channel]);
		if ((channelMask[channel] & 0x01) != 0)
		{
			uint8_t note;
			if (readInc(note, j, len, fd))
				break;
			if (channel < nChannels)
			{
				_commands[channel][row].SetITNote(note);
				lastCmd[channel].SetITNote(note);
			}
		}
		if ((channelMask[channel] & 0x02) != 0)
		{
			uint8_t sample;
			if (readInc(sample, j, len, fd))
				break;
			if (channel < nChannels)
			{
				_commands[channel][row].SetSample(sample);
				lastCmd[channel].SetSample(sample);
			}
		}
		if ((channelMask[channel] & 0x04) != 0)
		{
			uint8_t volume;
			if (readInc(volume, j, len, fd))
				break;
			if (channel < nChannels)
			{
				_commands[channel][row].SetITVolume(volume);
				lastCmd[channel].SetITVolume(volume);
			}
		}
		if ((channelMask[channel] & 0x08) != 0)
		{
			uint8_t effect, param;
			if (readInc(effect, j, len, fd) || readInc(param, j, len, fd))
				break;
			if (channel < nChannels)
			{
				_commands[channel][row].SetITEffect(effect, param);
				lastCmd[channel].SetITEffect(effect, param);
			}
		}
	}
}

void ModuleCommand::SetVolume(const uint8_t volume) noexcept
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

void ModuleCommand::SetITRepVal(const uint8_t channelMask, const ModuleCommand &lastCmd) noexcept
{
	if ((channelMask & 0x10) != 0)
		Note = lastCmd.Note;
	if ((channelMask & 0x20) != 0)
		Sample = lastCmd.Sample;
	if ((channelMask & 0x40) != 0)
	{
		VolEffect = lastCmd.VolEffect;
		VolParam = lastCmd.VolParam;
	}
	if ((channelMask & 0x80) != 0)
	{
		Effect = lastCmd.Effect;
		Param = lastCmd.Param;
	}
}

void ModuleCommand::SetITNote(uint8_t note) noexcept
{
	if (note < 0x80)
		note++;
	Note = note;
}

void ModuleCommand::SetITVolume(const uint8_t volume) noexcept
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
