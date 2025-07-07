// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2019-2023 Rachel Mant <git@dragonmux.network>
#include "../libAudio.h"
#include "../genericModule/genericModule.h"
#include "moduleMixer.h"
#include "waveTables.h"

int16_t channel_t::applyTremolo(const ModuleFile &module, const uint16_t newVolume) noexcept
{
	int16_t result{};
	if (newVolume)
	{
		const uint8_t type = tremoloType & 0x03U;
		const uint16_t depth = tremoloDepth << 4U;
		if (type == 1)
			result = (rampDownTable[tremoloPos] * depth) >> 8U;
		else if (type == 2)
			result = (squareTable[tremoloPos] * depth) >> 8U;
		else if (type == 3)
			result = (randomTable[tremoloPos] * depth) >> 8U;
		else
			result = (sinusTable[tremoloPos] * depth) >> 8U;
	}
	if (module.ticks() > startTick)
		tremoloPos = uint32_t(tremoloPos + tremoloSpeed) & 0x3FU;
	return result;
}

uint16_t channel_t::applyTremor(const ModuleFile &module, const uint16_t newVolume) noexcept
{
	uint16_t result{};
	if (module.ticks() || module.typeIs<MODULE_S3M>())
	{
		const uint8_t onTime = (tremor >> 4U) + 1U;
		const uint8_t count
		{
			[=](const uint8_t duration) noexcept -> uint8_t
			{
				const uint8_t count{tremorCount};
				if (count > duration)
					return 0;
				return count;
			}(onTime + (tremor & 0x0FU) + 1U)
		};

		if (count > onTime)
			result = newVolume;
		tremorCount = count + 1;
	}
	Flags |= CHN_FASTVOLRAMP;
	return result;
}

uint16_t channel_t::applyNoteFade(const uint16_t newVolume) noexcept
{
	if (!Instrument)
		return newVolume;
	const uint16_t FadeOut{Instrument->GetFadeOut()};
	if (FadeOut)
	{
		if (FadeOutVol < (FadeOut << 1U))
			FadeOutVol = 0;
		else
			FadeOutVol -= FadeOut << 1U;
		return uint32_t(newVolume * FadeOutVol) >> 16U;
	}
	else if (FadeOutVol == 0U)
		return 0U;
	return newVolume;
}

uint16_t channel_t::applyVolumeEnvelope(const ModuleFile &module, const uint16_t newVolume) noexcept
{
	if (!Instrument)
		return newVolume;
	auto &envelope{Instrument->GetEnvelope(envelopeType_t::volume)};
	if (!envelope.GetEnabled() || !envelope.HasNodes())
		return newVolume;
	uint8_t volValue{envelope.Apply(EnvVolumePos)};
	auto result{uint16_t(muldiv_t<uint32_t>{}(newVolume, volValue, 1U << 6U))};
	clipInt<uint16_t>(result, 0, 128);
	++EnvVolumePos;
	if (envelope.GetLooped())
	{
		uint16_t endTick = envelope.GetLoopEnd();
		if (EnvVolumePos == ++endTick)
		{
			EnvVolumePos = envelope.GetLoopBegin();
			if (envelope.IsZeroLoop() && envelope.Apply(EnvVolumePos) == 0)
			{
				Flags |= CHN_NOTEFADE;
				FadeOutVol = 0;
			}
		}
	}
	if (envelope.GetSustained() && !(Flags & CHN_NOTEOFF))
	{
		uint16_t endTick = envelope.GetSustainEnd();
		if (EnvVolumePos == ++endTick)
			EnvVolumePos = envelope.GetSustainBegin();
	}
	else if (envelope.IsAtEnd(EnvVolumePos))
	{
		if (module.typeIs<MODULE_IT>() || (Flags & CHN_NOTEOFF))
			Flags |= CHN_NOTEFADE;
		EnvVolumePos = envelope.GetLastTick();
		if (envelope.Apply(EnvVolumePos) == 0)
		{
			Flags |= CHN_NOTEFADE;
			FadeOutVol = 0;
			return 0;
		}
	}
	return result;
}

