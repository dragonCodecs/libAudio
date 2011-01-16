#include <sys/stat.h>
#include <sys/types.h>
#include <wavpack/wavpack.h>
#include <string.h>
#include <string>

#include "libAudio.h"
#include "libAudio_Common.h"

typedef struct _WavPack_Intern
{
	WavpackStreamReader *callbacks;
	WavpackContext *p_dec;
	FILE *f_WVP;
	FILE *f_WVPC;
	FileInfo *p_FI;
	BYTE buffer[8192];
	int middlebuff[4096];
	Playback *p_Playback;
	char err[80];
	bool eof;
} WavPack_Intern;

int f_fread_wp(void *p_file, void *data, int size)
{
	return fread(data, 1, size, (FILE *)p_file);
}

UINT f_ftell(void *p_file)
{
	return ftell((FILE *)p_file);
}

int f_fseek_abs(void *p_file, UINT pos)
{
	return fseek((FILE *)p_file, pos, SEEK_SET);
}

int f_fseek_rel(void *p_file, int pos, int loc)
{
	return fseek((FILE *)p_file, pos, loc);
}

int f_fungetc(void *p_file, int c)
{
	return ungetc(c, (FILE *)p_file);
}

UINT f_flen(void *p_file)
{
	struct stat stats;

	fstat(fileno((FILE *)p_file), &stats);
	return stats.st_size;
}

int f_fcanseek(void *p_file)
{
	return (fseek((FILE *)p_file, 0, SEEK_CUR) == 0 ? TRUE : FALSE);
}

void *WavPack_OpenR(const char *FileName)
{
	WavPack_Intern *ret = NULL;
	FILE *f_WVP = NULL, *f_WVPC = NULL;

	ret = (WavPack_Intern *)malloc(sizeof(WavPack_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(WavPack_Intern));

	f_WVP = fopen(FileName, "rb");
	if (f_WVP == NULL)
		return f_WVP;

	// If there's a correction file, open it.
	// This is in effect a function, hence it's in it's own code block.
	{
		std::string fname = std::string(FileName);
		fname.append("c");
		f_WVPC = fopen(fname.c_str(), "rb");
		fname.clear();
	}

	ret->f_WVP = f_WVP;
	ret->f_WVPC = f_WVPC;
	ret->callbacks = (WavpackStreamReader *)malloc(sizeof(WavpackStreamReader));
	ret->callbacks->read_bytes = f_fread_wp;
	ret->callbacks->get_pos = f_ftell;
	ret->callbacks->set_pos_abs = f_fseek_abs;
	ret->callbacks->set_pos_rel = f_fseek_rel;
	ret->callbacks->push_back_byte = f_fungetc;
	ret->callbacks->get_length = f_flen;
	ret->callbacks->can_seek = f_fcanseek;
	ret->callbacks->write_bytes = NULL;
	ret->p_dec = WavpackOpenFileInputEx(ret->callbacks, f_WVP, f_WVPC, ret->err, OPEN_NORMALIZE | OPEN_TAGS, 15);

	return ret;
}

FileInfo *WavPack_GetFileInfo(void *p_WVPFile)
{
	FileInfo *ret = NULL;
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	p_WF->p_FI = ret;
	ret->Channels = WavpackGetNumChannels(p_WF->p_dec);
	ret->BitsPerSample = WavpackGetBitsPerSample(p_WF->p_dec);
	ret->BitRate = WavpackGetSampleRate(p_WF->p_dec);

	{
		int i = WavpackGetTagItem(p_WF->p_dec, "album", NULL, 0) + 1;
		if (i > 1)
		{
			ret->Album = (char *)malloc(i);
			WavpackGetTagItem(p_WF->p_dec, "album", ret->Album, i);
		}
	}
	{
		int i = WavpackGetTagItem(p_WF->p_dec, "artist", NULL, 0) + 1;
		if (i > 1)
		{
			ret->Artist = (char *)malloc(i);
			WavpackGetTagItem(p_WF->p_dec, "artist", ret->Artist, i);
		}
	}
	{
		int i = WavpackGetTagItem(p_WF->p_dec, "title", NULL, 0) + 1;
		if (i > 1)
		{
			ret->Title = (char *)malloc(i);
			WavpackGetTagItem(p_WF->p_dec, "title", ret->Title, i);
		}
	}

	if (ExternalPlayback == 0)
		p_WF->p_Playback = new Playback(ret, WavPack_FillBuffer, p_WF->buffer, 8192, p_WVPFile);

	return ret;
}

int WavPack_CloseFileR(void *p_WVPFile)
{
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;
	int ret = 0;

	delete p_WF->p_Playback;

	WavpackCloseFile(p_WF->p_dec);

	if (p_WF->f_WVPC != NULL)
		fclose(p_WF->f_WVPC);

	ret = fclose(p_WF->f_WVP);

	free(p_WF->callbacks);
	free(p_WF);

	return ret;
}

long WavPack_FillBuffer(void *p_WVPFile, BYTE *OutBuffer, int nOutBufferLen)
{
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;
	BYTE *OBuff = OutBuffer;
	if (p_WF->eof == true)
		return -2;

	while (OBuff - OutBuffer < nOutBufferLen && p_WF->eof == false)
	{
		short *out = (short *)p_WF->buffer;
		int j = WavpackUnpackSamples(p_WF->p_dec, p_WF->middlebuff, (4096 / p_WF->p_FI->Channels)) * p_WF->p_FI->Channels;

		if (j == 0)
			p_WF->eof = true;

		for (int i = 0; i < j; )
		{
			for (int k = 0; k < p_WF->p_FI->Channels; k++)
			{
				out[i + k] = (short)p_WF->middlebuff[i + k];
			}

			i += p_WF->p_FI->Channels;
		}

		memcpy(OBuff, out, j * 2);
		OBuff += j * 2;
	}

	return (OBuff - OutBuffer);
}

void WavPack_Play(void *p_WVPFile)
{
	WavPack_Intern *p_WF = (WavPack_Intern *)p_WVPFile;

	p_WF->p_Playback->Play();
}

bool Is_WavPack(const char *FileName)
{
	FILE *f_WVP = fopen(FileName, "rb");
	char WavPackSig[4];

	if (f_WVP == NULL)
		return false;

	fread(WavPackSig, 4, 1, f_WVP);
	fclose(f_WVP);

	if (strncmp(WavPackSig, "wvpk", 4) != 0)
		return false;

	return true;
}

API_Functions WavPackDecoder =
{
	WavPack_OpenR,
	WavPack_GetFileInfo,
	WavPack_FillBuffer,
	WavPack_CloseFileR,
	WavPack_Play
};

