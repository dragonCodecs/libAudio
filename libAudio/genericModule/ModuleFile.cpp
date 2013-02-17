#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"

#ifndef _WINDOWS
#ifndef max
#define max(a, b) (a > b ? a : b)
#endif
#endif

void *ModuleAllocator::operator new(size_t size)
{
	void *ret = ::operator new(size);
	memset(ret, 0x00, size);
	return ret;
}

void *ModuleAllocator::operator new[](size_t size)
{
	void *ret = ::operator new[](size);
	memset(ret, 0x00, size);
	return ret;
}

ModuleFile::ModuleFile(MOD_Intern *p_MF) : ModuleType(MODULE_MOD), Channels(NULL), MixerChannels(NULL)
{
	uint32_t i, maxPattern;
	FILE *f_MOD = p_MF->f_MOD;

	p_Header = new ModuleHeader(p_MF);
	fseek(f_MOD, 20, SEEK_SET);
	p_Samples = new ModuleSample *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
		p_Samples[i] = ModuleSample::LoadSample(p_MF, i);
	fseek(f_MOD, 130, SEEK_CUR);
	if (p_Header->nSamples != 15)
		fseek(f_MOD, 4, SEEK_CUR);

	// Count the number of patterns present
	for (i = 0, maxPattern = 0; i < 128; i++)
	{
		if (p_Header->Orders[i] < 64)
			maxPattern = max(maxPattern, p_Header->Orders[i]);
	}
	p_Header->nPatterns = maxPattern + 1;
	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	for (i = 0; i < p_Header->nPatterns; i++)
		p_Patterns[i] = new ModulePattern(p_MF, p_Header->nChannels);

	MODLoadPCM(f_MOD);
	MinPeriod = 56;
	MaxPeriod = 7040;
}

ModuleFile::ModuleFile(S3M_Intern *p_SF) : ModuleType(MODULE_S3M), Channels(NULL), MixerChannels(NULL)
{
	uint32_t i;
	FILE *f_S3M = p_SF->f_S3M;

	p_Header = new ModuleHeader(p_SF);
	p_Samples = new ModuleSample *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
	{
		uint32_t SeekLoc = ((uint32_t)(p_Header->SamplePtrs[i])) << 4;
		fseek(f_S3M, SeekLoc, SEEK_SET);
		p_Samples[i] = ModuleSample::LoadSample(p_SF, i);
	}

	// Count the number of channels present
	p_Header->nChannels = 32;
	for (i = 0; i < 32; i++)
	{
		if ((p_Header->ChannelSettings[i] & 0x80) != 0)
		{
			p_Header->nChannels = i;
			break;
		}
	}

	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	for (i = 0; i < p_Header->nPatterns; i++)
	{
		uint32_t SeekLoc = ((uint32_t)p_Header->PatternPtrs[i]) << 4;
		fseek(f_S3M, SeekLoc, SEEK_SET);
		p_Patterns[i] = new ModulePattern(p_SF, p_Header->nChannels);
	}

	S3MLoadPCM(f_S3M);
	MinPeriod = 64;
	MaxPeriod = 32767;
}

ModuleFile::ModuleFile(STM_Intern *p_SF) : ModuleType(MODULE_STM), Channels(NULL), MixerChannels(NULL)
{
	uint32_t i;
	FILE *f_STM = p_SF->f_STM;

	p_Header = new ModuleHeader(p_SF);
	p_Samples = new ModuleSample *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
		p_Samples[i] = ModuleSample::LoadSample(p_SF, i);
	fseek(f_STM, 128, SEEK_CUR);
	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	for (i = 0; i < p_Header->nPatterns; i++)
		p_Patterns[i] = new ModulePattern(p_SF);
	fseek(f_STM, 1104 + (1024 * p_Header->nPatterns), SEEK_SET);

	STMLoadPCM(f_STM);
	MinPeriod = 64;
	MaxPeriod = 32767;
}

ModuleFile::ModuleFile(FC1x_Intern *p_FF) : ModuleType(MODULE_FC1x), Channels(NULL), MixerChannels(NULL)
{
}

