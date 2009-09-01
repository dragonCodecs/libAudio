#include <sys/stat.h>
#include <sys/types.h>

#include <MusePack\mpcdec.h>

#include "libAudio.h"
#include "libAudio_Common.h"

typedef struct _MPC_Intern
{
	FILE *f_MPC;
	Playback *p_Playback;
	BYTE buffer[8192];
	mpc_reader *callbacks;
	mpc_streaminfo *info;
	mpc_decoder *decoder;
	FileInfo *p_FI;
	MPC_SAMPLE_FORMAT framebuff[MPC_DECODER_BUFFER_LENGTH];
	int PCMUsed;
	FILE *f_Out;
} MPC_Intern;

short FloatToShort(MPC_SAMPLE_FORMAT Sample)
{
	float s = (Sample * (1 << 15));
	static float max_clip = (1 << 15), min_clip = -max_clip;//-1 << 15;
	//static char r[4];

	// Perform cliping
	if (s < min_clip)
		s = min_clip;
	if (s > max_clip)
		s = max_clip;

	// Re-order the output for little-endian
	/*r[0] = (s >> 0) & 0xFF;
	r[1] = (s >> 8) & 0xFF;
	r[2] = (s >> 16) & 0xFF;
	r[3] = (s >> 24) & 0xFF;

	return ((int *)r)[0];*/
	return (short)s;
}

int f_fread(void *p_MPCFile, void *p_Buffer, int size)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	return fread(p_Buffer, 1, size, p_MF->f_MPC);
}

UCHAR f_fseek(void *p_MPCFile, int offset)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	return (fseek(p_MF->f_MPC, offset, SEEK_SET) == 0 ? TRUE : FALSE);
}

int f_ftell(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	return ftell(p_MF->f_MPC);
}

int f_flen(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	struct stat stats;

	fstat(fileno(p_MF->f_MPC), &stats);
	return stats.st_size;
}

UCHAR f_fcanseek(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;

	return (fseek(p_MF->f_MPC, 0, SEEK_CUR) == 0 ? TRUE : FALSE);
}

void *MPC_OpenR(char *FileName)
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

	ret->info = (mpc_streaminfo *)malloc(sizeof(mpc_streaminfo));
	ret->decoder = (mpc_decoder *)malloc(sizeof(mpc_decoder));

	return ret;
}

FileInfo *MPC_GetFileInfo(void *p_MPCFile)
{
	FileInfo *ret = NULL;
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	memset(ret, 0x00, sizeof(FileInfo));

	mpc_streaminfo_init(p_MF->info);
	mpc_streaminfo_read(p_MF->info, p_MF->callbacks);

	p_MF->p_FI = ret;
	ret->BitsPerSample = (p_MF->info->bitrate == 0 ? 16 : p_MF->info->bitrate);
	ret->BitRate = p_MF->info->sample_freq;
	ret->Channels = p_MF->info->channels;

	mpc_decoder_setup(p_MF->decoder, p_MF->callbacks);
	mpc_decoder_initialize(p_MF->decoder, p_MF->info);

	p_MF->p_Playback = new Playback(ret, MPC_FillBuffer, p_MF->buffer, 8192, p_MPCFile);

	return ret;
}

long MPC_FillBuffer(void *p_MPCFile, BYTE *OutBuffer, int nOutBufferLen)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	BYTE *OBuff = OutBuffer;
	static bool eof = false;

	while (OBuff - OutBuffer < nOutBufferLen && eof == false)
	{
		short *out = (short *)(p_MF->buffer + (OBuff - OutBuffer));
		int nOut = 0;

		if (p_MF->PCMUsed == 0)
		{
			if (mpc_decoder_decode(p_MF->decoder, p_MF->framebuff, NULL, NULL) == 0)
				return -2;
		}

		for (int i = p_MF->PCMUsed; i < MPC_DECODER_BUFFER_LENGTH / sizeof(MPC_SAMPLE_FORMAT); i += p_MF->p_FI->Channels)
		{
			short Sample = 0;

			if (p_MF->p_FI->Channels == 1)
			{
				Sample = FloatToShort(p_MF->framebuff[i]);
				out[(nOut / 2)] = Sample;
				nOut += 2;
			}
			else if (p_MF->p_FI->Channels == 2)
			{
				Sample = FloatToShort(p_MF->framebuff[i]);
				out[(nOut / 2) + 0] = Sample;

				Sample = FloatToShort(p_MF->framebuff[i + 1]);
				out[(nOut / 2) + 1] = Sample;
				nOut += 4;
			}

			if (((OBuff - OutBuffer) + nOut) >= nOutBufferLen)
			{
				p_MF->PCMUsed = i + 1;
				i = MPC_DECODER_BUFFER_LENGTH / sizeof(MPC_SAMPLE_FORMAT);
			}
		}

		if (((OBuff - OutBuffer) + nOut) < nOutBufferLen)
		{
			p_MF->PCMUsed = 0;
		}

		fwrite(out, nOut, 1, p_MF->f_Out);
		memcpy(OBuff, out, nOut);
		OBuff += nOut;
	}

	return OBuff - OutBuffer;
}

void MPC_Play(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	int res = 1;

	p_MF->f_Out = fopen("mpc.raw", "wb");
	while (res > 0)
	{
		res = MPC_FillBuffer(p_MPCFile, p_MF->buffer, 8192);
	}
	//p_MF->p_Playback->Play();
	fclose(p_MF->f_Out);
}

bool Is_MPC(char *FileName)
{
	FILE *f_MPC = fopen(FileName, "rb");
	char MPCSig[3];

	fread(MPCSig, 3, 1, f_MPC);
	fclose(f_MPC);

	if (strncmp(MPCSig, "MP+", 3) != 0 &&
		strncmp(MPCSig, "MPC", 3) != 0)
		return false;

	return true;
}