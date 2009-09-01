#include <stdio.h>
#include <conio.h>
#include <malloc.h>
#include <windows.h>

#include <al.h>
#include <alc.h>

#include <neaacdec.h>
#include <mp4ff.h>

#include "libAudio.h"
#include "libAudio_Common.h"

typedef struct _AAC_Intern
{
	FILE *f_AAC;
	NeAACDecHandle p_dec;
	mp4ff_t *p_mp4;
	FileInfo *p_FI;
	int nTrack, nLoops, nCurrLoop;
	mp4ff_callback_t *callbacks;
	BYTE buffer[8192];
	Playback *p_Playback;
} AAC_Intern;

int GetAACTrack(mp4ff_t *infile)
{
    /* find AAC track */
    int i, rc;
    int numTracks = mp4ff_total_tracks(infile);

    for (i = 0; i < numTracks; i++)
    {
        UCHAR *buff = NULL;
        UINT buff_size = 0;
        mp4AudioSpecificConfig mp4ASC;

        mp4ff_get_decoder_config(infile, i, &buff, &buff_size);

        if (buff != NULL)
        {
            rc = NeAACDecAudioSpecificConfig(buff, buff_size, &mp4ASC);
            free(buff);

            if (rc < 0)
                continue;
            return i;
        }
    }

    /* can't decode this */
    return -1;
}

uint32_t f_fread(void *UserData, void *Buffer, uint32_t Length)
{
	return fread(Buffer, 1, Length, (FILE *)UserData);
}

uint32_t f_fseek(void *UserData, uint64_t Pos)
{
	return fseek((FILE *)UserData, (long)Pos, SEEK_SET);
}

void *AAC_OpenR(char *FileName)
{
	AAC_Intern *ret = NULL;
	FILE *f_AAC = NULL;
	mp4ff_callback_t *callbacks = (mp4ff_callback_t*)malloc(sizeof(mp4ff_callback_t));
	if (callbacks == NULL)
		return ret;

	ret = (AAC_Intern *)malloc(sizeof(AAC_Intern));
	if (ret == NULL)
		return ret;

	f_AAC = ret->f_AAC = fopen(FileName, "rb");
	if (f_AAC == NULL)
		return NULL;

	memset(callbacks, 0x00, sizeof(*callbacks));
	callbacks->read = f_fread;
	callbacks->seek = f_fseek;
	callbacks->user_data = f_AAC;
	ret->callbacks = callbacks;

	ret->p_dec = NeAACDecOpen();
	ret->p_mp4 = mp4ff_open_read(callbacks);
	ret->nTrack = GetAACTrack(ret->p_mp4);

	return ret;
}

/*long InterpretFormat(BYTE oF)
{
	switch (oF)
	{
		case FAAD_FMT_16BIT:
			return 16;
		case FAAD_FMT_24BIT:
			return 24;
		case FAAD_FMT_32BIT:
			return 32;
		case FAAD_FMT_DOUBLE:
			return (sizeof(double) * 8);
		case FAAD_FMT_FLOAT:
			return (sizeof(float) * 8);

		default:
			return 0;
	}
}*/