void channel_t::applyPanningEnvelope() noexcept
{
	if (!Instrument)
		return;
	auto &envelope{Instrument->GetEnvelope(envelopeType_t::panning)};
	if (!envelope.GetEnabled() || !envelope.HasNodes())
		return;
	uint16_t result{RawPanning};
	auto panningValue{int8_t(envelope.Apply(EnvPanningPos) - 128)};
	clipInt<int8_t>(panningValue, -32, 32);
	if (result >= 128)
		result += (panningValue * (256 - result)) / 32;
	else
		result += (panningValue * result) / 32;
	clipInt<uint16_t>(result, 0, 256);
	panning = result;
	++EnvPanningPos;
	if (envelope.GetLooped())
	{
		uint16_t endTick = envelope.GetLoopEnd();
		if (EnvPanningPos == ++endTick)
			EnvPanningPos = endTick;
	}
	if (envelope.GetSustained() && !(Flags & CHN_NOTEOFF))
	{
		uint16_t endTick = envelope.GetSustainEnd();
		if (EnvPanningPos == ++endTick)
			EnvPanningPos = envelope.GetSustainBegin();
	}
	else if (envelope.IsAtEnd(EnvPanningPos))
		EnvPanningPos = envelope.GetLastTick();
}

uint32_t channel_t::applyPitchEnvelope(const uint32_t period) noexcept
{
	if (!Instrument)
		return period;
	auto &envelope{Instrument->GetEnvelope(envelopeType_t::pitch)};
	if (!envelope.GetEnabled() || !envelope.HasNodes())
		return period;
	auto pitchValue{int8_t(envelope.Apply(EnvPitchPos) - 128)};
	clipInt<int8_t>(pitchValue, -32, 32);
	auto result{period};
	if (envelope.IsFilter())
	{
	}
	else
	{
		if (pitchValue < 0)
		{
			uint16_t adjust = uint16_t(-pitchValue) << 3U;
			if (adjust > 255U)
				adjust = 255U;
			result = linearSlideUp(period, adjust);
		}
		else if (pitchValue > 0)
		{
			uint16_t adjust = uint16_t(pitchValue) << 3U;
			if (adjust > 255U)
				adjust = 255U;
			result = linearSlideDown(period, adjust);
		}
	}
	++EnvPitchPos;
	if (envelope.GetLooped())
	{
		uint16_t endTick = envelope.GetLoopEnd();
		if (EnvPitchPos == ++endTick)
			EnvPitchPos = envelope.GetLoopBegin();
	}
	if (envelope.GetSustained() && (Flags & CHN_NOTEOFF) == 0)
	{
		uint16_t endTick = envelope.GetSustainEnd();
		if (EnvPitchPos == ++endTick)
			EnvPitchPos = envelope.GetSustainBegin();
	}
	else if (envelope.IsAtEnd(EnvPitchPos))
		EnvPitchPos = envelope.GetLastTick();
	return result;
}

int16_t channel_t::applyVibrato(const ModuleFile &module, const uint32_t period) noexcept
{
	if (Flags & CHN_VIBRATO)
	{
		auto delta{[](const uint8_t type, const uint8_t position) noexcept -> int16_t
		{
			if (type == 1)
				return rampDownTable[position];
			else if (type == 2)
				return squareTable[position];
			else if (type == 3)
				return randomTable[position];
			else
				return sinusTable[position];
		}(vibratoType & 3U, vibratoPosition)};
		const bool oldSfx = module.typeIs<MODULE_IT>() && module.useOldEffects();
		const uint8_t depthShift = oldSfx ? 7 : 6;
		delta = (delta * vibratoDepth) >> depthShift;
		if (module.typeIs<MODULE_IT>() && module.hasLinearSlides())
		{
			if (delta < 0)
			{
				const auto amount{uint16_t(-delta)};
				delta = static_cast<int16_t>(linearSlideDown(period, static_cast<uint8_t>(amount >> 2U)) - period);
				const uint8_t findSlide = amount & 3U;
				if (findSlide)
					delta += static_cast<int16_t>(fineLinearSlideDown(period, findSlide) - period);
			}
			else if (delta > 0)
			{
				const auto amount{uint16_t(delta)};
				delta = static_cast<int16_t>(linearSlideUp(period, static_cast<uint8_t>(amount >> 2U)) - period);
				const uint8_t findSlide = amount & 3U;
				if (findSlide)
					delta += static_cast<int16_t>(fineLinearSlideUp(period, findSlide) - period);
			}
		}
		if (module.ticks() || oldSfx)
			vibratoPosition = uint32_t(vibratoPosition + vibratoSpeed) & 0x3FU;
		return delta;
	}
	return 0;
}

