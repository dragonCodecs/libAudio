// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2012-2023 Rachel Mant <git@dragonmux.network>
#include <substrate/utility>
#include <substrate/promotion_helpers>
#include "genericModule.h"

using substrate::make_unique;

static const uint16_t Periods[60] =
{
	1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017, 961, 907,
	856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
	428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
	107, 101, 95, 90, 85, 80, 76, 71, 67, 64, 60, 57
};

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
pattern_t::pattern_t(const uint32_t _channels, const uint16_t rows, const uint32_t type) :
	Channels{_channels}, _commands{_channels}, _rows{rows}
{
	if (!_commands.valid())
		throw ModuleLoaderError{type};
}

pattern_t::pattern_t(const modMOD_t &file, const uint32_t channels) : pattern_t{channels, 64, E_BAD_MOD}
{
	const fd_t &fd = file.fd();
	for (size_t row = 0; row < _rows; ++row)
	{
		for (size_t channel = 0; channel < channels; ++channel)
		{
			// Read 4 bytes of data and unpack it into the structure.
			std::array<uint8_t, 4> data{};
			if (row == 0)
			{
				_commands[channel] = makeUnique<command_t []>(_rows);
				if (!_commands[channel])
					throw ModuleLoaderError{E_BAD_MOD};
			}
			fd.read(data);
			_commands[channel][row].setMODData(data);
		}
	}
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define checkLength(cnt, length) \
	if ((cnt) + 1 >= (length)) \
		break

pattern_t::pattern_t(const modS3M_t &file, const uint32_t channels) : pattern_t{channels, 64, E_BAD_S3M}
{
	uint32_t length{};
	const fd_t &fd = file.fd();

	for (uint32_t i = 0; i < channels; ++i)
	{
		_commands[i] = makeUnique<command_t []>(_rows);
		if (!_commands[i])
			throw ModuleLoaderError{E_BAD_S3M};
	}
	if (!fd.read(&length, sizeof(uint16_t)))
		throw ModuleLoaderError{E_BAD_S3M};

	for (uint32_t j = 0, row = 0; row < _rows && j < length;)
	{
		uint8_t byte{};
		checkLength(j, length);
		if (!fd.read(byte))
			throw ModuleLoaderError{E_BAD_S3M};
		else if (byte == 0)
		{
			++row;
			continue;
		}

		const uint8_t channel = byte & 0x1FU;
		if (byte & 0x20U)
		{
			uint8_t note{};
			uint8_t sample{};
			if (!fd.read(note) ||
				!fd.read(sample))
				throw ModuleLoaderError{E_BAD_S3M};
			else if (channel < channels)
				_commands[channel][row].setS3MNote(note, sample);
			j += 2;
			checkLength(j, length);
		}
		if (byte & 0x40U)
		{
			uint8_t volume{};
			if (!fd.read(volume))
				throw ModuleLoaderError{E_BAD_S3M};
			else if (channel < channels)
				_commands[channel][row].setS3MVolume(volume);
			++j;
			checkLength(j, length);
		}
		if (byte & 0x80U)
		{
			uint8_t effect{};
			uint8_t param{};
			if (!fd.read(effect) ||
				!fd.read(param))
				throw ModuleLoaderError{E_BAD_S3M};
			if (channel < channels)
				_commands[channel][row].setS3MEffect(effect, param);
			j += 2;
			checkLength(j, length);
		}
	}
}

#undef checkLength

pattern_t::pattern_t(const modSTM_t &file) : pattern_t(4, 64, E_BAD_STM)
{
	const fd_t &fd = file.fd();

	for (size_t row{}; row < _rows; ++row)
	{
		for (size_t channel{}; channel < 4; ++channel)
		{
			if (row == 0)
			{
				_commands[channel] = makeUnique<command_t []>(_rows);
				if (!_commands[channel])
					throw ModuleLoaderError{E_BAD_STM};
			}

			uint8_t Note{};
			uint8_t Param{};

			if (!fd.read(Note) ||
				!fd.read(Param))
				throw ModuleLoaderError{E_BAD_STM};
			_commands[channel][row].setSTMNote(Note);
			uint8_t Volume = Param & 0x07U;
			_commands[channel][row].setSample(Param >> 3U);
			if (!fd.read(Param))
				throw ModuleLoaderError{E_BAD_STM};
			Volume += Param >> 1U;
			_commands[channel][row].setVolume(Volume);
			const uint8_t Effect = Param & 0x0FU;
			if (!fd.read(Param))
				throw ModuleLoaderError{E_BAD_STM};
			_commands[channel][row].setSTMEffect(Effect, Param);
		}
	}
}

pattern_t::pattern_t(const modAON_t &file, const uint32_t channels) : pattern_t{channels, 64, E_BAD_AON}
{
	using arithUInt = substrate::promoted_type_t<uint8_t>;
	const fd_t &fd = file.fd();
	for (size_t row{}; row < _rows; ++row)
	{
		for (size_t channel{}; channel < channels; ++channel)
		{
			if (row == 0)
			{
				_commands[channel] = make_unique<command_t []>(_rows);
				if (!_commands[channel])
					throw ModuleLoaderError{E_BAD_AON};
			}
			uint8_t note{};
			uint8_t sample{};
			uint8_t effect{};
			uint8_t param{};
			if (!fd.read(note) ||
				!fd.read(sample) ||
				!fd.read(effect) ||
				!fd.read(param))
				throw ModuleLoaderError{E_BAD_AON};
			const uint8_t arpIndex = ((arithUInt{sample} >> 6U) & 0x03U) | ((arithUInt{effect} >> 4U) & 0x0CU);
			_commands[channel][row].setAONNote(note);
			_commands[channel][row].setSample(sample & 0x3FU);
			_commands[channel][row].setAONArpIndex(arpIndex);
			_commands[channel][row].setAONEffect(effect & 0x3FU, param);
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

pattern_t::pattern_t(const modIT_t &file, const uint32_t channels) : Channels{channels}
{
	std::array<char, 4> dontCare{};
	std::array<uint8_t, 64> channelMask{};
	uint16_t len{};
	std::array<command_t, 64> lastCmd{};
	const fd_t &fd = file.fd();

	_commands = fixedVector_t<commandPtr_t>(channels);
	if (!_commands.valid() ||
		!fd.read(len) ||
		!fd.read(&_rows, 2) ||
		!fd.read(dontCare))
		throw ModuleLoaderError{E_BAD_IT};

	for (size_t channel = 0; channel < channels; ++channel)
	{
		_commands[channel] = makeUnique<command_t []>(_rows);
		if (!_commands[channel])
			throw ModuleLoaderError{E_BAD_IT};
	}

	uint16_t row = 0;
	uint16_t j = 0;
	while (row < _rows)
	{
		uint16_t channel = 0;
		uint8_t b{};
		if (readInc(b, j, len, fd))
			break;
		if (b == 0)
		{
			++row;
			continue;
		}
		channel = b & 0x7FU;
		if (channel != 0)
			channel = (channel - 1U) & 0x3FU;
		if ((b & 0x80U) != 0 && readInc(channelMask[channel], j, len, fd))
			break;
		if (channel < channels)
			_commands[channel][row].setITRepVal(channelMask[channel], lastCmd[channel]);
		if ((channelMask[channel] & 0x01U) != 0)
		{
			uint8_t note{};
			if (readInc(note, j, len, fd))
				break;
			if (channel < channels)
			{
				_commands[channel][row].setITNote(note);
				lastCmd[channel].setITNote(note);
			}
		}
		if ((channelMask[channel] & 0x02U) != 0)
		{
			uint8_t sample{};
			if (readInc(sample, j, len, fd))
				break;
			if (channel < channels)
			{
				_commands[channel][row].setSample(sample);
				lastCmd[channel].setSample(sample);
			}
		}
		if ((channelMask[channel] & 0x04U) != 0)
		{
			uint8_t volume{};
			if (readInc(volume, j, len, fd))
				break;
			if (channel < channels)
			{
				_commands[channel][row].setITVolume(volume);
				lastCmd[channel].setITVolume(volume);
			}
		}
		if ((channelMask[channel] & 0x08U) != 0)
		{
			uint8_t effect{};
			uint8_t param{};
			if (readInc(effect, j, len, fd) || readInc(param, j, len, fd))
				break;
			if (channel < channels)
			{
				_commands[channel][row].setITEffect(effect, param);
				lastCmd[channel].setITEffect(effect, param);
			}
		}
	}
}

void command_t::setVolume(const uint8_t volume) noexcept
{
	VolEffect = VOLCMD_VOLUME;
	VolParam = volume;
}

uint8_t command_t::modPeriodToNoteIndex(const uint16_t period) noexcept
{
	if (period == 0)
		return 0;
	uint8_t min{0};
	uint8_t max{59};
	do
	{
		const auto i = min + ((max - min) / 2);
		if (Periods[i] == period)
			return 37 + i;
		else if (Periods[i] < period)
		{
			if (i > 0)
			{
				uint32_t Dist1 = period - Periods[i];
				uint32_t Dist2 = abs(Periods[i - 1] - period);
				if (Dist1 < Dist2)
					return 37 + i;
			}
			max = i - 1;
		}
		else if (Periods[i] > period)
		{
			if (i < 59)
			{
				uint32_t Dist1 = Periods[i] - period;
				uint32_t Dist2 = abs(period - Periods[i + 1]);
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

void command_t::setMODData(const std::array<uint8_t, 4> &data) noexcept
{
	using arithUInt = substrate::promoted_type_t<uint8_t>;
	Sample = (arithUInt{data[0]} & 0xF0U) | (arithUInt{data[2]} >> 4U);
	Note = modPeriodToNoteIndex(((arithUInt{data[0]} & 0x0FU) << 8U) | data[1]);
	translateMODEffect(data[2] & 0x0FU, data[3]);
}

void command_t::setS3MNote(const uint8_t note, const uint8_t sample)
{
	Sample = sample;
	if (note < 0xF0U)
		Note = (note & 0x0FU) + (12U * (note >> 4U)) + 13U;
	else if (note == 0xFF)
		Note = 0;
	else
		Note = note;
}

void command_t::setS3MVolume(const uint8_t volume)
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

void command_t::setSTMNote(const uint8_t note)
{
	if (note > 250)
	{
		if (note == 254 || note == 252)
			Note = 254;
	}
	else
	{
		uint8_t Octave = (note & 0xF0U) >> 4U;
		uint8_t Pitch = note & 0x0FU;
		Pitch %= 12;
		Note = (Octave * 12) + Pitch + 37;
	}
}

void command_t::setITRepVal(const uint8_t channelMask, const command_t &lastCmd) noexcept
{
	if ((channelMask & 0x10U) != 0)
		Note = lastCmd.Note;
	if ((channelMask & 0x20U) != 0)
		Sample = lastCmd.Sample;
	if ((channelMask & 0x40U) != 0)
	{
		VolEffect = lastCmd.VolEffect;
		VolParam = lastCmd.VolParam;
	}
	if ((channelMask & 0x80U) != 0)
	{
		Effect = lastCmd.Effect;
		Param = lastCmd.Param;
	}
}

void command_t::setITNote(const uint8_t note) noexcept
{
	if (note < 0x80)
		Note = note + 1U;
	else
		Note = note;
}

void command_t::setITVolume(const uint8_t volume) noexcept
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
