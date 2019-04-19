#ifndef FILE_INFO__HXX
#define FILE_INFO__HXX

#include <stdint.h>
#include <memory>
#include <vector>

struct fileInfo_t final
{
	uint64_t totalTime;
	uint32_t bitsPerSample;
	uint32_t bitRate;
	uint8_t channels;
	// int bitStream;
	std::unique_ptr<char []> title;
	std::unique_ptr<char []> artist;
	std::unique_ptr<char []> album;
	std::vector<std::unique_ptr<char []>> other;

	fileInfo_t() noexcept : totalTime{0}, bitsPerSample{0}, bitRate{0},
		channels{0}, title{}, artist{}, album{}, other{} { }

	void operator =(const fileInfo_t &info) noexcept
	{
		totalTime = info.totalTime;
		bitsPerSample = info.bitsPerSample;
		bitRate = info.bitRate;
		channels = info.channels;
	}
};

#endif /*FILE_INFO__HXX*/