int16_t channel_t::applyAutoVibrato(const ModuleFile &module, const uint32_t period, int8_t &fractionalPeriod) noexcept
{
	if (Sample && Sample->GetVibratoDepth() != 0)
	{
		ModuleSample &sample = *Sample;
		if (!sample.GetVibratoRate())
			autoVibratoDepth = sample.GetVibratoDepth() << 8U;
		else
		{
			if (module.ModuleType == MODULE_IT)
				autoVibratoDepth += sample.GetVibratoRate();
			else if (!(Flags & CHN_NOTEOFF))
				autoVibratoDepth += (sample.GetVibratoDepth() << 8U) / (sample.GetVibratoRate() >> 3U);
			if ((autoVibratoDepth >> 8U) > vibratoDepth)
				autoVibratoDepth = vibratoDepth << 8U;
		}
		autoVibratoPos += sample.GetVibratoSpeed();
		const auto delta{[](const uint8_t type, uint8_t &position) noexcept -> int8_t
		{
			if (type == 1) // Square
				return (position & 0x80U) ? 64 : -64;
			else if (type == 2) // Ramp up
				return ((0x40U + (position >> 1U)) & 0x7FU) - 0x40;
			else if (type == 3) // Ramp down
				return ((0x40U - (position >> 1U)) & 0x7FU) - 0x40;
			else if (type == 4) // Random
			{
				auto result = randomTable[position & 0x3FU];
				++position;
				return result;
			}
			else
				return ft2VibratoTable[position];
		}(sample.GetVibratoType(), autoVibratoPos)};
		int16_t vibrato = (delta * autoVibratoDepth) >> 8U;
		if (module.ModuleType == MODULE_IT)
		{
			uint32_t a{0}, b{0};
			uint8_t value{0};
			if (vibrato < 0)
			{
				value = uint8_t(uint16_t(-vibrato) >> 8U);
				a = linearSlideUp(value);
				b = linearSlideUp(value + 1);
			}
			else
			{
				value = uint8_t(uint16_t(vibrato) >> 8U);
				a = linearSlideDown(value);
				b = linearSlideDown(value + 1);
			}
			value >>= 2U;
			const int32_t result = muldiv_t<>{}(period, a + (((b - a) * (value & 0x3FU)) >> 6U), 256);
			fractionalPeriod = uint32_t(result) & 0xFFU;
			return static_cast<int16_t>(period - (result >> 8U));
		}
		else
			return vibrato >> 6U;
	}
	return 0;
}

void channel_t::applyPanbrello() noexcept
{
	if (Flags & CHN_PANBRELLO)
	{
		const auto delta{[](const uint8_t type, const uint8_t position) noexcept -> int8_t
		{
			if (type == 1)
				return rampDownTable[position];
			else if (type == 2)
				return squareTable[position];
			else if (type == 3)
				return randomTable[position];
			else
				return sinusTable[position];
		}(panbrelloType & 0x03U, ((panbrelloPosition + 16U) >> 2U) & 0x3FU)};
		panbrelloPosition += panbrelloSpeed;
		panning += (delta * panbrelloDepth + 2U) >> 3U;
		clipInt<uint16_t>(panning, 0, 256);
	}
}