FileInfo *AAC_GetFileInfo(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	FileInfo *ret = NULL;
	UCHAR *Buff = NULL;
	UINT nBuffLen = 0;
	NeAACDecConfiguration *ADC;

	p_AF->p_FI = ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	mp4ff_get_decoder_config(p_AF->p_mp4, p_AF->nTrack, &Buff, &nBuffLen);
	NeAACDecInit2(p_AF->p_dec, Buff, nBuffLen, (ULONG *)&ret->BitRate, (UCHAR *)&ret->Channels);
	ADC = NeAACDecGetCurrentConfiguration(p_AF->p_dec);

	mp4ff_meta_get_album(p_AF->p_mp4, &ret->Album);
	mp4ff_meta_get_artist(p_AF->p_mp4, &ret->Artist);
	mp4ff_meta_get_title(p_AF->p_mp4, &ret->Title);
	//ret->BitRate = mp4ff_get_avg_bitrate(p_AF->p_mp4, p_AF->nTrack);
	//ret->BitsPerSample = InterpretFormat(ADC->outputFormat);
	ret->BitsPerSample = 16;
	ret->OtherComments.push_back("");
	mp4ff_meta_get_comment(p_AF->p_mp4, &ret->OtherComments[0]);
	ret->nOtherComments = (ret->OtherComments[0] == NULL ? 0 : 1);
	p_AF->nLoops = mp4ff_num_samples(p_AF->p_mp4, p_AF->nTrack);
	p_AF->nCurrLoop = 0;

	p_AF->p_Playback = new Playback(ret, AAC_FillBuffer, p_AF->buffer, 8192, p_AACFile);
	ADC->outputFormat = FAAD_FMT_16BIT;
	NeAACDecSetConfiguration(p_AF->p_dec, ADC);

	return ret;
}

int AAC_CloseFileR(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	FILE *f_AAC = p_AF->f_AAC;
	int ret;

	delete p_AF->p_Playback;

	NeAACDecClose(p_AF->p_dec);
	mp4ff_close(p_AF->p_mp4);

	ret = fclose(f_AAC);
	free(p_AF);
	return ret;
}

long AAC_FillBuffer(void *p_AACFile, BYTE *OutBuffer, int nOutBufferLen)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;
	FILE *f_AAC = p_AF->f_AAC;
	BYTE *OBuf = OutBuffer;
	static bool eof = false;

	while ((OBuf - OutBuffer) < nOutBufferLen && eof == false)
	{
		if (p_AF->nCurrLoop < p_AF->nLoops)
		{
			int i = 0;
			UCHAR *Buff = NULL;
			UINT nBuff = 0;
			NeAACDecFrameInfo FI;
			static BYTE *Buff2 = NULL;

			if (mp4ff_read_sample(p_AF->p_mp4, p_AF->nTrack, p_AF->nCurrLoop, &Buff, &nBuff) == 0)
			{
				eof = true;
				return -2;
			}

			Buff2 = (BYTE *)NeAACDecDecode(p_AF->p_dec, &FI, Buff, nBuff);
			free(Buff);
			p_AF->nCurrLoop++;

			if (FI.error != 0)
			{
				printf("Error: %s\n", NeAACDecGetErrorMessage(FI.error));
				/*printf("\nPress Any Key To Continue....\n");
				getch();
				exit(FI.error);*/
				continue;
			}

			memcpy(OBuf, Buff2, (FI.samples * (p_AF->p_FI->BitsPerSample / 8)));
			OBuf += (FI.samples * (p_AF->p_FI->BitsPerSample / 8));
//			free(Buff2);
		}
		else if (p_AF->nCurrLoop == p_AF->nLoops)
			return -1;
	}

	return OBuf - OutBuffer;
}

void AAC_Play(void *p_AACFile)
{
	AAC_Intern *p_AF = (AAC_Intern *)p_AACFile;

	p_AF->p_Playback->Play();
}

// Standard "ftyp" Atom for a MOV based MP4 AAC file:
// 00 00 00 20 66 74 79 70 4D 34 41 20 
// .  .  .     f  t  y  p  M  4  A     

bool Is_AAC(char *FileName)
{
	FILE *f_AAC = fopen(FileName, "rb");
	CHAR Len[4];
	char TypeSig[4];
	char FileType[4];

	if (f_AAC == NULL)
		return false;

	// How to deal with endieness:
	fread(&Len, 4, 1, f_AAC);
	//if (Len[3] != 32)
	//	return false;

	fread(TypeSig, 4, 1, f_AAC);
	fread(FileType, 4, 1, f_AAC);
	fclose(f_AAC);

	if (strncmp(TypeSig, "ftyp", 4) != 0 ||
		(strncmp(FileType, "M4A ", 4) != 0 &&
		strncmp(FileType, "mp42", 4) != 0))
		return false;

	return true;
}