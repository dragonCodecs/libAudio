#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

void *IT_OpenR(const char *FileName)
{
	IT_Intern *ret = NULL;
	FILE *f_IT = NULL;

	ret = (IT_Intern *)malloc(sizeof(IT_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(IT_Intern));

	f_IT = fopen(FileName, "rb");
	if (f_IT == NULL)
		return f_IT;
	ret->f_IT = f_IT;

	return ret;
}

FileInfo *IT_GetFileInfo(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	FileInfo *ret = NULL;

	if (p_IF == NULL)
		return ret;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));
	p_IF->p_FI = ret;

	ret->BitRate = 44100;
	ret->BitsPerSample = 16;
	ret->Channels = 2;
	try
	{
		p_IF->p_File = new ModuleFile(p_IF);
	}
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->GetError());
		return NULL;
	}
	ret->Title = p_IF->p_File->GetTitle();

	if (ExternalPlayback == 0)
		p_IF->p_Playback = new Playback(ret, IT_FillBuffer, p_IF->buffer, 8192, p_ITFile);
	p_IF->p_File->InitMixer(ret);

	return ret;
}

#if 0
FileInfo *IT_GetFileInfo_(void *p_ITFile)
{
	char ID[4];

	{
		// Discardable extra:
		uint16_t len;
		char *Unknown;
		fread(&len, 2, 1, f_IT);
		Unknown = (char *)malloc(len);
		fread(Unknown, len, 1, f_IT);
		free(Unknown);
	}

	if ((p_IF->p_Head->flags & 0x80) != 0)
	{
		p_IF->p_MidiCfg = (MidiConfig *)malloc(sizeof(MidiConfig));
		fread(&p_IF->p_MidiCfg, sizeof(MidiConfig), 1, f_IT);
	}

	fread(ID, 4, 1, f_IT);
	if (ID == "PNAM")
	{
		fread(&p_IF->PatName.nNames, 4, 1, f_IT);
		p_IF->PatName.nNames /= 20;
		p_IF->PatName.p_Names = (BYTE **)malloc(sizeof(BYTE *) * p_IF->PatName.nNames);
		for (UINT i = 0; i < p_IF->PatName.nNames; i++)
		{
			p_IF->PatName.p_Names[i] = (BYTE *)malloc(20);
			fread(p_IF->PatName.p_Names[i], 20, 1, f_IT);
		}
	}
	else
		fseek(f_IT, -4, SEEK_CUR);

	fread(ID, 4, 1, f_IT);
	if (ID == "CNAME")
	{
		fread(&p_IF->ChanName.nNames, 4, 1, f_IT);
		p_IF->ChanName.nNames /= 20;
		p_IF->ChanName.p_Names = (BYTE **)malloc(sizeof(BYTE *) * p_IF->ChanName.nNames);
		for (UINT i = 0; i < p_IF->ChanName.nNames; i++)
		{
			p_IF->ChanName.p_Names[i] = (BYTE *)malloc(20);
			fread(p_IF->ChanName.p_Names[i], 20, 1, f_IT);
		}
	}
	else
		fseek(f_IT, -4, SEEK_CUR);

	if (p_IF->p_Head->insnum != 0)
	{
		// Allocate enough memory
		if (p_IF->p_Head->cmwt < 0x200)
			p_IF->p_OldIns = (ITOldInstrument *)malloc(sizeof(ITOldInstrument) * p_IF->p_Head->insnum);
		else
			p_IF->p_Ins = (ITInstrument *)malloc(sizeof(ITInstrument) * p_IF->p_Head->insnum);

		// Read in the instruments
		for (WORD i = 0; i < p_IF->p_Head->insnum; i++)
		{
			fseek(f_IT, p_IF->p_InstOffsets[i], SEEK_SET);
			if (p_IF->p_Head->cmwt < 0x200)
				fread(&p_IF->p_OldIns[i], sizeof(ITOldInstrument), 1, f_IT);
			else
				fread(&p_IF->p_Ins[i], sizeof(ITInstrument), 1, f_IT);
		}
	}

	// This code works flawlessly on a properly encoded/saved
	// .IT file. MPT Compatability files from the current release
	// do not follow the standard properly, so this messes up.
	for (int i = 0; i < 64; i++)
	{
		if (p_IF->p_Head->chnpan[i] > 128)
		{
			p_IF->nChannels = i;
			break;
		}
	}
	if (p_IF->nChannels == 0)
		p_IF->nChannels = 64;
	if (p_IF->nChannels < 4)
		p_IF->nChannels = 4;

	if (p_IF->p_Head->patnum != 0)
	{

		for (UINT i = 0; i < p_IF->p_Head->patnum; i++)
		{
			BYTE b = 0, ChanMask[64];
			WORD Rows, Len;
			WORD nRows = 0, j = 0;
			Command LastVal[64];
			Command *Commands = NULL;

			while (nRows < Rows)
			{
				int nChan = 0;
				// Check for a normal volume command
				if ((ChanMask[nChan] & 4) != 0)
				{
					BYTE vol = 0;
					if (j >= Len)
						break;
					fread(&vol, 1, 1, f_IT);
					j++;

					if (nChan < p_IF->nChannels)
					{
						if (vol <= 64)
						{
							Commands[nPatPos].vol = vol;
							Commands[nPatPos].volcmd = VOLCMD_VOLUME;
						}
						else if (vol <= 74)
						{
							Commands[nPatPos].vol = vol - 65;
							Commands[nPatPos].volcmd = VOLCMD_FINEVOLUP;
						}
						else if (vol <= 84)
						{
							Commands[nPatPos].vol = vol - 75;
							Commands[nPatPos].volcmd = VOLCMD_FINEVOLDOWN;
						}
						else if (vol <= 94)
						{
							Commands[nPatPos].vol = vol - 85;
							Commands[nPatPos].volcmd = VOLCMD_VOLSLIDEUP;
						}
						else if (vol <= 104)
						{
							Commands[nPatPos].vol = vol - 95;
							Commands[nPatPos].volcmd = VOLCMD_VOLSLIDEDOWN;
						}
						else if (vol <= 114)
						{
							Commands[nPatPos].vol = vol - 105;
							Commands[nPatPos].volcmd = VOLCMD_PITCHDOWN;
						}
						else if (vol <= 124)
						{
							Commands[nPatPos].vol = vol - 115;
							Commands[nPatPos].volcmd = VOLCMD_PITCHUP;
						}
						else if (vol >= 128 && vol <= 192)
						{
							Commands[nPatPos].vol = vol - 128;
							Commands[nPatPos].volcmd = VOLCMD_PANNING;
						}
						else if (vol >= 193 && vol <= 202)
						{
							Commands[nPatPos].vol = vol - 193;
							Commands[nPatPos].volcmd = VOLCMD_PORTATO;
						}
						else if (vol >= 203 && vol <= 212)
						{
							Commands[nPatPos].vol = vol - 203;
							Commands[nPatPos].volcmd = VOLCMD_VIBRATO;
						}
					}
				}
			}
		}
	}
}
#endif

