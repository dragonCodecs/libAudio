#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "ProTracker.h"
#include "moduleMixer/moduleMixer.h"

#ifndef _WINDOWS
#ifdef CHAR
#undef CHAR
#endif
#define CHAR char
#ifndef max
#define max(a, b) (a > b ? a : b)
#endif
#endif

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

	ret->f_MOD = f_MOD;
	ret->p_Header = (MODHeader *)malloc(sizeof(MODHeader));
	memset(ret->p_Header, 0x00, sizeof(MODHeader));

	return ret;
}

FileInfo *MOD_GetFileInfo(void *p_MODFile)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	FileInfo *ret = NULL;
	FILE *f_MOD;
	CHAR MODMagic[4];
	UINT i, maxPattern;

	if (p_MF == NULL)
		return ret;
	f_MOD = p_MF->f_MOD;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));
	p_MF->p_FI = ret;

	fread(p_MF->p_Header->Name, 20, 1, f_MOD); // Get the module name, padding with NULLs
	ret->Title = (char *)malloc(p_MF->p_Header->Name[19] == 0 ? strlen(p_MF->p_Header->Name) : 21);
	if (p_MF->p_Header->Name[19] == 0)
		strcpy(ret->Title, p_MF->p_Header->Name);
	else
	{
		memcpy(ret->Title, p_MF->p_Header->Name, 20);
		ret->Title[20] = 0;
	}
	ret->BitRate = 44100;
	ret->BitsPerSample = 16;
	ret->Channels = 2;

	// Defaults
	p_MF->nSamples = 31;
	p_MF->nChannels = 4;
	// Now find out the actual values
	fseek(f_MOD, (sizeof(MODSample) * 31) + 130, SEEK_CUR);
	fread(MODMagic, 4, 1, f_MOD);
	fseek(f_MOD, 20, SEEK_SET);
	if (strncmp(MODMagic, "M.K.", 4) == 0 || strncmp(MODMagic, "M!K!", 4) == 0 ||
		strncmp(MODMagic, "M&K!", 4) == 0 || strncmp(MODMagic, "N.T.", 4) == 0)
		p_MF->nChannels = 4;
	else if (strncmp(MODMagic, "CD81", 4) == 0 || strncmp(MODMagic, "OKTA", 4) == 0)
		p_MF->nChannels = 8;
	else if (strncmp(MODMagic, "FLT", 3) == 0 && MODMagic[3] >= '4' && MODMagic[3] <= '9')
		p_MF->nChannels = MODMagic[3] - '0';
	else if (strncmp(MODMagic + 1, "CHN", 3) == 0 && MODMagic[0] >= '4' && MODMagic[0] <= '9')
		p_MF->nChannels = MODMagic[0] - '0';
	else if (strncmp(MODMagic + 2, "CH", 2) == 0 && MODMagic[0] == '1' && MODMagic[1] >= '0' && MODMagic[1] <= '9')
		p_MF->nChannels = MODMagic[1] - '0' + 10;
	else if (strncmp(MODMagic + 2, "CH", 2) == 0 && MODMagic[0] == '2' && MODMagic[1] >= '0' && MODMagic[1] <= '9')
		p_MF->nChannels = MODMagic[1] - '0' + 20;
	else if (strncmp(MODMagic + 2, "CH", 2) == 0 && MODMagic[0] == '3' && MODMagic[1] >= '0' && MODMagic[1] <= '9')
		p_MF->nChannels = MODMagic[1] - '0' + 30;
	else if (strncmp(MODMagic, "TDZ", 3) == 0 && MODMagic[3] >= '4' && MODMagic[3] <= '9')
		p_MF->nChannels = MODMagic[3] - '0';
	else if (strncmp(MODMagic, "16CN", 4) == 0)
		p_MF->nChannels = 16;
	else if (strncmp(MODMagic, "32CN", 4) == 0)
		p_MF->nChannels = 32;
	else
		p_MF->nSamples = 15;

	p_MF->p_Samples = (MODSample *)malloc(sizeof(MODSample) * p_MF->nSamples);
	fread(p_MF->p_Samples, sizeof(MODSample), p_MF->nSamples, f_MOD);
	for (i = 0; i < p_MF->nSamples; i++)
	{
		MODSample *smp = &p_MF->p_Samples[i];
		smp->Length = BE2LE(smp->Length);
		smp->LoopStart = BE2LE(smp->LoopStart);
		smp->LoopLen = BE2LE(smp->LoopLen);
		if (smp->Volume > 64)
			smp->Volume = 64;
		smp->FineTune &= 0x0F;
	}

	fread(((BYTE *)p_MF->p_Header) + 20, 130, 1, f_MOD);
	if (p_MF->nSamples != 15)
		fseek(f_MOD, 4, SEEK_CUR);
	if (p_MF->p_Header->nOrders > 128)
		p_MF->p_Header->nOrders = 128;
	if (p_MF->p_Header->RestartPos > 127)
		p_MF->p_Header->RestartPos = 127;

	// Count the number of patterns present
	for (i = 0, maxPattern = 0; i < p_MF->p_Header->nOrders; i++)
	{
		if (p_MF->p_Header->Orders[i] < 64)
		{
			maxPattern = max(maxPattern, p_MF->p_Header->Orders[i]);
		}
	}
	p_MF->nPatterns = maxPattern + 1;
	p_MF->p_Patterns = (MODPattern *)malloc(sizeof(MODPattern) * p_MF->nPatterns);
	for (i = 0; i < p_MF->nPatterns; i++)
	{
		UINT j;
		MODPattern *ptn = &p_MF->p_Patterns[i];
		ptn->Commands = (MODCommand (*)[64])malloc(sizeof(MODCommand[64]) * p_MF->nChannels);
		for (j = 0; j < 64; j++)
		{
			UINT k;
			for (k = 0; k < p_MF->nChannels; k++)
			{
				MODCommand *cmd;
				BYTE Data[4];
				// Read 4 bytes of data and unpack it into a MODCommand structure.
				cmd = &ptn->Commands[k][j];
				fread(Data, 4, 1, f_MOD);
				cmd->Sample = (Data[0] & 0xF0) | (Data[2] >> 4);
				cmd->Period = (((WORD)(Data[0] & 0x0F)) << 8) | Data[1];
				cmd->Effect = (((WORD)(Data[2] & 0x0F)) << 8) | Data[3];
			}
		}
	}

	p_MF->p_PCM = (BYTE **)malloc(sizeof(BYTE *) * p_MF->nSamples);
	for (i = 0; i < p_MF->nSamples; i++)
	{
		UINT realLength = p_MF->p_Samples[i].Length * 2;
		if (realLength != 0)
		{
			p_MF->p_PCM[i] = (BYTE *)malloc(realLength);
			fread(p_MF->p_PCM[i], realLength, 1, f_MOD);
			if (strncasecmp((char *)p_MF->p_PCM[i], "ADPCM", 5) == 0)
			{
				UINT j;
				BYTE *compressionTable = p_MF->p_PCM[i];
				BYTE *compBuffer = &p_MF->p_PCM[i][16];
				BYTE delta = 0;
				realLength -= 16;
				p_MF->p_Samples[i].Length = realLength;
				realLength *= 2;
				p_MF->p_PCM[i] = (BYTE *)malloc(realLength);
				for (j = 0; j < p_MF->p_Samples[i].Length; j++)
				{
					delta += compressionTable[compBuffer[j] & 0x0F];
					p_MF->p_PCM[i][(j * 2) + 0] = delta;
					delta += compressionTable[(compBuffer[j] >> 4) & 0x0F];
					p_MF->p_PCM[i][(j * 2) + 1] = delta;
				}
				free(compressionTable);
				compBuffer = compressionTable = NULL;
			}
		}
		else
			p_MF->p_PCM[i] = NULL;
	}

	if (ExternalPlayback == 0)
		p_MF->p_Playback = new Playback(ret, MOD_FillBuffer, p_MF->buffer, 8192, p_MODFile);
	CreateMixer(p_MF);

	return ret;
}

