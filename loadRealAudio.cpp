#include "libAudio.h"
#include "libAudio_Common.h"

typedef struct _RealMedia_File
{
	DWORD cType;
	DWORD cSize;
	WORD cVer;
	DWORD fVer;
	DWORD nHeaders;
} RealMedia_File;

typedef struct _RealAudio_Intern
{
	FILE *f_RLA;
} RealAudio_Intern;

void *RealAudio_OpenR(char *FileName)
{
	RealAudio_Intern *ret = NULL;
	FILE *f_RLA = NULL;

	ret = (RealAudio_Intern *)malloc(sizeof(RealAudio_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(RealAudio_Intern));

	f_RLA = fopen(FileName, "rb");
	if (f_RLA == NULL)
		return f_RLA;

	ret->f_RLA = f_RLA;

	return ret;
}

bool Is_RealAudio(char *FileName)
{
	FILE *f_RLA = fopen(FileName, "rb");
	char RLASig[4];

	fread(RLASig, 4, 1, f_RLA);
	fclose(f_RLA);

	if (strncmp(RLASig, ".RMF", 4) != 0 &&
		strncmp(RLASig, ".ra\xFD", 4) != 0)
		return false;

	return true;
}