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
		const uint8_t depthShift = module.ModuleType == MODULE_IT ? 7 : 6;
		delta = (delta * VibratoDepth) >> depthShift;
		if (module.ModuleType == MODULE_IT && module.p_Header->Flags & FILE_FLAGS_LINEAR_SLIDES)
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
		if (module.TickCount || module.ModuleType == MODULE_IT)
			VibratoPos = (VibratoPos + VibratoSpeed) & 0x3F;
		return delta;
	}
	return 0;
}

int16_t Channel::applyAutoVibrato() noexcept
{
	if (Sample != nullptr && Sample->GetVibratoDepth() != 0)
	{
		int8_t delta{0};
		ModuleSample &sample = *Sample;
		const uint8_t vibratoType = sample.GetVibratoType();
		//channel->AutoVibratoRate = sample->GetVibratoRate();
		const uint8_t vibratoPos = (AutoVibratoPos + sample.GetVibratoSpeed()) & 0x03;
		if (vibratoType == 1)
			delta = RampDownTable[vibratoPos];
		else if (vibratoType == 2)
			delta = SquareTable[vibratoPos];
		else if (vibratoType == 3)
			delta = RandomTable[vibratoPos];
		else
			delta = SinusTable[vibratoPos];
		AutoVibratoPos = vibratoPos;
		return int16_t((delta * sample.GetVibratoDepth()) >> 7);
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
		uint16_t Pan = Panning;
		if (PanType == 1)
			delta = RampDownTable[PanPos];
		else if (PanType == 2)
			delta = SquareTable[PanPos];
		else if (PanType == 3)
			delta = RandomTable[PanPos];
		else
			delta = SinusTable[PanPos];
		Pan += (delta * PanbrelloDepth) >> 4;
		clipInt<uint16_t>(Pan, 0, 256);
		Panning = Pan;
		PanbrelloPos += PanbrelloSpeed;
	}
}