void channel_t::noteCut(ModuleFile &module, uint32_t tick) noexcept
{
	/*if (tick == 0 && module.typeIs<MODULE_S3M>())
		return;*/
	if (module.ticks() == tick)
	{
		RawVolume = 0;
		Flags |= CHN_FASTVOLRAMP;
	}
}

void channel_t::noteOff() noexcept
{
	bool noteOn = !(Flags & CHN_NOTEOFF);
	Flags |= CHN_NOTEOFF;
	if (Instrument && !Instrument->GetEnvEnabled(envelopeType_t::volume))
		Flags |= CHN_NOTEFADE;
	if (!Length)
		return;
	if ((Flags & CHN_SUSTAINLOOP) && Sample && noteOn)
	{
		if (Sample->GetLooped())
		{
			if (Sample->GetBidiLoop())
				Flags |= CHN_LPINGPONG;
			else
				Flags &= ~(CHN_LPINGPONG | CHN_FPINGPONG);
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
			Flags &= ~(CHN_LOOP | CHN_LPINGPONG | CHN_FPINGPONG);
			Length = Sample->GetLength();
		}
	}
	if (Instrument)
	{
		if (Instrument->GetEnvLooped(envelopeType_t::volume) && Instrument->GetFadeOut())
			Flags |= CHN_NOTEFADE;
	}
}

int32_t channel_t::patternLoop(const uint8_t param, const uint16_t row) noexcept
{
	if (param)
	{
		if (patternLoopCount)
		{
			if (!--patternLoopCount)
			{
				// Reset the default start position for the next
				// CMDEX_LOOP
				patternLoopStart = 0;
				return -1;
			}
		}
		else
			patternLoopCount = param;
		return patternLoopStart;
	}
	else
		patternLoopStart = row;
	return -1;
}

void channel_t::portamentoUp(const ModuleFile &module, uint8_t param) noexcept
{
	if (!param && module.typeIs<MODULE_MOD>())
		return;

	if (param)
		portamento = param;
	else
		param = portamento;

	const auto command = param & 0xF0U;
	if (module.typeIs<MODULE_S3M, MODULE_STM, MODULE_IT>() && command >= 0xE0U)
	{
		param &= 0x0fU;
		if (param)
		{
			if (command == 0xF0U)
				finePortamentoUp(module, param);
			else
				extraFinePortamentoUp(module, param);
		}
		return;
	}
	if (module.ticks() > startTick)// || module.speed() == 1)
	{
		if (!Period)
			return;
		if (module.hasLinearSlides())// && module.typeIs<MODULE_XM>())
		{
			uint32_t oldPeriod = Period;
			Period = linearSlideDown(oldPeriod, param);
			if (Period == oldPeriod)
				--Period;
		}
		else
			Period -= uint16_t{param} << 2U;
		if (Period < module.minimumPeriod())
		{
			Period = module.minimumPeriod();
			if (module.typeIs<MODULE_IT>())
			{
				Flags |= CHN_NOTEFADE;
				FadeOutVol = 0;
			}
		}
	}
}

void channel_t::portamentoDown(const ModuleFile &module, uint8_t param) noexcept
{
	if (!param && module.typeIs<MODULE_MOD>())
		return;

	if (param)
		portamento = param;
	else
		param = portamento;

	const auto command = param & 0xF0U;
	if (module.typeIs<MODULE_S3M, MODULE_STM, MODULE_IT>() && command >= 0xE0U)
	{
		param &= 0x0fU;
		if (param)
		{
			if (command == 0xF0U)
				finePortamentoDown(module, param);
			else
				extraFinePortamentoDown(module, param);
		}
		return;
	}

	if (module.ticks() > startTick)// || module.speed() == 1)
	{
		if (!Period)
			return;
		if (module.hasLinearSlides())// && !module.typeIs<MODULE_XM>())
		{
			uint32_t oldPeriod = Period;
			Period = linearSlideUp(oldPeriod, param);
			if (Period == oldPeriod)
				++Period;
		}
		else
			Period += uint16_t{param} << 2U;
		if (Period > module.maximumPeriod())
		{
			Period = module.maximumPeriod();
			if (module.typeIs<MODULE_IT>())
			{
				Flags |= CHN_NOTEFADE;
				FadeOutVol = 0;
			}
		}
	}
}

