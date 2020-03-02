#include "../libAudio.h"
#include "../genericModule/genericModule.h"
#include "moduleMixer.h"
#include "waveTables.h"

int16_t Channel::applyVibrato(const ModuleFile &module, const uint32_t period) noexcept
{
	if (Flags & CHN_VIBRATO)
	{
		auto delta{[](const uint8_t type, const uint8_t position) noexcept -> int16_t
		{
			if (type == 1)
				return RampDownTable[position];
			else if (type == 2)
				return SquareTable[position];
			else if (type == 3)
				return RandomTable[position];
			else
				return SinusTable[position];
		}(vibratoType & 3U, vibratoPosition)};
		const bool oldSfx = module.typeIs<MODULE_IT>() && module.useOldEffects();
		const uint8_t depthShift = oldSfx ? 7 : 6;
		delta = (delta * vibratoDepth) >> depthShift;
		if (module.typeIs<MODULE_IT>() && module.hasLinearSlides())
		{
			if (delta < 0)
			{
				const uint16_t amount = uint16_t(-delta);
				delta = linearSlideDown(period, amount >> 2) - period;
				const uint8_t findSlide = amount & 3U;
				if (findSlide)
					delta += fineLinearSlideDown(period, findSlide) - period;
			}
			else if (delta > 0)
			{
				const uint16_t amount = uint16_t(delta);
				delta = linearSlideUp(period, amount >> 2) - period;
				const uint8_t findSlide = amount & 3U;
				if (findSlide)
					delta += fineLinearSlideUp(period, findSlide) - period;
			}
		}
		if (module.ticks() || oldSfx)
			vibratoPosition = (vibratoPosition + vibratoSpeed) & 0x3FU;
		return delta;
	}
	return 0;
}

int16_t Channel::applyAutoVibrato(const ModuleFile &module, const uint32_t period, int8_t &fractionalPeriod) noexcept
{
	if (Sample && Sample->GetVibratoDepth() != 0)
	{
		ModuleSample &sample = *Sample;
		if (!sample.GetVibratoRate())
			AutoVibratoDepth = sample.GetVibratoDepth() << 8;
		else
		{
			if (module.ModuleType == MODULE_IT)
				AutoVibratoDepth += sample.GetVibratoRate();
			else if (!(Flags & CHN_NOTEOFF))
				AutoVibratoDepth += (sample.GetVibratoDepth() << 8) / (sample.GetVibratoRate() >> 3);
			if ((AutoVibratoDepth >> 8) > vibratoDepth)
				AutoVibratoDepth = vibratoDepth << 8;
		}
		AutoVibratoPos += sample.GetVibratoSpeed();
		const auto delta{[](const uint8_t type, uint8_t &position) noexcept -> int8_t
		{
			if (type == 1) // Square
				return (position & 0x80) ? 64 : -64;
			else if (type == 2) // Ramp up
				return ((0x40 + (position >> 1)) & 0x7F) - 0x40;
			else if (type == 3) // Ramp down
				return ((0x40 - (position >> 1)) & 0x7F) - 0x40;
			else if (type == 4) // Random
			{
				auto result = RandomTable[position & 0x3F];
				++position;
				return result;
			}
			else
				return FT2VibratoTable[position & 0xFF];
		}(sample.GetVibratoType(), AutoVibratoPos)};
		int16_t vibrato = (delta * AutoVibratoDepth) >> 8;
		if (module.ModuleType == MODULE_IT)
		{
			uint32_t a{0}, b{0};
			int16_t value{0};
			if (vibrato < 0)
			{
				value = (-vibrato) >> 8;
				a = linearSlideUp(value);
				b = linearSlideUp(value + 1);
			}
			else
			{
				value = vibrato >> 8;
				a = linearSlideDown(value);
				b = linearSlideDown(value + 1);
			}
			value >>= 2;
			const int32_t result = muldiv(period, a + (((b - a) * (value & 0x3F)) >> 6), 256);
			fractionalPeriod = uint32_t(result) & 0xFF;
			return period - (result >> 8);
		}
		else
			return vibrato >> 6;
	}
	return 0;
}

