#include <libAudio.h>
#include <stdio.h>
#ifdef _WINDOWS
//#include <process.h>
//#include <windows.h>
#endif
#include <inttypes.h>

int main(int argc, char **argv)
{
	if (argc < 2)
		return -1;

	for (int i = 1; i < argc; i++)
	{
		void *AudioFile = Audio_OpenR(argv[i]);
		if (AudioFile == NULL)
			continue;
		const auto info = Audio_GetFileInfo(AudioFile);
		if (info == NULL)
		{
			Audio_CloseFileR(AudioFile);
			continue;
		}

		printf("File %s, TotalTime: %" PRIu64 "m %us, BitRate: %uHz, Title: %s, Artist: %s, Album: %s, Channels: %d\n",
			argv[i], info->totalTime / 60, uint8_t(info->totalTime % 60), info->bitRate, info->title.get(),
			info->artist.get(), info->album.get(), info->channels);
		/*{ // The following code makes the UTF-8 returned in info display correctly under Windows.. the only OS that has a problem with this..
			wchar_t *Title;
			int wTitleLen = MultiByteToWideChar(CP_UTF8, 0, info->Title, -1, NULL, NULL);
			Title = new wchar_t[wTitleLen];
			MultiByteToWideChar(CP_UTF8, 0, info->Title, -1, Title, wTitleLen);
			MessageBoxW(NULL, Title, L"libAudio_TestApp", MB_OK);
			delete [] Title;
		}*/

		Audio_Play(AudioFile);
		Audio_CloseFileR(AudioFile);
	}
	return 0;
}
