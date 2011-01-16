#include "libAudio.h"
#ifdef _WINDOWS
#include <conio.h>
#define strncasecmp strnicmp
#else
#include <string.h>
#endif
#include <string>
#include <stdio.h>

static BYTE Buffer[8192];

int ToType(char *str)
{
	if (strncasecmp(str, "AAC", 3) == 0)
		return AUDIO_AAC;
	else if (strncasecmp(str, "MP4", 3) == 0 ||
		strncasecmp(str, "M4A", 3) == 0)
		return AUDIO_MP4;
	else if (strncasecmp(str, "FLAC", 4) == 0)
		return AUDIO_FLAC;
	else if (strncasecmp(str, "OGG", 3) == 0)
		return AUDIO_OGG_VORBIS;
	else if (strncasecmp(str, "MP3", 3) == 0)
		return AUDIO_MP3;
	else if (strncasecmp(str, "MPC", 3) == 0 ||
		strncasecmp(str, "MUSEPACK", 8) == 0)
		return AUDIO_MUSEPACK;
	else if (strncasecmp(str, "OFG", 3) == 0 ||
		strncasecmp(str, "OPTIMFROG", 9) == 0)
		return AUDIO_OPTIMFROG;
	else if (strncasecmp(str, "WAV", 3) == 0)
		return AUDIO_WAVE;
	else if (strncasecmp(str, "WPC", 3) == 0 ||
		strncasecmp(str, "WAVPACK", 7) == 0)
		return AUDIO_WAVPACK;
	else if (strncasecmp(str, "WMA", 3) == 0)
		return AUDIO_WMA;
	else
		return AUDIO_OGG_VORBIS;
}

int main(int argc, char **argv)
{
	int ret = 1, Type = 0, loops = 0;
	void *AudioFileIn = NULL;
	void *AudioFileOut = NULL;
	FileInfo *p_FI = NULL;

	if (argc < 2)
		return -1;

	AudioFileIn = Audio_OpenR(argv[1]);
	if (argc > 2)
	{
		if (argc == 3)
			Type = AUDIO_OGG_VORBIS;
		else
			Type = ToType(argv[3]);
		AudioFileOut = Audio_OpenW(argv[2], Type);
	}
	else
	{
		std::string fnm = std::string(strdup(argv[1]));
		fnm.resize(fnm.size() - 4);
		fnm.append(".ogg");
		AudioFileOut = Audio_OpenW((char *)fnm.c_str(), AUDIO_OGG_VORBIS);
		fnm.clear();
		Type = AUDIO_OGG_VORBIS;
	}
	if (AudioFileIn == NULL || AudioFileOut == NULL)
		return -2;
	p_FI = Audio_GetFileInfo(AudioFileIn);
	if (p_FI == NULL)
		return -3;
	Audio_SetFileInfo(AudioFileOut, p_FI, Type);

	printf("Input File %s, BitRate: %iHz, Title: %s, Artist: %s, Album: %s, Channels: %i\n", argv[1],
		p_FI->BitRate, p_FI->Title, p_FI->Artist, p_FI->Album, p_FI->Channels);

	while (ret > 0)
	{
		ret = Audio_FillBuffer(AudioFileIn, Buffer, 8192);
		Audio_WriteBuffer(AudioFileOut, Buffer, ret, Type);
		loops++;
		fprintf(stdout, "%fs done\r", ((double)(8192 * loops)) / (double)p_FI->BitRate);
		fflush(stdout);
	}
	fprintf(stdout, "\nTranscode complete!\n");

	Audio_CloseFileR(AudioFileIn);
	Audio_CloseFileW(AudioFileOut, Type);

#ifdef _WINDOWS
	printf("\nPress Any Key To Continue....\n");
	getch();
#endif
}
