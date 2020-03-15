#include <cstdint>
#include <map>
#include <string>
#include <array>
#include <algorithm>

#include "libAudio.h"
#include "libAudio.hxx"
// XXX: This header actually needs installing and the current header mess figured out + fixed.
#include "console.hxx"

using libAudio::console::operator ""_s;
using libAudio::console::asTime_t;

std::array<uint8_t, 8192> buffer;

struct audioClose_t final { void operator ()(void *ptr) noexcept { audioCloseFile(ptr); } };

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

uint8_t mapType(std::string typeName)
{
	std::transform(typeName.begin(), typeName.end(), typeName.begin(), ::toupper);
	const auto entry = typeMap.find(typeName);
	if (entry == typeMap.end())
		return AUDIO_OGG_VORBIS;
	return entry->second;
}

int usage(const char *const program) noexcept
{
	console.info("Usage:"_s);
	console.info(program, " <type> [fileIn fileOut] ... [fileIn fileOut]"_s);
	return -2;
}

void printInfo(const char *const fileName, const fileInfo_t &info) noexcept
{
	console.info("Input file '", fileName, "', TotalTime: ", asTime_t{info.totalTime},
		", Sample Rate: ", info.bitRate, "Hz, Title: ", info.title, ", Artist: ", info.artist,
		", Album: ", info.album, ", Channels: ", info.channels);
}

void printStatus(const uint32_t loops, const fileInfo_t &info) noexcept
{
	const uint8_t bytesPerSample = info.channels * (info.bitsPerSample / 8);
	const uint64_t samples = (buffer.size() / bytesPerSample) * loops;
	console.info(samples / info.bitRate, "s done\r"_s, nullptr);
}

int main(int argc, char **argv)
{
	console = {stdout, stderr};
	if (argc < 2)
		return usage(argv[0]);
	ExternalPlayback = 1;
	const uint8_t type = mapType(argv[1]);
	argc -= argc % 2;
	for (uint32_t i = 2; i < uint32_t(argc); i += 2)
	{
		std::unique_ptr<void, audioClose_t> inFile{audioOpenR(argv[i])};
		std::unique_ptr<void, audioClose_t> outFile{audioOpenW(argv[i + 1], type)};

		if (!inFile || !outFile)
		{
			if (!inFile)
				console.error("Failed to open input file "_s, argv[i]);
			if (!outFile)
				console.error("Failed to open output file "_s, argv[i + 1]);
			continue;
		}
		const fileInfo_t *fileInfo{audioGetFileInfo(inFile.get())};
		printInfo(argv[i], *fileInfo);
		if (!audioSetFileInfo(outFile.get(), fileInfo))
		{
			console.error("Failed to set file information for "_s, argv[i + 1]);
			continue;
		}

		uint32_t loops = 0;
		for (int64_t result = 1; result > 0; ++loops)
		{
			result = audioFillBuffer(inFile.get(), buffer.data(), uint32_t(buffer.size()));
			result = audioWriteBuffer(outFile.get(), buffer.data(), result);
			printStatus(loops + 1, *fileInfo);
		}
		console.info("Transcode to "_s, argv[i + 1], " complete"_s);
	}
	return 0;
}