long IT_FillBuffer(void *p_ITFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	int32_t ret = 0, Read;
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	if (p_IF->p_File == NULL)
		return -1;
	do
	{
		Read = p_IF->p_File->Mix(p_IF->buffer, nOutBufferLen - ret);
		if (Read >= 0 && OutBuffer != p_IF->buffer)
			memcpy(OutBuffer + ret, p_IF->buffer, Read);
		if (Read >= 0)
			ret += Read;
	}
	while (ret < nOutBufferLen && Read >= 0);
	return (ret == 0 ? Read : ret);
}

int IT_CloseFileR(void *p_ITFile)
{
	int ret = 0;
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	if (p_IF == NULL)
		return 0;

	delete p_IF->p_Playback;
	delete p_IF->p_File;

	ret = fclose(p_IF->f_IT);
	free(p_IF);
	return ret;
}

void IT_Play(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;

	p_IF->p_Playback->Play();
}

void IT_Pause(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;

	p_IF->p_Playback->Pause();
}

void IT_Stop(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;

	p_IF->p_Playback->Stop();
}

bool Is_IT(const char *FileName)
{
	FILE *f_IT = fopen(FileName, "rb");
	char ITSig[4];
	if (f_IT == NULL)
		return false;

	fread(ITSig, 4, 1, f_IT);
	fclose(f_IT);

	if (strncmp(ITSig, "IMPM", 4) != 0)
		return false;

	return true;
}

API_Functions ITDecoder =
{
	IT_OpenR,
	IT_GetFileInfo,
	IT_FillBuffer,
	IT_CloseFileR,
	IT_Play,
	IT_Pause,
	IT_Stop
};
