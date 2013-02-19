#include <stdio.h>
#include <malloc.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

void *AON_OpenR(const char *FileName)
{
	AON_Intern *ret = NULL;
	FILE *f_AON = NULL;

	ret = (AON_Intern *)malloc(sizeof(AON_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(AON_Intern));

	f_AON = fopen(FileName, "rb");
	if (f_AON == NULL)
		return f_AON;
	ret->f_AON = f_AON;

	return ret;
}

FileInfo *AON_GetFileInfo(void *p_AONFile)
{
	AON_Intern *p_SF = (AON_Intern *)p_AONFile;
	FileInfo *ret = NULL;

	if (p_SF == NULL)
		return ret;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));
	p_SF->p_FI = ret;

	ret->BitRate = 44100;
	ret->BitsPerSample = 16;
	try
	{
		p_SF->p_File = new ModuleFile(p_SF);
	}
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->GetError());
		return NULL;
	}
	ret->Title = p_SF->p_File->GetTitle();
	ret->Artist = p_SF->p_File->GetAuthor();
	{
		const char *Remark = p_SF->p_File->GetRemark();
		if (Remark != NULL)
		{
			ret->OtherComments.push_back(Remark);
			ret->nOtherComments++;
		}
	}
	//ret->Channels = p_SF->p_File->GetChannels();

	//if (ExternalPlayback == 0)
	//	p_SF->p_Playback = new Playback(ret, AON_FillBuffer, p_SF->buffer, 8192, p_AONFile);
	//p_SF->p_File->InitMixer(ret);

	return ret;
}

long AON_FillBuffer(void *p_AONFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	int32_t ret = 0, Read;
	AON_Intern *p_SF = (AON_Intern *)p_AONFile;
	if (p_SF->p_File == NULL)
		return -1;
	do
	{
		Read = p_SF->p_File->Mix(p_SF->buffer, nOutBufferLen - ret);
		if (Read >= 0 && OutBuffer != p_SF->buffer)
			memcpy(OutBuffer + ret, p_SF->buffer, Read);
		if (Read >= 0)
			ret += Read;
	}
	while (ret < nOutBufferLen && Read >= 0);
	return (ret == 0 ? Read : ret);
}

int AON_CloseFileR(void *p_AONFile)
{
	int ret = 0;
	AON_Intern *p_SF = (AON_Intern *)p_AONFile;
	if (p_SF == NULL)
		return 0;

	delete p_SF->p_Playback;
	delete p_SF->p_File;

	ret = fclose(p_SF->f_AON);
	free(p_SF);
	return ret;
}

void AON_Play(void *p_AONFile)
{
	AON_Intern *p_SF = (AON_Intern *)p_AONFile;

	//p_SF->p_Playback->Play();
}

void AON_Pause(void *p_AONFile)
{
	AON_Intern *p_SF = (AON_Intern *)p_AONFile;

	p_SF->p_Playback->Pause();
}

void AON_Stop(void *p_AONFile)
{
	AON_Intern *p_SF = (AON_Intern *)p_AONFile;

	p_SF->p_Playback->Stop();
}

bool Is_AON(const char *FileName)
{
	FILE *f_AON = fopen(FileName, "rb");
	char AONMagic1[4], AONMagic2[42];
	if (f_AON == NULL)
		return false;

	fread(AONMagic1, 4, 1, f_AON);
	fread(AONMagic2, 42, 1, f_AON);
	fclose(f_AON);

	if (strncmp(AONMagic1, "AON", 3) == 0 && (AONMagic1[3] == '4' || AONMagic1[3] == '8') &&
		strncmp(AONMagic2, "artofnoise by bastian spiegel (twice/lego)", 42) == 0)
		return true;
	else
		return false;
}

API_Functions AONDecoder =
{
	AON_OpenR,
	AON_GetFileInfo,
	AON_FillBuffer,
	AON_CloseFileR,
	AON_Play,
	AON_Pause,
	AON_Stop
};