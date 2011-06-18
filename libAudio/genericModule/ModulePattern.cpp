#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"

ModulePattern::ModulePattern(MOD_Intern *p_MF, uint32_t nChannels)
{
	uint32_t i, j;
	Commands = new ModuleCommand[nChannels][64];
	for (i = 0; i < 64; i++)
	{
		for (j = 0; j < nChannels; j++)
		{
			// Read 4 bytes of data and unpack it into the structure.
			uint8_t Data[4];
			fread(Data, 4, 1, p_MF->f_MOD);
			Commands[j][i].SetMODData(Data);
		}
	}
}

#define checkLength(cnt, Length) \
	if (cnt + 1 >= Length) \
		break

ModulePattern::ModulePattern(S3M_Intern *p_SF, uint32_t nChannels)
{
	uint32_t i, j, Length;
	uint8_t row = 0;
	FILE *f_S3M = p_SF->f_S3M;
	Commands = new ModuleCommand[nChannels][64];
	fread(&Length, sizeof(uint16_t), 1, f_S3M);
	for (i = 0; i < 64; i++)
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
			Commands[j][i].SetS3MData(Note, Sample);
			j += 2;
			checkLength(j, Length);
		}
		if ((byte & 0x40) != 0)
		{
			fread(&Volume, 1, 1, f_S3M);
			j++;
			checkLength(j, Length);
		}
		if ((byte & 0x80) != 0)
		{
			fread(&Effect, 1, 1, f_S3M);
			fread(&Param, 1, 1, f_S3M);
			j += 2;
			checkLength(j, Length);
		}
	}
}

#undef checkLength

ModulePattern::~ModulePattern()
{
	delete Commands;
}

ModuleCommand **ModulePattern::GetCommands()
{
	return (ModuleCommand **)Commands;
}

void ModuleCommand::SetMODData(uint8_t Data[4])
{
	Sample = (Data[0] & 0xF0) | (Data[2] >> 4);
	Period = (((uint16_t)(Data[0] & 0x0F)) << 8) | Data[1];
	Effect = (((uint16_t)(Data[2] & 0x0F)) << 8) | Data[3];
}

void ModuleCommand::SetS3MData(uint8_t Note, uint8_t sample)
{
	Sample = sample;
	Period = Note; // TODO: run a conversion!
}

uint8_t ModuleCommand::GetSample()
{
	return Sample;
}

uint16_t ModuleCommand::GetPeriod()
{
	return Period;
}

uint16_t ModuleCommand::GetEffect()
{
	return Effect;
}
