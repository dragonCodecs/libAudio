#ifndef PLAYBACK__HXX
#define PLAYBACK__HXX

#include <stdint.h>
#include "libAudio.h"
#include "libAudio.hxx"

using bufferFillFunc_t = long (*)(void *p_File, uint8_t *OutBuffer, int nOutBufferLen);

struct playback_t final
{
private:
	void *audioFile;
	bufferFillFunc_t fillBuffer;
	uint8_t *buffer;
	uint32_t bufferLength;
	uint32_t bitRate;
	uint8_t channels;

public:
	playback_t(void *const audioFile, const bufferFillFunc_t fillBuffer, uint8_t *const buffer,
		const uint32_t bufferLength, const fileInfo_t &fileInfo);
	void play();
	void pause();
	void stop();
};

#endif /*PLAYBACK__HXX*/
