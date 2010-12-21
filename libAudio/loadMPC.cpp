#ifndef __NO_MPC__
#include <sys/stat.h>
#include <sys/types.h>
#include <malloc.h>
#include <string.h>

#include <MusePack/mpcdec.h>

#include "libAudio.h"
#include "libAudio_Common.h"

#define SHORT_MAX 0x7FFF

typedef struct _MPC_Intern
{
	FILE *f_MPC;
	Playback *p_Playback;
	BYTE buffer[8192];
	mpc_reader *callbacks;
	mpc_streaminfo *info;
	mpc_demux *demuxer;
	FileInfo *p_FI;
	mpc_frame_info *frame;
	MPC_SAMPLE_FORMAT framebuff[MPC_DECODER_BUFFER_LENGTH];
	int PCMUsed;
} MPC_Intern;

short FloatToShort(MPC_SAMPLE_FORMAT Sample)
{
	float s = (Sample * SHORT_MAX);
//	static float max_clip = SHORT_MAX, min_clip = -max_clip;//-1 << 15;

	// Perform cliping
	if (s < -SHORT_MAX)
		s = -SHORT_MAX;
	if (s > SHORT_MAX)
		s = SHORT_MAX;

	return (short)s;
}

INT f_fread(mpc_reader *p_MFCFile, void *p_Buffer, int size)
{
	MPC_Intern *p_MF = (MPC_Intern *)(p_MFCFile->data);
	return fread(p_Buffer, 1, size, p_MF->f_MPC);
}

UCHAR f_fseek(mpc_reader *p_MFCFile, int offset)
{
	MPC_Intern *p_MF = (MPC_Intern *)(p_MFCFile->data);
	return (fseek(p_MF->f_MPC, offset, SEEK_SET) == 0 ? TRUE : FALSE);
}

int f_ftell(mpc_reader *p_MFCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)(p_MFCFile->data);
	return ftell(p_MF->f_MPC);
}

int f_flen(mpc_reader *p_MFCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)(p_MFCFile->data);
	struct stat stats;

	fstat(fileno(p_MF->f_MPC), &stats);
	return stats.st_size;
}

UCHAR f_fcanseek(mpc_reader *p_MFCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)(p_MFCFile->data);

	return (fseek(p_MF->f_MPC, 0, SEEK_CUR) == 0 ? TRUE : FALSE);
}

void *MPC_OpenR(const char *FileName)
{
	MPC_Intern *ret = NULL;
	FILE *f_MPC = NULL;

	f_MPC = fopen(FileName, "rb");
	if (f_MPC == NULL)
		return ret;

	ret = (MPC_Intern *)malloc(sizeof(MPC_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(MPC_Intern));

	ret->f_MPC = f_MPC;
	ret->callbacks = (mpc_reader *)malloc(sizeof(mpc_reader));
	ret->callbacks->read = f_fread;
	ret->callbacks->seek = f_fseek;
	ret->callbacks->tell = f_ftell;
	ret->callbacks->get_size = f_flen;
	ret->callbacks->canseek = f_fcanseek;
	ret->callbacks->data = ret;
	ret->demuxer = mpc_demux_init(ret->callbacks);

	ret->info = (mpc_streaminfo *)malloc(sizeof(mpc_streaminfo));
	ret->frame = (mpc_frame_info *)malloc(sizeof(mpc_frame_info));
	ret->frame->buffer = ret->framebuff;

	return ret;
}

FileInfo *MPC_GetFileInfo(void *p_MFCFile)
{
	FileInfo *ret = NULL;
	MPC_Intern *p_MF = (MPC_Intern *)p_MFCFile;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	memset(ret, 0x00, sizeof(FileInfo));

	mpc_demux_get_info(p_MF->demuxer, p_MF->info);

	p_MF->p_FI = ret;
	ret->BitsPerSample = (p_MF->info->bitrate == 0 ? 16 : p_MF->info->bitrate);
	ret->BitRate = p_MF->info->sample_freq;
	ret->Channels = p_MF->info->channels;

	if (ExternalPlayback == 0)
		p_MF->p_Playback = new Playback(ret, MPC_FillBuffer, p_MF->buffer, 8192, p_MFCFile);

	return ret;
}

long MPC_FillBuffer(void *p_MFCFile, BYTE *OutBuffer, int nOutBufferLen)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MFCFile;
	BYTE *OBuff = OutBuffer;

	while (OBuff - OutBuffer < nOutBufferLen)
	{
		short *out = (short *)(p_MF->buffer + (OBuff - OutBuffer));
		int nOut = 0;

		if (p_MF->PCMUsed == 0)
		{
			if (mpc_demux_decode(p_MF->demuxer, p_MF->frame) != 0 || p_MF->frame->bits == -1)
				return -2;
		}

		for (int i = p_MF->PCMUsed; i < p_MF->frame->samples; i++)
		{
			short Sample = 0;

			if (p_MF->p_FI->Channels == 1)
			{
				Sample = FloatToShort(p_MF->frame->buffer[i]);
				out[(nOut / 2)] = Sample;
				nOut += 2;
			}
			else if (p_MF->p_FI->Channels == 2)
			{
				Sample = FloatToShort(p_MF->frame->buffer[(i * 2) + 0]);
				out[(nOut / 2) + 0] = Sample;

				Sample = FloatToShort(p_MF->frame->buffer[(i * 2) + 1]);
				out[(nOut / 2) + 1] = Sample;
				nOut += 4;
			}

			if (((OBuff - OutBuffer) + nOut) >= nOutBufferLen)
			{
				p_MF->PCMUsed = ++i;
				i = p_MF->frame->samples;
			}
		}

		if (((OBuff - OutBuffer) + nOut) < nOutBufferLen)
		{
			p_MF->PCMUsed = 0;
		}

		memcpy(OBuff, out, nOut);
		OBuff += nOut;
	}

	return OBuff - OutBuffer;
}

int MPC_CloseFileR(void *p_MFCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MFCFile;
	int ret;

	delete p_MF->p_Playback;

	mpc_demux_exit(p_MF->demuxer);
	free(p_MF->info);
	free(p_MF->frame);
	free(p_MF->callbacks);

	ret = fclose(p_MF->f_MPC);
	free(p_MF);
	return ret;
}

void MPC_Play(void *p_MFCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MFCFile;

	p_MF->p_Playback->Play();
}

bool Is_MPC(const char *FileName)
{
	FILE *f_MPC = fopen(FileName, "rb");
	char MPCSig[3];

	if (f_MPC == NULL)
		return false;

	fread(MPCSig, 3, 1, f_MPC);
	fclose(f_MPC);

	if (strncmp(MPCSig, "MP+", 3) != 0 &&
		strncmp(MPCSig, "MPC", 3) != 0)
		return false;

	return true;
}
#endif /* __NO_MPC__ */
