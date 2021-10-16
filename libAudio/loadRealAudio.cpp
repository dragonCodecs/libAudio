// SPDX-License-Identifier: BSD-3-Clause
#include "libAudio.h"
#include "libAudio.hxx"

typedef struct _RealMedia_File
{
	uint32_t cType;
	uint32_t cSize;
	uint16_t cVer;
	uint32_t fVer;
	uint32_t nHeaders;
} RealMedia_File;

typedef struct _RealAudio_Intern
{
	FILE *f_RLA;
} RealAudio_Intern;

void *realAudioOpenR(char *fileName)
{
	RealAudio_Intern *ret = NULL;
	FILE *f_RLA = NULL;

	ret = (RealAudio_Intern *)malloc(sizeof(RealAudio_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(RealAudio_Intern));

	f_RLA = fopen(fileName, "rb");
	if (f_RLA == NULL)
		return f_RLA;

	ret->f_RLA = f_RLA;

	return ret;
}

bool isRealAudio(char *fileName)
{
	FILE *f_RLA = fopen(fileName, "rb");
	char RLASig[4];

	if (f_RLA == NULL)
		return false;

	fread(RLASig, 4, 1, f_RLA);
	fclose(f_RLA);

	if (strncmp(RLASig, ".RMF", 4) != 0 &&
		strncmp(RLASig, ".ra\xFD", 4) != 0)
		return false;

	return true;
}
