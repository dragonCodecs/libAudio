#include "libAudio.h"
#include "libAudio.hxx"

bool Is_SID(const char *fileName)
{
	FILE *f_SID = fopen(fileName, "rb");
	char SIDMagic[4];
	if (f_SID == NULL)
		return false;

	fread(&SIDMagic, 1, 4, f_SID);
	fclose(f_SID);

	if (strncmp(SIDMagic, "PSID", 4) == 0)
		return true;
	else
		return false;
}
