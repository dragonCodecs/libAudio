#include <stdint.h>
#include <libAudio.h>
// XXX: This header actually needs installing and the current header mess figured out + fixed.
#include "../libAudio/console.hxx"

using libAudio::console::asTime_t;

int main(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	console = {stdout, stderr};

	for (int i = 1; i < argc; i++)
	{
		void *audioFile = audioOpenR(argv[i]);
		if (!audioFile)
			continue;
		const auto info = audioGetFileInfo(audioFile);
		if (!info)
		{
			audioCloseFile(audioFile);
			continue;
		}

		console.info("File '", argv[i], "', TotalTime: ", asTime_t{info->totalTime}, ", Sample Rate: ", info->bitRate,
			"Hz, Title: ", info->title, ", Artist: ", info->artist, ", Album: ", info->album, ", Channels: ",
			info->channels);

		audioPlay(audioFile);
		audioCloseFile(audioFile);
	}
	return 0;
}
