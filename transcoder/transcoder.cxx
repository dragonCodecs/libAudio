#include <stdint.h>
#include <inttypes.h>
#include <map>
#include <string>
#include <array>

#include "libAudio.h"
#include "libAudio.hxx"

uint8_t type = AUDIO_OGG_VORBIS;
std::array<uint8_t, 8192> buffer;

struct audioCloseR_t final { void operator ()(void *ptr) noexcept { Audio_CloseFileR(ptr); } };
struct audioCloseW_t final { void operator ()(void *ptr) noexcept { Audio_CloseFileW(ptr, type); } };

const std::map<std::string, uint8_t> typeMap
{
	{"OGG", AUDIO_OGG_VORBIS},
	{"FLAC", AUDIO_FLAC},
	{"WAVE", AUDIO_WAVE},
	{"MP4", AUDIO_MP4},
	{"M4A", AUDIO_M4A},
	{"AAC", AUDIO_AAC},
	{"MP3", AUDIO_MP3},
	{"MPC", AUDIO_MUSEPACK},
	{"WVP", AUDIO_WAVPACK},
	{"OFR", AUDIO_OPTIMFROG},
	{"RA", AUDIO_REALAUDIO},
	{"WMA", AUDIO_WMA}
};

uint8_t mapType(const std::string typeName)
{
	const auto entry = typeMap.find(typeName);
	if (entry == typeMap.end())
		return AUDIO_OGG_VORBIS;
	return entry->second;
}

int usage(const char *const program) noexcept
{
	printf("Usage: \n"
		"%s <type> [fileIn fileOut] ... [fileIn fileOut]\n",
		program);
	return -2;
}

void printInfo(const char *const fileName, FileInfo &info) noexcept
{
	printf("Input file %s, Sample Rate: %u, Title: %s, Artist: %s, Album: %s, Channels: %u\n",
		fileName, info.BitRate, info.Title, info.Artist, info.Album, info.Channels);
}

void printStatus(const uint32_t loops, FileInfo &info) noexcept
{
	const uint8_t bytesPerSample = info.Channels * (info.BitsPerSample / 8);
	const uint64_t samples = (buffer.size() / bytesPerSample) * loops;
	printf("%" PRIu64 "s done\r", samples / info.BitRate);
	fflush(stdout);
}

int main(int argc, char **argv)
{
	if (argc < 2)
		return usage(argv[0]);
	type = mapType(argv[1]);
	argc -= argc % 2;
	for (uint32_t i = 2; i < uint32_t(argc); i += 2)
	{
		std::unique_ptr<void, audioCloseR_t> inFile{Audio_OpenR(argv[i])};
		std::unique_ptr<void, audioCloseW_t> outFile{Audio_OpenW(argv[i + 1], type)};

		if (!inFile || !outFile)
		{
			if (!inFile)
				printf("Failed to open input file %s\n", argv[i]);
			if (!outFile)
				printf("Failed to open output file %s\n", argv[i + 1]);
			continue;
		}
		FileInfo *fileInfo{Audio_GetFileInfo(inFile.get())};
		printInfo(argv[i], *fileInfo);
		Audio_SetFileInfo(outFile.get(), fileInfo, type);

		uint32_t loops = 0;
		for (int64_t result = 0; result > 0; ++loops)
		{
			result = Audio_FillBuffer(inFile.get(), buffer.data(), buffer.size());
			Audio_WriteBuffer(outFile.get(), buffer.data(), buffer.size(), type);
			printStatus(loops + 1, *fileInfo);
		}
	}
	return 0;
}