inline void channel_t::finePortamentoUp(const ModuleFile &module, uint8_t param) noexcept
{
	if (module.ticks() == startTick && Period && param)
	{
		if (module.hasLinearSlides())
			Period = linearSlideDown(Period, param & 0x0FU);
		else
			Period -= uint16_t{param} << 2U;
		if (Period < module.minimumPeriod())
			Period = module.minimumPeriod();
	}
}

inline void channel_t::finePortamentoDown(const ModuleFile &module, uint8_t param) noexcept
{
	if (module.ticks() == startTick && Period && param)
	{
		if (module.hasLinearSlides())
			Period = linearSlideUp(Period, param & 0x0FU);
		else
			Period += uint16_t{param} << 2U;
		if (Period > module.maximumPeriod())
			Period = module.maximumPeriod();
	}
}

inline void channel_t::extraFinePortamentoUp(const ModuleFile &module, uint8_t param) noexcept
{
	if (module.ticks() == startTick && Period && param)
	{
		if (module.hasLinearSlides())// && !module.typeIs<MODULE_XM>())
			Period = fineLinearSlideDown(Period, param & 0x0FU);
		else
			Period -= param;
		if (Period < module.minimumPeriod())
			Period = module.minimumPeriod();
	}
}

inline void channel_t::extraFinePortamentoDown(const ModuleFile &module, uint8_t param) noexcept
{
	if (module.ticks() == startTick && Period && param)
	{
		if (module.hasLinearSlides())// && !module.typeIs<MODULE_XM>())
			Period = fineLinearSlideUp(Period, param & 0x0FU);
		else
			Period += param;
		if (Period > module.maximumPeriod())
			Period = module.maximumPeriod();
	}
}

void channel_t::tonePortamento(const ModuleFile &module, uint8_t param)
{
	if (param)
		portamentoSlide = param;
	Flags |= CHN_PORTAMENTO;
	if (Period && portamentoTarget && module.ticks() != startTick)
	{
		if (Period < portamentoTarget)
		{
			uint16_t delta{};
			if (module.hasLinearSlides())
			{
				const auto slide = static_cast<int16_t>(linearSlideUp(Period, portamentoSlide) - Period);
				delta = slide < 1 ? 1 : uint16_t(slide);
			}
			else
				delta = uint16_t(portamentoSlide) << 2U;
			if (portamentoTarget - Period < delta)
				delta = static_cast<uint16_t>(portamentoTarget - Period);
			Period += delta;
		}
		else if (Period > portamentoTarget)
		{
			uint16_t delta{};
			if (module.hasLinearSlides())
			{
				const auto slide = static_cast<int16_t>(linearSlideDown(Period, portamentoSlide) - Period);
				delta = slide > -1 ? 1 : uint16_t(-slide);
			}
			else
				delta = uint16_t(portamentoSlide) << 2U;
			if (Period - portamentoTarget < delta)
				delta = static_cast<uint16_t>(Period - portamentoTarget);
			Period -= delta;
		}
	}
}

void channel_t::vibrato(uint8_t param, uint8_t multiplier)
{
	if (param & 0x0FU)
		vibratoDepth = (param & 0x0FU) * multiplier;
	if (param & 0xF0U)
		vibratoSpeed = param >> 4U;
	Flags |= CHN_VIBRATO;
}

void channel_t::panbrello(uint8_t param)
{
	if (param & 0x0FU)
		panbrelloDepth = param & 0x0FU;
	if (param & 0xF0U)
		panbrelloSpeed = param >> 4U;
	Flags |= CHN_PANBRELLO;
}

