#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <conio.h>
#include <windows.h>
#endif
#include <string.h>

#include <neaacdec.h>
#include <mp4ff.h>

#include "libAudio.h"
#include "libAudio_Common.h"

typedef struct _M4A_Intern
{
	FILE *f_M4A;
	NeAACDecHandle p_dec;
	mp4ff_t *p_mp4;
	FileInfo *p_FI;
	int nTrack, nLoops, nCurrLoop;
	bool eof;
	mp4ff_callback_t *callbacks;
	BYTE buffer[8192];
	Playback *p_Playback;
} M4A_Intern;

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

void *M4A_OpenR(char *FileName)
{
	M4A_Intern *ret = NULL;
	FILE *f_M4A = NULL;
	mp4ff_callback_t *callbacks = (mp4ff_callback_t*)malloc(sizeof(mp4ff_callback_t));
	if (callbacks == NULL)
		return ret;

	ret = (M4A_Intern *)malloc(sizeof(M4A_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(M4A_Intern));

	f_M4A = ret->f_M4A = fopen(FileName, "rb");
	if (f_M4A == NULL)
	{
		free(ret);
		return f_M4A;
	}

	memset(callbacks, 0x00, sizeof(*callbacks));
	callbacks->read = f_fread;
	callbacks->seek = f_fseek;
	callbacks->user_data = f_M4A;
	ret->callbacks = callbacks;

	ret->eof = false;
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

FileInfo *M4A_GetFileInfo(void *p_M4AFile)
{
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;
	FileInfo *ret = NULL;
	char *value;
	UCHAR *Buff = NULL;
	UINT nBuffLen = 0;
	NeAACDecConfiguration *ADC;

	p_MF->p_FI = ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	mp4ff_get_decoder_config(p_MF->p_mp4, p_MF->nTrack, &Buff, &nBuffLen);
	NeAACDecInit2(p_MF->p_dec, Buff, nBuffLen, (ULONG *)&ret->BitRate, (UCHAR *)&ret->Channels);
	ADC = NeAACDecGetCurrentConfiguration(p_MF->p_dec);

	mp4ff_meta_get_album(p_MF->p_mp4, &ret->Album);
	mp4ff_meta_get_artist(p_MF->p_mp4, &ret->Artist);
	mp4ff_meta_get_title(p_MF->p_mp4, &ret->Title);
	//ret->BitRate = mp4ff_get_avg_bitrate(p_MF->p_mp4, p_MF->nTrack);
	//ret->BitsPerSample = InterpretFormat(ADC->outputFormat);
	ret->BitsPerSample = 16;
	mp4ff_meta_get_comment(p_MF->p_mp4, &value);
	if (value == NULL)
		ret->nOtherComments = 0;
	else
	{
		ret->nOtherComments = 1;
		ret->OtherComments.push_back(value);
	}
	p_MF->nLoops = mp4ff_num_samples(p_MF->p_mp4, p_MF->nTrack);
	p_MF->nCurrLoop = 0;

	if (ExternalPlayback == 0)
		p_MF->p_Playback = new Playback(ret, M4A_FillBuffer, p_MF->buffer, 8192, p_M4AFile);
	ADC->outputFormat = FAAD_FMT_16BIT;
	NeAACDecSetConfiguration(p_MF->p_dec, ADC);

	return ret;
}

int M4A_CloseFileR(void *p_M4AFile)
{
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;
	FILE *f_M4A = p_MF->f_M4A;
	int ret;

	delete p_MF->p_Playback;

	NeAACDecClose(p_MF->p_dec);
	mp4ff_close(p_MF->p_mp4);

	ret = fclose(f_M4A);
	free(p_MF);
	return ret;
}

long M4A_FillBuffer(void *p_M4AFile, BYTE *OutBuffer, int nOutBufferLen)
{
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;
	FILE *f_M4A = p_MF->f_M4A;
	BYTE *OBuf = OutBuffer;

	while ((OBuf - OutBuffer) < nOutBufferLen && p_MF->eof == false)
	{
		if (p_MF->nCurrLoop < p_MF->nLoops)
		{
			int i = 0;
			BYTE *Buff = NULL;
			UINT nBuff = 0;
			NeAACDecFrameInfo FI;
			static BYTE *Buff2 = NULL;

			if (mp4ff_read_sample(p_MF->p_mp4, p_MF->nTrack, p_MF->nCurrLoop, &Buff, &nBuff) == 0)
			{
				p_MF->eof = true;
				return -2;
			}

			Buff2 = (BYTE *)NeAACDecDecode(p_MF->p_dec, &FI, Buff, nBuff);
			free(Buff);
			p_MF->nCurrLoop++;

			if (FI.error != 0)
			{
				printf("Error: %s\n", NeAACDecGetErrorMessage(FI.error));
				/*printf("\nPress Any Key To Continue....\n");
				getch();
				exit(FI.error);*/
				continue;
			}

			memcpy(OBuf, Buff2, (FI.samples * (p_MF->p_FI->BitsPerSample / 8)));
			OBuf += (FI.samples * (p_MF->p_FI->BitsPerSample / 8));
//			free(Buff2);
		}
		else if (p_MF->nCurrLoop == p_MF->nLoops)
			return -1;
	}

	return OBuf - OutBuffer;
}

void M4A_Play(void *p_M4AFile)
{
	M4A_Intern *p_MF = (M4A_Intern *)p_M4AFile;

	p_MF->p_Playback->Play();
}

// Standard "ftyp" Atom for a MOV based MP4 AAC file:
// 00 00 00 20 66 74 79 70 4D 34 41 20 
// .  .  .     f  t  y  p  M  4  A     

bool Is_M4A(char *FileName)
{
	FILE *f_M4A = fopen(FileName, "rb");
	CHAR Len[4];
	char TypeSig[4];
	char FileType[4];

	if (f_M4A == NULL)
		return false;

	// How to deal with endieness:
	fread(&Len, 4, 1, f_M4A);
	//if (Len[3] != 32)
	//	return false;

	fread(TypeSig, 4, 1, f_M4A);
	fread(FileType, 4, 1, f_M4A);
	fclose(f_M4A);

	if (strncmp(TypeSig, "ftyp", 4) != 0 ||
		(strncmp(FileType, "M4A ", 4) != 0 &&
		strncmp(FileType, "mp42", 4) != 0))
		return false;

	return true;
}
