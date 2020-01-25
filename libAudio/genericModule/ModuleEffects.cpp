#include "genericModule.h"

void ModuleCommand::TranslateMODEffect(const uint8_t cmd, const uint8_t param) noexcept
{
	switch (cmd)
	{
		case 0:
			if (param == 0)
				Effect = CMD_NONE;
			else
				Effect = CMD_ARPEGGIO;
			Param = param;
			break;
		case 1:
			Effect = CMD_PORTAMENTOUP;
			Param = param;
			break;
		case 2:
			Effect = CMD_PORTAMENTODOWN;
			Param = param;
			break;
		case 3:
			Effect = CMD_TONEPORTAMENTO;
			Param = param;
			break;
		case 4:
			Effect = CMD_VIBRATO;
			Param = param;
			break;
		case 5:
			Effect = CMD_TONEPORTAVOL;
			Param = param;
			break;
		case 6:
			Effect = CMD_VIBRATOVOL;
			Param = param;
			break;
		case 7:
			Effect = CMD_TREMOLO;
			Param = param;
			break;
		case 8:
			Effect = CMD_PANNING;
			Param = param;
			break;
		case 9:
			Effect = CMD_OFFSET;
			Param = param;
			break;
		case 10:
			Effect = CMD_VOLUMESLIDE;
			Param = param;
			break;
		case 11:
			Effect = CMD_POSITIONJUMP;
			Param = param;
			break;
		case 12:
			Effect = CMD_VOLUME;
			Param = param;
			break;
		case 13:
			Effect = CMD_PATTERNBREAK;
			Param = param;
			break;
		case 14:
			Effect = CMD_MOD_EXTENDED;
			Param = param;
			break;
		case 15:
			/* Speed rules (as per http://www.aes.id.au/modformat.html)
			 * param == 0 => param = 1
			 * param <= 32 => Speed = NewSpeed (TPR)
			 * param > 32 => Tempo = NewSpeed (BPM)
			 */
			if (param <= 32)
				Effect = CMD_SPEED;
			else
				Effect = CMD_TEMPO;
			Param = (param == 0 ? 1 : param);
			break;
	}
}

void ModuleCommand::SetSTMEffect(uint8_t effect, uint8_t param)
{
	switch (effect)
	{
		case 1:
			Effect = CMD_SPEED;
			Param = param >> 4;
			break;
		case 2:
			Effect = CMD_POSITIONJUMP;
			Param = param;
			break;
		case 3:
			Effect = CMD_PATTERNBREAK;
			Param = ((param >> 4) * 10) + (param & 0x0F);
			break;
		case 4:
			Effect = CMD_VOLUMESLIDE;
			Param = param;
			break;
		case 5:
			Effect = CMD_PORTAMENTODOWN;
			Param = param;
			break;
		case 6:
			Effect = CMD_PORTAMENTOUP;
			Param = param;
			break;
		case 7:
			Effect = CMD_TONEPORTAMENTO;
			Param = param;
			break;
		case 8:
			Effect = CMD_VIBRATO;
			Param = param;
			break;
		case 9:
			Effect = CMD_TREMOR;
			Param = param;
			break;
		case 10:
			Effect = CMD_ARPEGGIO;
			Param = param;
			break;
		case 11:
			Effect = CMD_VIBRATOVOL;
			Param = param;
			break;
		case 12:
			Effect = CMD_TONEPORTAVOL;
			Param = param;
			break;
		default:
			Effect = CMD_NONE;
	}
}

void ModuleCommand::SetS3MEffect(uint8_t effect, uint8_t param)
{
	if (effect > 1 && effect <= 12)
	{
		SetSTMEffect(effect, param);
		return;
	}
	switch (effect)
	{
		case 1:
			Effect = CMD_SPEED;
			Param = param;
			break;
		case 13:
			Effect = CMD_CHANNELVOLUME;
			Param = param;
			break;
		case 14:
			Effect = CMD_CHANNELVOLSLIDE;
			Param = param;
			break;
		case 15:
			Effect = CMD_OFFSET;
			Param = param;
			break;
		case 16:
			Effect = CMD_PANNINGSLIDE;
			Param = param;
			break;
		case 17:
			Effect = CMD_RETRIGER;
			Param = param;
			break;
		case 18:
			Effect = CMD_TREMOLO;
			Param = param;
			break;
		case 19:
			Effect = CMD_S3M_EXTENDED;
			Param = param;
			break;
		case 20:
			Effect = CMD_TEMPO;
			Param = param;
			break;
		case 21:
			Effect = CMD_FINEVIBRATO;
			Param = param;
			break;
		case 22:
			Effect = CMD_GLOBALVOLUME;
			Param = param;
			break;
		case 23:
			Effect = CMD_GLOBALVOLSLIDE;
			Param = param;
			break;
		case 24:
			Effect = CMD_PANNING;
			Param = param;
			break;
		case 25:
			Effect = CMD_PANBRELLO;
			Param = param;
			break;
		default:
			Effect = CMD_NONE;
	}
}

void ModuleCommand::SetAONEffect(uint8_t effect, uint8_t param)
{
	// Effect 8 is unused, 0 is CMD_NONE
	// and all the standard MOD effect IDs
	// map as in MOD otherwise
	if (effect >= 1 && effect != 8 && effect <= 15)
	{
		TranslateMODEffect(effect, param);
		return;
	}
	switch (effect)
	{
		// TODO: Look up what these translate to and how they work
		// from the code at line 28906 in aon.8free.S
		// AON_DOFX
		// aon_fxcom
		// aon_fxdat
		/*case 16:
			Effect = CMD_
			break;
		case 17:
			Effect = CMD_
			break;
		case 18:
			Effect = CMD_
			break;
		case 19:
			Effect = CMD_
			break;
		case 20:
			Effect = CMD_
			break;
		case 21:
			Effect = CMD_
			break;*/
		case 22:
			Effect = CMD_TONEPORTAVOLUP;
			Param = param;
			break;
		case 23:
			Effect = CMD_TONEPORTAVOLDOWN;
			Param = param;
			break;
		/*case 24:
			Effect = CMD_
			break;
		case 25:
			Effect = CMD_
			break;
		case 26:
			Effect = CMD_
			break;
		case 27:
			Effect = CMD_
			break;
		case 28:
			Effect = CMD_
			break;
		case 29:
			Effect = CMD_
			break;
		case 30:
			Effect = CMD_
			break;
		case 33:
			Effect = CMD_
			break;*/
		default:
			Effect = CMD_NONE;
	}
}

void ModuleCommand::SetITEffect(uint8_t effect, uint8_t param)
{
	if (effect < 26)
	{
		SetS3MEffect(effect, param);
		if (effect == 3)
			Param = param;
		return;
	}
	Effect = CMD_NONE;
}
