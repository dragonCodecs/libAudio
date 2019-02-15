#include "playback.hxx"
#include "openALPlayback.hxx"

using player_t = openALPlayback_t;

playback_t::playback_t(void *const audioFile_, const bufferFillFunc_t fillBuffer_, uint8_t *const buffer_,
	const uint32_t bufferLength_, const fileInfo_t &fileInfo) : audioFile{audioFile_}, fillBuffer{fillBuffer_},
	buffer{buffer_}, bufferLength{bufferLength_}, bitRate{fileInfo.bitRate}, channels{fileInfo.channels},
	sleepTime{}, playbackMode{playbackMode_t::wait}, player{makeUnique<player_t>(*this)}
{
	std::chrono::seconds bufferSize{bufferLength};
	bufferSize /= channels * (fileInfo.bitsPerSample / 8);
	sleepTime = std::chrono::duration_cast<std::chrono::nanoseconds>(bufferSize) / bitRate;
}

void playback_t::play()
{
	if (audioFile && player)
		player->play();
}

void playback_t::pause()
{
	if (player)
		player->pause();
}

void playback_t::stop()
{
	if (player)
		player->stop();
}

long audioPlayer_t::refillBuffer() const noexcept
	{ return player.refillBuffer(); }
long playback_t::refillBuffer() noexcept
	{ return fillBuffer(audioFile, buffer, bufferLength); }
void playback_t::mode(playbackMode_t _mode) noexcept { playbackMode = _mode; }

uint8_t *audioPlayer_t::buffer() const noexcept { return player.buffer; }
uint32_t audioPlayer_t::bufferLength() const noexcept { return player.bufferLength; }
uint8_t audioPlayer_t::bitsPerSample() const noexcept { return player.bitsPerSample; }
uint32_t audioPlayer_t::bitRate() const noexcept { return player.bitRate; }
uint8_t audioPlayer_t::channels() const noexcept { return player.channels; }
std::chrono::nanoseconds audioPlayer_t::sleepTime() const noexcept { return player.sleepTime; }
bool audioPlayer_t::isPlaying() const noexcept { return state == playState_t::playing; }
playbackMode_t audioPlayer_t::mode() const noexcept { return player.playbackMode; }
