#include "libAudio.h"
#include <stdio.h>
#ifdef _WINDOWS
#include <conio.h>
#endif
//#include <process.h>
//#include <windows.h>
#include <math.h>

int main(int argc, char **argv)
{
	int Type = 0;
	void *AudioFile = NULL;
	FileInfo *p_FI = NULL;

	if (argc < 2)
		return -1;

	for (int i = 1; i < argc; i++)
	{
		AudioFile = Audio_OpenR(argv[i], &Type);
		if (AudioFile == NULL)
			continue;
		p_FI = Audio_GetFileInfo(AudioFile, Type);
		if (p_FI == NULL)
		{
			Audio_CloseFileR(AudioFile, Type);
			continue;
		}

		printf("File %s, TotalTime: %um %us, BitRate: %iHz, Title: %s, Artist: %s, Album: %s, Channels: %i\n", argv[i],
			(uint32_t)(p_FI->TotalTime / 60.0), (uint32_t)fmod(p_FI->TotalTime, 60.0),
			(int)p_FI->BitRate, p_FI->Title, p_FI->Artist, p_FI->Album, p_FI->Channels);
		/*{ // The following code makes the UTF-8 returned in p_FI display correctly under Windows.. the only OS that has a problem with this..
			wchar_t *Title;
			int wTitleLen = MultiByteToWideChar(CP_UTF8, 0, p_FI->Title, -1, NULL, NULL);
			Title = new wchar_t[wTitleLen];
			MultiByteToWideChar(CP_UTF8, 0, p_FI->Title, -1, Title, wTitleLen);
			MessageBoxW(NULL, Title, L"libAudio_TestApp", MB_OK);
			delete [] Title;
		}*/

		Audio_Play(AudioFile, Type);

		Audio_CloseFileR(AudioFile, Type);
		AudioFile = NULL;
	}

#ifdef _WINDOWS
	printf("\nPress Any Key To Continue....\n");
	getch();
#endif
	return 0;
}
