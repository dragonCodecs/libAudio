#include "../libAudio.h"
#include "../genericModule/genericModule.h"
#include "moduleMixer.h"
#include "waveTables.h"

int16_t Channel::applyVibrato() noexcept
{
	int16_t period{0};
	if (Sample != nullptr && Sample->GetVibratoDepth() != 0)
	{
		int8_t Delta;
		ModuleSample &sample = *Sample;
		uint8_t VibratoType = sample.GetVibratoType();
		//channel->AutoVibratoRate = sample->GetVibratoRate();
		const uint8_t vibratoPos = (AutoVibratoPos + sample.GetVibratoSpeed()) & 0x03;
		if (VibratoType == 1)
			Delta = RampDownTable[vibratoPos];
		else if (VibratoType == 2)
			Delta = SquareTable[vibratoPos];
		else if (VibratoType == 3)
			Delta = RandomTable[vibratoPos];
		else
			Delta = SinusTable[vibratoPos];
		period = int16_t((Delta * sample.GetVibratoDepth()) >> 7);
		AutoVibratoPos = vibratoPos;
	}
	return period;
}

void Channel::applyPanbrello() noexcept
{
	if (Flags & CHN_PANBRELLO)
	{
		int8_t Delta;
		const uint8_t PanPos = ((uint16_t(PanbrelloPos) + 16) >> 2) & 0x3F;
		const uint8_t PanType = PanbrelloType & 0x03;
		uint16_t Pan = Panning;
		if (PanType == 1)
			Delta = RampDownTable[PanPos];
		else if (PanType == 2)
			Delta = SquareTable[PanPos];
		else if (PanType == 3)
			Delta = RandomTable[PanPos];
		else
			Delta = SinusTable[PanPos];
		Pan += (Delta * PanbrelloDepth) >> 4;
		clipInt<uint16_t>(Pan, 0, 256);
		Panning = Pan;
		PanbrelloPos += PanbrelloSpeed;
	}
}