long MOD_FillBuffer(void *p_MODFile, BYTE *OutBuffer, int nOutBufferLen)
{
	long ret = 0, Read;
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	do
	{
		Read = FillMODBuffer(p_MF, nOutBufferLen - ret);
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
	UINT i;
	int ret = 0;
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;
	if (p_MF == NULL)
		return 0;

	delete p_MF->p_Playback;
	DestroyMixer(p_MF->p_Mixer);

	for (i = 0; i < p_MF->nSamples; i++)
		free(p_MF->p_PCM[i]);
	for (i = 0; i < p_MF->nPatterns; i++)
		free(p_MF->p_Patterns[i].Commands);
	free(p_MF->p_Header);
	free(p_MF->p_Samples);
	free(p_MF->p_PCM);
	free(p_MF->p_Patterns);
	ret = fclose(p_MF->f_MOD);
	free(p_MF);
	return ret;
}

void MOD_Play(void *p_MODFile)
{
	MOD_Intern *p_MF = (MOD_Intern *)p_MODFile;

	p_MF->p_Playback->Play();
}

bool Is_MOD(const char *FileName)
{
	FILE *f_MOD = fopen(FileName, "rb");
	CHAR MODMagic[4];
	if (f_MOD == NULL)
		return false;

	fseek(f_MOD, (sizeof(MODSample) * 31) + 150, SEEK_CUR);
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
	MOD_Play
};
