#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

void *MOD_OpenR(const char *FileName)
{
	MOD_Intern *ret = NULL;
	FILE *f_MOD = NULL;

	ret = (MOD_Intern *)malloc(sizeof(MOD_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(MOD_Intern));

	f_MOD = fopen(FileName, "rb");
	if (f_MOD == NULL)
		return f_MOD;
	ret->f_Module = f_MOD;

	return ret;
}

FileInfo *MOD_GetFileInfo(void *p_MODFile)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	FileInfo *ret = NULL;

	if (p_MF == NULL)
		return ret;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));
	p_MF->p_FI = ret;

	ret->BitRate = 44100;
	ret->BitsPerSample = 16;
	ret->Channels = 2;
	try
	{
		p_MF->p_File = new ModuleFile(p_MF);
	}
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->GetError());
		return NULL;
	}
	ret->Title = p_MF->p_File->title().release();

	if (ExternalPlayback == 0)
		p_MF->p_Playback = new Playback(ret, MOD_FillBuffer, p_MF->buffer, 8192, p_MODFile);
	p_MF->p_File->InitMixer(ret);

	return ret;
}

long MOD_FillBuffer(void *p_MODFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	int32_t ret = 0, Read;
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	if (p_MF->p_File == NULL)
		return -1;
	do
	{
		Read = p_MF->p_File->Mix(p_MF->buffer, nOutBufferLen - ret);
		if (Read >= 0 && OutBuffer != p_MF->buffer)
			memcpy(OutBuffer + ret, p_MF->buffer, Read);
		if (Read >= 0)
			ret += Read;
	}
	while (ret < nOutBufferLen && Read >= 0);
	return (ret == 0 ? Read : ret);
}

int MOD_CloseFileR(void *p_MODFile)
{
	int ret = 0;
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	if (p_MF == NULL)
		return 0;

	delete p_MF->p_Playback;
	delete p_MF->p_File;

	ret = fclose(p_MF->f_Module);
	free(p_MF);
	return ret;
}

void MOD_Play(void *p_MODFile)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;

	p_MF->p_Playback->Play();
}

void MOD_Pause(void *p_MODFile)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;

	p_MF->p_Playback->Pause();
}

void MOD_Stop(void *p_MODFile)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;

	p_MF->p_Playback->Stop();
}

bool Is_MOD(const char *FileName)
{
	FILE *f_MOD = fopen(FileName, "rb");
	char MODMagic[4];
	if (f_MOD == NULL)
		return false;

	fseek(f_MOD, (30 * 31) + 150, SEEK_CUR);
	fread(MODMagic, 4, 1, f_MOD);
	fclose(f_MOD);

	if (strncmp(MODMagic, "M.K.", 4) == 0 || strncmp(MODMagic, "M!K!", 4) == 0 ||
		strncmp(MODMagic, "M&K!", 4) == 0 || strncmp(MODMagic, "N.T.", 4) == 0)
		return true;
	else if (strncmp(MODMagic, "CD81", 4) == 0 || strncmp(MODMagic, "OKTA", 4) == 0)
		return true;
	else if (strncmp(MODMagic, "FLT", 3) == 0 && MODMagic[3] >= '4' && MODMagic[3] <= '9')
		return true;
	else if (strncmp(MODMagic + 1, "CHN", 3) == 0 && MODMagic[0] >= '4' && MODMagic[0] <= '9')
		return true;
	else if (strncmp(MODMagic + 2, "CH", 2) == 0 && MODMagic[0] == '1' && MODMagic[1] >= '0' && MODMagic[1] <= '9')
		return true;
	else if (strncmp(MODMagic + 2, "CH", 2) == 0 && MODMagic[0] == '2' && MODMagic[1] >= '0' && MODMagic[1] <= '9')
		return true;
	else if (strncmp(MODMagic + 2, "CH", 2) == 0 && MODMagic[0] == '3' && MODMagic[1] >= '0' && MODMagic[1] <= '9')
		return true;
	else if (strncmp(MODMagic, "TDZ", 3) == 0 && MODMagic[3] >= '4' && MODMagic[3] <= '9')
		return true;
	else if (strncmp(MODMagic, "16CN", 4) == 0)
		return true;
	else if (strncmp(MODMagic, "32CN", 4) == 0)
		return true;
	else // Probbaly not a MOD, but just as likely with the above tests that it is..
		return false; // so we can't do old ProTracker MODs with 15 samples and can't take the auto-detect tests 100% seriously.
}

API_Functions MODDecoder =
{
	MOD_OpenR,
	MOD_GetFileInfo,
	MOD_FillBuffer,
	MOD_CloseFileR,
	MOD_Play,
	MOD_Pause,
	MOD_Stop
};
