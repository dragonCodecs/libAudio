#include "libAudio.h"
#include <conio.h>
#include <string>

static BYTE Buffer[8192];

int ToType(char *str)
{
	if (strnicmp(str, "AAC", 3) == 0)
		return AUDIO_AAC;
	else if (strnicmp(str, "MP4", 3) == 0 ||
		strnicmp(str, "M4A", 3) == 0)
		return AUDIO_MP4;
	else if (strnicmp(str, "FLAC", 4) == 0)
		return AUDIO_FLAC;
	else if (strnicmp(str, "OGG", 3) == 0)
		return AUDIO_OGG_VORBIS;
	else if (strnicmp(str, "MP3", 3) == 0)
		return AUDIO_MP3;
	else if (strnicmp(str, "MPC", 3) == 0 ||
		strnicmp(str, "MUSEPACK", 8) == 0)
		return AUDIO_MUSEPACK;
	else if (strnicmp(str, "OFG", 3) == 0 ||
		strnicmp(str, "OPTIMFROG", 9) == 0)
		return AUDIO_OPTIMFROG;
	else if (strnicmp(str, "WAV", 3) == 0)
		return AUDIO_WAVE;
	else if (strnicmp(str, "WPC", 3) == 0 ||
		strnicmp(str, "WAVPACK", 7) == 0)
		return AUDIO_WAVPACK;
	else if (strnicmp(str, "WMA", 3) == 0)
		return AUDIO_WMA;
	else
		return AUDIO_OGG_VORBIS;
}

void main(int argc, char **argv)
{
	int Type = 0, ret = 1, oType = 0, loops = 0;
	void *AudioFileIn = NULL;
	void *AudioFileOut = NULL;
	FileInfo *p_FI = NULL;

	if (argc < 2)
		return;

	AudioFileIn = Audio_OpenR(argv[1], &Type);
	if (argc > 2)
	{
		if (argc == 3)
			oType = AUDIO_OGG_VORBIS;
		else
			oType = ToType(argv[3]);
		AudioFileOut = Audio_OpenW(argv[2], oType);
	}
	else
	{
		std::string fnm = std::string(strdup(argv[1]));
		fnm.resize(fnm.size() - 4);
		fnm.append(".ogg");
		AudioFileOut = Audio_OpenW((char *)fnm.c_str(), AUDIO_OGG_VORBIS);
		fnm.clear();
		oType = AUDIO_OGG_VORBIS;
	}
	if (AudioFileIn == NULL || AudioFileOut == NULL)
		return;
	p_FI = Audio_GetFileInfo(AudioFileIn, Type);
	if (p_FI == NULL)
		return;
	Audio_SetFileInfo(AudioFileOut, p_FI, oType);

	printf("Input File %s, BitRate: %iHz, Title: %s, Artist: %s, Album: %s, Channels: %i\n", argv[1],
		p_FI->BitRate, p_FI->Title, p_FI->Artist, p_FI->Album, p_FI->Channels);

	while (ret > 0)
	{
		ret = Audio_FillBuffer(AudioFileIn, Buffer, 8192, Type);
		Audio_WriteBuffer(AudioFileOut, Buffer, ret, oType);
		loops++;
		fprintf(stdout, "%fs done\r", ((double)(8192 * loops)) / (double)p_FI->BitRate);
		fflush(stdout);
	}
	fprintf(stdout, "\nTranscode complete!\n");

	Audio_CloseFileR(AudioFileIn, Type);
	Audio_CloseFileW(AudioFileOut, oType);

	printf("\nPress Any Key To Continue....\n");
	getch();
}