#include "../libAudio.h"
#include "../genericModule/genericModule.h"
#include "moduleMixer.h"
#include "waveTables.h"

int16_t Channel::applyVibrato(const ModuleFile &module, const uint32_t period) noexcept
{
	if (Flags & CHN_VIBRATO)
	{
		int16_t delta{0};
		const uint8_t vibratoType = VibratoType & 0x03;
		if (vibratoType == 1)
			delta = RampDownTable[VibratoPos];
		else if (vibratoType == 2)
			delta = SquareTable[VibratoPos];
		else if (vibratoType == 3)
			delta = RandomTable[VibratoPos];
		else
			delta = SinusTable[VibratoPos];
		const uint8_t depthShift = module.checkTypeAndNotFlags(MODULE_IT, FILE_FLAGS_OLD_IT_EFFECTS) ? 7 : 6;
		delta = (delta * VibratoDepth) >> depthShift;
		if (module.checkTypeAndFlags(MODULE_IT, FILE_FLAGS_LINEAR_SLIDES))
		{
			if (delta < 0)
			{
				const int16_t value = -delta;
				delta = LinearSlideDown(period, value >> 2) - period;
				if (value & 0x03)
					delta += FineLinearSlideDown(period, value & 0x03) - period;
			}
			else
			{
				const int16_t value = delta;
				delta = LinearSlideUp(period, value >> 2) - period;
				if (value & 0x03)
					delta += FineLinearSlideUp(period, value & 0x03) - period;
			}
		}
		if (module.TickCount || module.checkTypeAndNotFlags(MODULE_IT, FILE_FLAGS_OLD_IT_EFFECTS))
			VibratoPos = (VibratoPos + VibratoSpeed) & 0x3F;
		return delta;
	}
	return 0;
}

int16_t Channel::applyAutoVibrato(const ModuleFile &module, const uint32_t period, int8_t &fractionalPeriod) noexcept
{
	if (Sample != nullptr && Sample->GetVibratoDepth() != 0)
	{
		ModuleSample &sample = *Sample;
		if (!sample.GetVibratoRate())
			AutoVibratoDepth = sample.GetVibratoDepth() << 8;
		else
		{
			if (module.ModuleType == MODULE_IT)
				AutoVibratoDepth += sample.GetVibratoRate();
			else if (!(Flags & CHN_NOTEOFF))
				AutoVibratoDepth += (sample.GetVibratoDepth() << 8) / (sample.GetVibratoRate() / 2);
			if ((AutoVibratoDepth >> 8) > VibratoDepth)
				AutoVibratoDepth = VibratoDepth << 8;
		}
		AutoVibratoPos += sample.GetVibratoRate();
		int8_t delta{0};
		const uint8_t vibratoType = sample.GetVibratoType();
		if (vibratoType == 1) // Square
			delta = (AutoVibratoPos & 0x80) ? 64 : -64;
		else if (vibratoType == 2) // Ramp up
			delta = ((0x40 + (AutoVibratoPos >> 1)) & 0x7F) - 0x40;
		else if (vibratoType == 3) // Ramp down
			delta = ((0x40 - (AutoVibratoPos >> 1)) & 0x7F) - 0x40;
		else if (vibratoType == 4) // Random
		{
			delta = RandomTable[AutoVibratoPos & 0x3F];
			++AutoVibratoPos;
		}
		else
			delta = FT2VibratoTable[AutoVibratoPos & 0xFF];
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
			const int32_t result = muldiv(period, a + (((a - b) * (value & 0x3F)) >> 6), 256);
			fractionalPeriod = result & 0xFF;
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
		int8_t delta{0};
		const uint8_t PanPos = ((uint16_t(PanbrelloPos) + 16) >> 2) & 0x3F;
		const uint8_t PanType = PanbrelloType & 0x03;
		if (PanType == 1)
			delta = RampDownTable[PanPos];
		else if (PanType == 2)
			delta = SquareTable[PanPos];
		else if (PanType == 3)
			delta = RandomTable[PanPos];
		else
			delta = SinusTable[PanPos];
		uint16_t Pan = Panning + ((delta * PanbrelloDepth + 2) >> 3);
		clipInt<uint16_t>(Pan, 0, 256);
		Panning = Pan;
		PanbrelloPos += PanbrelloSpeed;
	}
}