void channel_t::channelVolumeSlide(const ModuleFile &module, uint8_t param) noexcept
{
	if (!param)
		param = _channelVolumeSlide;
	else
		_channelVolumeSlide = param;

	uint8_t newVolume = channelVolume;
	const uint8_t slideLo = param & 0x0FU;
	const uint8_t slideHi = param & 0xF0U;
	if (!module.ticks())
	{
		if (slideHi == 0xF0U && slideLo)
			newVolume -= slideLo;
		else if (slideLo == 0x0FU && slideHi)
			newVolume += slideHi >> 4U;
	}
	else if (slideLo)
		newVolume -= slideLo;
	else
		newVolume += slideHi >> 4U;

	if (newVolume != channelVolume)
	{
		clipInt<uint8_t>(newVolume, 0, 64);
		channelVolume = newVolume;
	}
}

void channel_t::sampleVolumeSlide(const ModuleFile &module, uint8_t param)
{
	if (param == 0)
		param = _sampleVolumeSlide;
	else
		_sampleVolumeSlide = param;
	uint16_t NewVolume{RawVolume};

	if (module.typeIs<MODULE_S3M, MODULE_STM, MODULE_IT>())
	{
		if ((param & 0x0FU) == 0x0FU)
		{
			if (param & 0xF0U)
				return fineSampleVolumeSlide(module, param >> 4U,
					[](const uint16_t volume, const uint8_t adjust) noexcept -> uint16_t
						{ return volume + adjust; }
				);
			else if (module.ticks() == startTick && !module.hasFastSlides())
				NewVolume -= 0x1EU; //0x0F * 2;
		}
		else if ((param & 0xF0U) == 0xF0U)
		{
			if (param & 0x0FU)
				return fineSampleVolumeSlide(module, param >> 4U,
					[](const uint16_t volume, const uint8_t adjust) noexcept -> uint16_t
						{ return volume - adjust; }
				);
			else if (module.ticks() == startTick && !module.hasFastSlides())
				NewVolume += 0x1EU; //0x0F * 2;
		}
	}

	if (module.ticks() > startTick || module.hasFastSlides())
	{
		if ((param & 0xF0U) && !(param & 0x0FU))
			NewVolume += (param & 0xF0U) >> 1U;
		else if ((param & 0x0FU) && !(param & 0xF0U))
			NewVolume -= (param & 0x0FU) << 1U;
		if (module.typeIs<MODULE_MOD>())
			Flags |= CHN_FASTVOLRAMP;
	}
	clipInt<uint16_t>(NewVolume, 0, 128);
	RawVolume = static_cast<uint8_t>(NewVolume);
}

inline void channel_t::fineSampleVolumeSlide(const ModuleFile &module, uint8_t param,
	uint16_t (*const op)(const uint16_t, const uint8_t)) noexcept
{
	if (param == 0)
		param = _fineSampleVolumeSlide;
	else
		_fineSampleVolumeSlide = param;

	if (module.ticks() == startTick)
	{
		auto newVolume{op(RawVolume, param << 1U)};
		if (module.typeIs<MODULE_MOD>())
			Flags |= CHN_FASTVOLRAMP;
		clipInt<uint16_t>(newVolume, 0, 128);
		RawVolume = static_cast<uint8_t>(newVolume);
	}
}

void channel_t::panningSlide(const ModuleFile &module, uint8_t param)
{
	if (!param)
		param = _panningSlide;
	else
		_panningSlide = param;

	uint16_t slide = RawPanning;
	const uint8_t slideLo = param & 0x0FU;
	const uint8_t slideHi = param & 0xF0U;
	if (!module.ticks())
	{
		if (slideLo == 0x0FU && slideHi)
			slide -= slideHi >> 2U;
		else if (slideHi == 0xF0U && slideLo)
			slide += slideLo << 2U;
	}
	else if (slideHi)
		slide -= slideHi >> 2U;
	else
		slide += slideLo << 2U;

	if (slide != RawPanning)
	{
		clipInt<uint16_t>(slide, 0, 256);
		RawPanning = slide;
	}
}