void Channel::applyPanbrello() noexcept
{
	if (Flags & CHN_PANBRELLO)
	{
		const auto delta{[](const uint8_t type, const uint8_t position) noexcept -> int8_t
		{
			if (type == 1)
				return RampDownTable[position];
			else if (type == 2)
				return SquareTable[position];
			else if (type == 3)
				return RandomTable[position];
			else
				return SinusTable[position];
		}(panbrelloType & 0x03U, ((panbrelloPosition + 16) >> 2U) & 0x3FU)};
		panbrelloPosition += panbrelloSpeed;
		Panning += (delta * panbrelloDepth + 2) >> 3U;
		clipInt<uint16_t>(Panning, 0, 256);
	}
}

void Channel::noteCut(ModuleFile &module, bool triggered) noexcept
{
	if (triggered)
	{
		if (!module.typeIs<MODULE_IT>() || module.totalInstruments())
			RawVolume = 0;
		FadeOutVol = 0;
		Flags |= CHN_FASTVOLRAMP| CHN_NOTEFADE;
	}
}

void Channel::noteOff() noexcept
{
	bool noteOn = !(Flags & CHN_NOTEOFF);
	Flags |= CHN_NOTEOFF;
	if (Instrument && !Instrument->GetEnvEnabled(ENVELOPE_VOLUME))
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
		if (Instrument->GetEnvLooped(ENVELOPE_VOLUME) && Instrument->GetFadeOut())
			Flags |= CHN_NOTEFADE;
	}
}

int32_t Channel::patternLoop(const uint8_t param, const uint16_t row) noexcept
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

