
#define HAVE_CONFIG_H
#include <Shorten/shorten.h>

#include "libAudio.h"
#include "libAudio_Common.h"

typedef struct _Shorten_Intern
{
	FILE *f_SHN;
} Shorten_Intern;

void *Shorten_OpenR(char *FileName)
{
	Shorten_Intern *ret = NULL;
	FILE *f_SHN = NULL;

	ret = (Shorten_Intern *)malloc(sizeof(Shorten_Intern));
	if (ret == NULL)
		return ret;

	f_SHN = fopen(FileName, "rb");
	if (f_SHN == NULL)
		return f_SHN;

	ret->f_SHN = f_SHN;

	return NULL;
}

bool Is_Shorten(char *FileName)
{
	FILE *f_SHN = fopen(FileName, "rb");
	char ShortenSig[4];

	if (f_SHN == NULL)
		return false;

	fread(ShortenSig, 4, 1, f_SHN);
	fclose(f_SHN);

	if (strncmp(ShortenSig, "ajkg", 4) != 0)
		return false;

	return true;
}
