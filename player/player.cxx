#include <libAudio.h>
#include <stdint.h>
#include <stdio.h>
#ifdef _WINDOWS
#include <conio.h>
#endif
//#include <process.h>
//#include <windows.h>
#include <stddef.h>

int main(int argc, char **argv)
{
	void *AudioFile = NULL;
	FileInfo *p_FI = NULL;

	if (argc < 2)
		return -1;

	for (int i = 1; i < argc; i++)
	{
		AudioFile = Audio_OpenR(argv[i]);
		if (AudioFile == NULL)
			continue;
		p_FI = Audio_GetFileInfo(AudioFile);
		if (p_FI == NULL)
		{
			Audio_CloseFileR(AudioFile);
			continue;
		}

		printf("File %s, TotalTime: %" PRIu64 "m %us, BitRate: %uHz, Title: %s, Artist: %s, Album: %s, Channels: %d\n", argv[i],
			p_FI->TotalTime / 60, uint8_t(p_FI->TotalTime % 60), p_FI->BitRate, p_FI->Title, p_FI->Artist, p_FI->Album, p_FI->Channels);
		/*{ // The following code makes the UTF-8 returned in p_FI display correctly under Windows.. the only OS that has a problem with this..
			wchar_t *Title;
			int wTitleLen = MultiByteToWideChar(CP_UTF8, 0, p_FI->Title, -1, NULL, NULL);
			Title = new wchar_t[wTitleLen];
			MultiByteToWideChar(CP_UTF8, 0, p_FI->Title, -1, Title, wTitleLen);
			MessageBoxW(NULL, Title, L"libAudio_TestApp", MB_OK);
			delete [] Title;
		}*/

		Audio_Play(AudioFile);
		Audio_CloseFileR(AudioFile);
		AudioFile = NULL;
	}

#ifdef _WINDOWS
	printf("\nPress Any Key To Continue....\n");
	getch();
#endif
	return 0;
}
