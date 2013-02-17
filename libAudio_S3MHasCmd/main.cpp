#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

uint8_t ExternalPlayback = 1;

class S3MHeader : public ModuleAllocator
{
public:
	// Common fields
	char *Name;
	uint16_t nOrders;
	uint16_t nSamples;
	uint16_t nPatterns;
	uint8_t *Orders;

	// Fields specific to certain formats

	// MOD
	uint8_t RestartPos;

	// S3M
	uint8_t Type;
	uint16_t Flags;
	uint16_t CreationVersion;
	uint16_t FormatVersion;
	uint8_t GlobalVolume;
	uint8_t InitialSpeed;
	uint8_t InitialTempo;
	uint8_t MasterVolume;
	uint8_t ChannelSettings[32];
	// Slightly badly named
	// SamplePtrs = pointers to where the sample *descriptors* are
	uint16_t *SamplePtrs;
	// PatternPtrs = pointers to where the compressed pattern data is
	uint16_t *PatternPtrs;

	uint8_t *Panning;

	uint8_t nChannels;
};

class S3MCommand : public ModuleAllocator
{
public:
	uint8_t Sample;
	uint8_t Note;
	uint8_t VolEffect;
	uint8_t VolParam;
	uint8_t Effect;
	uint8_t Param;
};

class S3MFile : public ModuleAllocator
{
public:
	uint8_t ModuleType;
	S3MHeader *p_Header;
	ModuleSample **p_Samples;
	ModulePattern **p_Patterns;
};

S3M_Intern *openS3M(const char *FileName)
{
	S3M_Intern *ret = NULL;
	FILE *f_S3M = NULL;
	ret = (S3M_Intern *)malloc(sizeof(S3M_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(S3M_Intern));

	f_S3M = fopen(FileName, "rb");
	if (f_S3M == NULL)
	{
		free(ret);
		return NULL;
	}
	ret->f_S3M = f_S3M;
	return ret;
}

S3MFile *readS3M(S3M_Intern *p_SF)
{
	FILE *f_S3M;
	FileInfo *p_FI;

	if (p_SF == NULL)
		return NULL;
	f_S3M = p_SF->f_S3M;

	p_FI = (FileInfo *)malloc(sizeof(FileInfo));
	if (p_FI == NULL)
		return NULL;
	memset(p_FI, 0x00, sizeof(FileInfo));
	p_SF->p_FI = p_FI;

	p_FI->BitRate = 44100;
	p_FI->BitsPerSample = 16;
	try
	{
		p_SF->p_File = new ModuleFile(p_SF);
	}
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->GetError());
		return NULL;
	}
	p_FI->Title = p_SF->p_File->GetTitle();
	p_FI->Channels = p_SF->p_File->GetChannels();

	return (S3MFile *)p_SF->p_File;
}

int closeS3M(S3M_Intern *p_SF)
{
	int ret = 0;
	if (p_SF == NULL)
		return 0;

	delete p_SF->p_Playback;
	delete p_SF->p_File;

	ret = fclose(p_SF->f_S3M);
	free(p_SF);
	return ret;
}

bool isHex(const char *str, int len)
{
	for (int i = 0; i < len; i++)
	{
		if (!((str[i] >= '0' && str[i] <= '9') ||
			(str[i] >= 'A' && str[i] <= 'Z') ||
			(str[i] >= 'a' && str[i] <= 'z')))
			return false;
	}
	return true;
}

uint8_t hex2int(char c)
{
	uint8_t ret = (uint8_t)(c - '0');
	if (ret > 9)
	{
		if (c >= 'A' && c <= 'Z')
			ret -= 17;
		else
			ret -= 49;
	}
	return ret;
}

uint8_t getEffect(const char *hex)
{
	size_t len = strlen(hex);
	if (len == 2 && isHex(hex, len))
	{
		uint8_t ret = hex2int(hex[0]) << 4;
		ret |= hex2int(hex[1]);
		if (ret != 0)
			return ret;
	}

	printf("Bad effect ID, must consist of exactly 2 non-zero hex digits!\n");
	exit(1);

	return 0;
}

int main(int argc, char **argv)
{
	uint8_t effect;

	if (argc != 3)
	{
		// usage();
		return 2;
	}

//	printf("Checking command line....\n");
	effect = getEffect(argv[2]);

	printf("Checking if S3M.... ");
	if (Is_S3M(argv[1]))
	{
		bool found = false;
		uint64_t i;
		S3M_Intern *p_SF = openS3M(argv[1]);
		S3MFile *module = readS3M(p_SF);
		ModulePattern **patterns = module->p_Patterns;

		printf("yes\n");
		printf("Name - '%s'....\n", module->p_Header->Name);
//		printf("Patterns - %u....\n", module->p_Header->nPatterns);
//		printf("Channels - %u....\n", module->p_Header->nChannels);

		printf("Checking for command");
		fflush(stdout);
		for (i = 0; i < module->p_Header->nPatterns; i++)
		{
			uint8_t j;
			S3MCommand (*commands)[64] = (S3MCommand (*)[64])patterns[i]->GetCommands();

			for (j = 0; j < module->p_Header->nChannels; j++)
			{
				for (uint8_t k = 0; k < 64; k++)
				{
					// If effect not "CMD_NONE" and equals effect
					if (commands[j][k].Effect == effect)
					{
						found = true;
						goto effectFound;
					}
				}
			}

			printf(".");
			fflush(stdout);
		}
effectFound:
		printf(" %s\n", found ? "yes" : "no");

		closeS3M(p_SF);
		return 0;
	}
	else
	{
		printf("no\n");
		return 1;
	}
}