void Channel::portamentoUp(const ModuleFile &module, uint8_t param) noexcept
{
	if (param)
		portamento = param;
	else
		param = portamento;
	if (!Period)
		return;
	const auto command = param & 0xF0U;
	if (module.typeIs<MODULE_S3M, MODULE_STM, MODULE_IT>() && command >= 0xE0U)
	{
		if (param & 0x0F)
		{
			if (command == 0xF0U)
				finePortamentoUp(module, param);
			else
				extraFinePortamentoUp(module, param);
		}
		return;
	}
	if (module.ticks() > StartTick || module.speed() == 1)
	{
		if (module.hasLinearSlides())// && module.typeIs<MODULE_XM>())
		{
			uint32_t oldPeriod = Period;
			Period = linearSlideDown(oldPeriod, param);
			if (Period == oldPeriod)
				--Period;
		}
		else
			Period -= uint16_t{param} << 2;
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

void Channel::portamentoDown(const ModuleFile &module, uint8_t param) noexcept
{
	if (param)
		portamento = param;
	else
		param = portamento;
	if (!Period)
		return;
	const auto command = param & 0xF0U;
	if (module.typeIs<MODULE_S3M, MODULE_STM, MODULE_IT>() && command >= 0xE0U)
	{
		if (param & 0x0F)
		{
			if (command == 0xF0U)
				finePortamentoDown(module, param);
			else
				extraFinePortamentoDown(module, param);
		}
		return;
	}
	if (module.ticks() > StartTick || module.speed() == 1)
	{
		if (module.hasLinearSlides())// && module.typeIs<MODULE_XM>())
		{
			uint32_t oldPeriod = Period;
			Period = linearSlideUp(oldPeriod, param);
			if (Period == oldPeriod)
				++Period;
		}
		else
			Period += uint16_t{param} << 2;
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

inline void Channel::finePortamentoUp(const ModuleFile &module, uint8_t param) noexcept
{
	if (module.ticks() == StartTick && Period && param)
	{
		if (module.hasLinearSlides())
			Period = linearSlideDown(Period, param & 0x0FU);
		else
			Period -= uint16_t{param} << 2U;
		if (Period < module.minimumPeriod())
			Period = module.minimumPeriod();
	}
}

inline void Channel::finePortamentoDown(const ModuleFile &module, uint8_t param) noexcept
{
	if (module.ticks() == StartTick && Period && param)
	{
		if (module.hasLinearSlides())
			Period = linearSlideUp(Period, param & 0x0FU);
		else
			Period += uint16_t{param} << 2U;
		if (Period > module.maximumPeriod())
			Period = module.maximumPeriod();
	}
}

inline void Channel::extraFinePortamentoUp(const ModuleFile &module, uint8_t param) noexcept
{
	if (module.ticks() == StartTick && Period && param)
	{
		if (module.hasLinearSlides())
			Period = fineLinearSlideDown(Period, param & 0x0F);
		else
			Period -= param;
		if (Period < module.minimumPeriod())
			Period = module.minimumPeriod();
	}
}

inline void Channel::extraFinePortamentoDown(const ModuleFile &module, uint8_t param) noexcept
{
	if (module.ticks() == StartTick && Period && param)
	{
		if (module.hasLinearSlides())
			Period = fineLinearSlideUp(Period, param & 0x0FU);
		else
			Period += param;
		if (Period > module.maximumPeriod())
			Period = module.maximumPeriod();
	}
}

void Channel::tonePortamento(const ModuleFile &module, uint8_t param)
{
	if (param != 0)
		portamentoSlide = param;
	Flags |= CHN_PORTAMENTO;
	if (Period && portamentoTarget && module.ticks() != StartTick)
	{
		if (Period < portamentoTarget)
		{
			uint16_t delta{};
			if (module.hasLinearSlides())
			{
				int16_t slide = linearSlideUp(Period, portamentoSlide) - Period;
				delta = slide < 1 ? 1 : uint16_t(slide);
			}
			else
				delta = uint16_t(portamentoSlide) << 2U;
			if (portamentoTarget - Period < delta)
				delta = portamentoTarget - Period;
			Period += delta;
		}
		else if (Period > portamentoTarget)
		{
			uint16_t delta{};
			if (module.hasLinearSlides())
			{
				int16_t slide = linearSlideDown(Period, portamentoSlide) - Period;
				delta = slide > -1 ? 1 : uint16_t(-slide);
			}
			else
				delta = uint16_t(portamentoSlide) << 2U;
			if (Period - portamentoTarget < delta)
				delta = Period - portamentoTarget;
			Period -= delta;
		}
	}
}

void Channel::vibrato(uint8_t param, uint8_t multiplier)
{
	if ((param & 0x0F) != 0)
		vibratoDepth = (param & 0x0F) * multiplier;
	if ((param & 0xF0) != 0)
		vibratoSpeed = param >> 4;
	Flags |= CHN_VIBRATO;
}

void Channel::panbrello(uint8_t param)
{
	if ((param & 0x0F) != 0)
		panbrelloDepth = param & 0x0F;
	if ((param & 0xF0) != 0)
		panbrelloSpeed = param >> 4;
	Flags |= CHN_PANBRELLO;
}

void Channel::volumeSlide(const ModuleFile &module, uint8_t param)
{
	if (!param)
		param = channelVolumeSlide;
	else
		channelVolumeSlide = param;

	uint8_t volume = channelVolume;
	const uint8_t slideLo = param & 0x0FU;
	const uint8_t slideHi = param & 0xF0U;
	if (!module.ticks())
	{
		if (slideHi == 0xF0U && slideLo)
			volume -= slideLo;
		else if (slideLo == 0x0FU && slideHi)
			volume += slideHi >> 4U;
	}
	else if (slideLo)
		volume -= slideLo;
	else
		volume += slideHi >> 4U;

	if (volume != channelVolume)
	{
		clipInt<uint8_t>(volume, 0, 64);
		channelVolume = volume;
	}
}

void Channel::panningSlide(const ModuleFile &module, uint8_t param)
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
			slide -= slideHi >> 2;
		else if (slideHi == 0xF0U && slideLo)
			slide += slideLo << 2;
	}
	else if (slideHi)
		slide -= slideHi >> 2;
	else
		slide += slideLo << 2;

	if (slide != RawPanning)
	{
		clipInt<uint16_t>(slide, 0, 256);
		RawPanning = slide;
	}
}
