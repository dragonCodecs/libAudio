#include <sys/stat.h>
#include <sys/types.h>
#include <OptimFROG/OptimFROG.h>
#include <string.h>

#include "libAudio.h"
#include "libAudio_Common.h"

typedef struct _OFROG_Intern
{
	FILE *f_OFG;
	ReadInterface *callbacks;
	FileInfo *p_FI;
	void *p_dec;
	Playback *p_Playback;
	BYTE buffer[8192];
} OFROG_Intern;

UCHAR __fclose(void *Inst)
{
	return (fclose((FILE *)Inst) == 0 ? TRUE : FALSE);
}

int __fread(void *Inst, void *dest, UINT count)
{
	int ret = fread(dest, 1, count, (FILE *)Inst);
	int err = ferror((FILE *)Inst);

	if (err == 0)
		return ret;

	return -1;
}

UCHAR __feof(void *Inst)
{
	return (feof((FILE *)Inst) == 0 ? FALSE : TRUE);
}

UCHAR __fseekable(void *Inst)
{
	return (fseek((FILE *)Inst, 0, SEEK_CUR) == 0 ? TRUE : FALSE);
}

__int64 __flen(void *Inst)
{
	struct stat stats;

	fstat(fileno((FILE *)Inst), &stats);
	return stats.st_size;
}

__int64 __ftell(void *Inst)
{
	return ftell((FILE *)Inst);
}

UCHAR __fseek(void *Inst, __int64 pos)
{
	return (fseek((FILE *)Inst, (long)pos, SEEK_SET) == 0 ? TRUE : FALSE);
}

void *OptimFROG_OpenR(const char *FileName)
{
	OFROG_Intern *ret;
	FILE *f_OFG;

	ret = (OFROG_Intern *)malloc(sizeof(OFROG_Intern));
	if (ret == NULL)
		return ret;

	f_OFG = fopen(FileName, "rb");
	if (f_OFG == NULL)
		return f_OFG;

	ret->f_OFG = f_OFG;
	ret->p_dec = OptimFROG_createInstance();

	ret->callbacks = (ReadInterface *)malloc(sizeof(ReadInterface));
	ret->callbacks->close = __fclose;
	ret->callbacks->read = __fread;
	ret->callbacks->eof = __feof;
	ret->callbacks->seekable = __fseekable;
	ret->callbacks->length = __flen;
	ret->callbacks->getPos = __ftell;
	ret->callbacks->seek = __fseek;

	OptimFROG_openExt(ret->p_dec, ret->callbacks, ret->f_OFG, TRUE);

	return ret;
}

FileInfo *OptimFROG_GetFileInfo(void *p_OFGFile)
{
	FileInfo *ret = NULL;
	OFROG_Intern *p_OF = (OFROG_Intern *)p_OFGFile;
	OptimFROG_Info *p_OFI = (OptimFROG_Info *)malloc(sizeof(OptimFROG_Info));
	OptimFROG_Tags *p_OFT = (OptimFROG_Tags *)malloc(sizeof(OptimFROG_Tags));

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	OptimFROG_getInfo(p_OF->p_dec, p_OFI);
	ret->Channels = p_OFI->channels;
	ret->BitRate = p_OFI->samplerate;
	ret->BitsPerSample = p_OFI->bitspersample;
	ret->TotalTime = (double)p_OFI->length_ms;
	p_OF->p_FI = ret;

	/*OptimFROG_getTags(p_OF->p_dec, p_OFT);
	for (UINT i = 0; i < p_OFT->keyCount; i++)
	{
		if (p_OFT->keys[i] == "title")
		{
			ret->Title = strdup(p_OFT->values[i]);
		}
	}
	OptimFROG_freeTags(p_OFT);*/

	free(p_OFI);
	free(p_OFT);

	if (ExternalPlayback == 0)
		p_OF->p_Playback = new Playback(ret, OptimFROG_FillBuffer, p_OF->buffer, 8192, p_OFGFile);

	return ret;
}

int OptimFROG_CloseFileR(void *p_OFGFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)p_OFGFile;
	int ret = 0;

	delete p_OF->p_Playback;

	OptimFROG_close(p_OF->p_dec);
	ret = fclose(p_OF->f_OFG);

	free(p_OF->callbacks);

	return ret;
}

long OptimFROG_FillBuffer(void *p_OFGFile, BYTE *OutBuffer, int nOutBufferLen)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)p_OFGFile;
	BYTE *OBuff = OutBuffer;
	static bool eof = false;
	if (eof == true)
		return -2;

	while (OBuff - OutBuffer < nOutBufferLen && eof == false)
	{
		short *out = (short *)p_OF->buffer;
		int nSamples = (8192 / p_OF->p_FI->Channels) / (p_OF->p_FI->BitsPerSample / 8);
		int ret = OptimFROG_read(p_OF->p_dec, OBuff, nSamples, TRUE);
		if (ret < nSamples)
			eof = true;

		memcpy(OBuff, out, 8192);
		OBuff += 8192;
	}

	return (OBuff - OutBuffer);
}

void OptimFROG_Play(void *p_OFGFile)
{
	OFROG_Intern *p_OF = (OFROG_Intern *)p_OFGFile;

	p_OF->p_Playback->Play();
}

bool Is_OptimFROG(const char *FileName)
{
	FILE *f_OFG = fopen(FileName, "rb");
	char OFGSig[4];

	if (f_OFG == NULL)
		return false;

	fread(OFGSig, 4, 1, f_OFG);
	fclose(f_OFG);

	if (strncmp(OFGSig, "OFR ", 4) != 0)
		return false;

	return true;
}