ModuleFile::~ModuleFile()
{
	uint32_t i;

	DeinitMixer();

	for (i = 0; i < p_Header->nSamples; i++)
	{
		delete [] p_PCM[i];
		p_PCM[i] = NULL;
	}
	delete [] p_PCM;
	for (i = 0; i < p_Header->nPatterns; i++)
	{
		delete p_Patterns[i];
		p_Patterns[i] = NULL;
	}
	delete [] p_Patterns;
	for (i = 0; i < p_Header->nSamples; i++)
	{
		delete p_Samples[i];
		p_Samples[i] = NULL;
	}
	delete [] p_Samples;
	delete p_Header;
}

const char *ModuleFile::GetTitle()
{
	return strdup(p_Header->Name);
}

uint8_t ModuleFile::GetChannels()
{
	return (p_Header->MasterVolume & 0x80) == 0 ? 1 : 2;
}

void ModuleFile::MODLoadPCM(FILE *f_MOD)
{
	uint32_t i;
	p_PCM = new uint8_t *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
	{
		uint32_t Length = p_Samples[i]->GetLength();
		if (Length != 0)
		{
			p_PCM[i] = new uint8_t[Length];
			fread(p_PCM[i], Length, 1, f_MOD);
			p_PCM[i][0] = p_PCM[i][1] = 0;
			/*if (strncasecmp((char *)p_PCM[i] + 2, "ADPCM", 5) == 0)
			{
				uint32_t j;
				uint8_t *compressionTable = p_PCM[i];
				uint8_t *compBuffer = &p_PCM[i][16];
				uint8_t delta = 0;
				Length -= 16;
				p_Samples[i]->Length = Length;
				Length *= 2;
				p_PCM[i] = (uint8_t *)malloc(Length);
				for (j = 0; j < p_Samples[i]->Length; j++)
				{
					delta += compressionTable[compBuffer[j] & 0x0F];
					p_PCM[i][(j * 2) + 0] = delta;
					delta += compressionTable[(compBuffer[j] >> 4) & 0x0F];
					p_PCM[i][(j * 2) + 1] = delta;
				}
				free(compressionTable);
				compBuffer = compressionTable = NULL;
			}*/
		}
		else
			p_PCM[i] = NULL;
	}
}

void ModuleFile::S3MLoadPCM(FILE *f_S3M)
{
	uint32_t i;
	p_PCM = new uint8_t *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
	{
		uint32_t SeekLoc, Length = p_Samples[i]->GetLength() << (p_Samples[i]->Get16Bit() ? 1 : 0);
		if (Length != 0 && p_Samples[i]->GetType() == 1)
		{
			SeekLoc = ((ModuleSampleNative *)p_Samples[i])->SamplePos << 4;
			p_PCM[i] = new uint8_t[Length];
			fseek(f_S3M, SeekLoc, SEEK_SET);
			fread(p_PCM[i], Length, 1, f_S3M);
			if (p_Header->FormatVersion == 2)
			{
				uint32_t j;
				if (p_Samples[i]->Get16Bit())
				{
					short *pcm = (short *)p_PCM[i];
					for (j = 0; j < (Length >> 1); j++)
						pcm[j] ^= 0x7FFF;
				}
				else
				{
					char *pcm = (char *)p_PCM[i];
					for (j = 0; j < Length; j++)
						pcm[j] ^= 0x7F;
				}
			}
		}
		else
			p_PCM[i] = NULL;
	}
}

void ModuleFile::STMLoadPCM(FILE *f_STM)
{
	uint32_t i;
	p_PCM = new uint8_t *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
	{
		uint32_t Length = p_Samples[i]->GetLength();
		if (Length != 0)
		{
			p_PCM[i] = new uint8_t[Length];
			fread(p_PCM[i], Length, 1, f_STM);
			fseek(f_STM, Length % 16, SEEK_CUR);
		}
		else
			p_PCM[i] = NULL;
	}
}

ModuleLoaderError::ModuleLoaderError(uint32_t error) : Error(error)
{
}

const char *ModuleLoaderError::GetError()
{
	switch (Error)
	{
		case E_BAD_S3M:
			return "Bad Scream Tracker III Module";
		case E_BAD_STM:
			return "Bad Scream Tracker Module - Maybe just song data?";
		default:
			return "Unknown error";
	}
}
