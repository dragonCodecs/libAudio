#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "genericModule/genericModule.h"

modIT_t::modIT_t(/*fd_t &&fd*/) noexcept : moduleFile_t(audioType_t::moduleIT, fd_t()) { }

void *IT_OpenR(const char *FileName)
{
	IT_Intern *const ret = new (std::nothrow) IT_Intern();
	if (ret == nullptr)
		return nullptr;

	FILE *const f_IT = fopen(FileName, "rb");
	if (f_IT == nullptr)
	{
		delete ret;
		return nullptr;
	}
	ret->f_Module = f_IT;

	fileInfo_t &info = ret->inner.fileInfo();
	info.bitRate = 44100;
	info.bitsPerSample = 16;
	info.channels = 2;
	try
	{
		ret->p_File = new ModuleFile(ret);
	}
	catch (ModuleLoaderError *e)
	{
		printf("%s\n", e->GetError());
		fclose(f_IT);
		delete ret;
		return nullptr;
	}
	info.title.reset(const_cast<char *>(ret->p_File->GetTitle()));
	info.artist.reset(const_cast<char *>(ret->p_File->GetAuthor()));

	if (ExternalPlayback == 0)
		ret->p_Playback = new Playback(info, IT_FillBuffer, ret->buffer, 8192, const_cast<IT_Intern *>(ret));
	ret->p_File->InitMixer(audioFileInfo(&ret->inner));

	return ret;
}

FileInfo *IT_GetFileInfo(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	return audioFileInfo(&p_IF->inner);
	/*FileInfo *ret = NULL;

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

	return ret;*/
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

		// Read in the instruments
		for (WORD i = 0; i < p_IF->p_Head->insnum; i++)
		{
			fseek(f_IT, p_IF->p_InstOffsets[i], SEEK_SET);
			if (p_IF->p_Head->cmwt < 0x200)
				fread(&p_IF->p_OldIns[i], sizeof(ITOldInstrument), 1, f_IT);
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
	p_IF->p_Playback = nullptr;
	delete p_IF->p_File;
	p_IF->p_File = nullptr;

	ret = fclose(p_IF->f_Module);
	delete p_IF;
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
