#include "playback.hxx"

playback_t::playback_t(void *const audioFile_, const bufferFillFunc_t fillBuffer_, uint8_t *const buffer_,
	const uint32_t bufferLength_, const fileInfo_t &fileInfo) : audioFile{audioFile_}, fillBuffer{fillBuffer_},
	buffer{buffer_}, bufferLength{bufferLength_}, bitRate{fileInfo.bitRate}, channels{fileInfo.channels} { }

void playback_t::play()
{
}

void playback_t::pause()
{
}

void playback_t::stop()
{
}
