#include "libAudio.h"
#include "libAudio.hxx"
#include "genericModule/genericModule.h"
#include "console.hxx"

uint8_t ExternalPlayback = 1;
uint8_t ToPlayback = 0;
using libAudio::console::operator ""_s;

class S3MHeader : public ModuleAllocator
{
public:
	// Common fields
	char *Name;
	char *Remark;
	uint16_t nOrders;
	uint16_t nSamples;
	uint16_t nInstruments;
	uint16_t nPatterns;
	uint8_t *Orders;
	uint16_t *Panning;
	uint16_t Flags;
	uint16_t CreationVersion;
	uint16_t FormatVersion;
	void *InstrumentPtrs;
	// Slightly badly named
	// SamplePtrs = pointers to where the sample *descriptors* are
	void *SamplePtrs;
	// PatternPtrs = pointers to where the compressed pattern data is
	void *PatternPtrs;

	// Fields specific to certain formats

	// MOD/AON
	uint8_t RestartPos;

	// S3M
	uint8_t Type;
	uint8_t GlobalVolume;
	uint8_t InitialSpeed;
	uint8_t InitialTempo;
	uint8_t MasterVolume;
	uint8_t ChannelSettings[32];

	// AON
	char *Author;
	char ArpTable[16][4];

	// FC1x
	uint32_t SeqLength;
	uint32_t PatternOffs;
	uint32_t PatLength;
	uint32_t FrequenciesOffs;
	uint32_t FrequenciesLength;
	uint32_t VolumeOffs;
	uint32_t VolumeLength;
	uint32_t SampleOffs;
	uint32_t SampleLength;

	// IT
	uint8_t Separation;
	uint32_t MessageOffs;
	std::array<uint8_t, 64> Volumes;
	std::array<bool, 64> PanSurround;

	uint8_t nChannels;
};

class S3MCommand : public ModuleAllocator
{
public:
	uint8_t Sample;
	uint8_t Note;
	uint8_t VolEffect;
	uint8_t VolParam;
	uint8_t Effect;
	uint8_t Param;
};

class S3MFile : public ModuleAllocator
{
public:
	uint8_t ModuleType;
	S3MHeader *p_Header;
	ModuleSample **p_Samples;
	ModulePattern **p_Patterns;

	virtual ~S3MFile() noexcept = default;
};

int64_t audioFile_t::writeBuffer(void const *, long) { return -2; }
bool audioFile_t::fileInfo(const fileInfo_t &) { return false; }

playback_t::playback_t(void *const audioFile_, const fileFillBuffer_t fillBuffer_, uint8_t *const buffer_,
	const uint32_t bufferLength_, const fileInfo_t &fileInfo) : audioFile{audioFile_}, fillBuffer{fillBuffer_},
	buffer{buffer_}, bufferLength{bufferLength_}, bitsPerSample(fileInfo.bitsPerSample), bitRate{fileInfo.bitRate},
	channels{fileInfo.channels}, sleepTime{}, playbackMode{playbackMode_t::wait}, player{nullptr}
{
	std::chrono::seconds bufferSize{bufferLength};
	bufferSize /= channels * (bitsPerSample / 8);
	sleepTime = std::chrono::duration_cast<std::chrono::nanoseconds>(bufferSize) / bitRate;
}

int64_t audioFillBuffer(void *audioFile, void *const buffer, const uint32_t length)
{
	const auto file = static_cast<audioFile_t *>(audioFile);
	if (!file)
		return 0;
	return file->fillBuffer(buffer, length);
}

int audioCloseFile(void *audioFile)
{
	const auto file = static_cast<const audioFile_t *>(audioFile);
	delete file;
	return 0;
}

bool isHex(const char *str, int len)
{
	for (int i = 0; i < len; i++)
	{
		if (!((str[i] >= '0' && str[i] <= '9') ||
			(str[i] >= 'A' && str[i] <= 'Z') ||
			(str[i] >= 'a' && str[i] <= 'z')))
			return false;
	}
	return true;
}

uint8_t hex2int(char c)
{
	uint8_t ret = uint8_t(c - '0');
	if (ret > 9)
	{
		if (c >= 'A' && c <= 'Z')
			ret -= 17;
		else
			ret -= 49;
	}
	return ret;
}

uint8_t getEffect(const char *hex)
{
	size_t len = strlen(hex);
	if (len == 2 && isHex(hex, len))
	{
		uint8_t ret = hex2int(hex[0]) << 4;
		ret |= hex2int(hex[1]);
		if (ret != 0)
			return ret;
	}

	console.error("Bad effect ID, must consist of exactly 2 non-zero hex digits!");
	exit(1);

	return 0;
}

int main(int argc, char **argv)
{
	uint8_t effect;

	if (argc != 3)
	{
		// usage();
		return 2;
	}
	console = {stdout, stderr};

//	printf("Checking command line....\n");
	effect = getEffect(argv[2]);

	const std::unique_ptr<modS3M_t> file{modS3M_t::openR(argv[1])};
	console.info("Checking if S3M.... "_s, file ? "yes" : "no");
	if (file)
	{
		const auto &fileInfo = file->fileInfo();
		const auto &context = *file->context();
		const auto module = reinterpret_cast<S3MFile *>(context.mod.get());

		console.info("Name - '"_s, fileInfo.title, "'...."_s);
		console.info("Checking for command"_s);
		console.info('\t', nullptr);

		bool found = false;
		const auto patterns = module->p_Patterns;
		for (uint16_t i = 0; i < module->p_Header->nPatterns; i++)
		{
			const auto &patternCommands = patterns[i]->commands();
			for (const auto &commands : patternCommands)
			{
				for (uint8_t k = 0; k < 64 && !found; k++)
				{
					const S3MCommand &command = reinterpret_cast<S3MCommand &>(commands[k]);
					// If effect not "CMD_NONE" and equals effect
					if (command.Effect == effect)
						found = true;
				}
				if (found)
					break;
			}
			if (found)
				break;

			fputc('.', stdout);
			fflush(stdout);
		}

		fputc('\n', stdout);
		console.info("Command found? "_s, found ? "yes"_s : "no"_s);
		return 0;
	}
	else
		return 1;
}
