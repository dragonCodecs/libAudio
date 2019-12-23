#include <cstdint>
#include <cstdio>
#include <array>

#include <libAudio.h>

std::array<char, 8192> buffer{};

int main(int32_t argc, char **argv)
{
	if (argc < 2)
		return -1;

	for (int32_t i = 1; i < argc; ++i)
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

		printf("Processing file %s.. ", argv[i]);
		fflush(stdout);
		while (audioFillBuffer(audioFile, buffer.data(), buffer.size()) >= 0)
			continue;
		puts("done");
		audioCloseFile(audioFile);
	}
	return 0;
}
