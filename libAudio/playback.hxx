#ifndef PLAYBACK__HXX
#define PLAYBACK__HXX

#include <stdint.h>
#include <mutex>
#include <chrono>
#include "libAudio.h"
#include "fileInfo.hxx"
#include "uniquePtr.hxx"

using bufferFillFunc_t = long (*)(void *p_File, uint8_t *OutBuffer, int nOutBufferLen);

enum class playState_t : uint8_t
{
	stop,
	stopped,
	playing,
	pause,
	paused
};

enum class playbackMode_t : uint8_t
	{ wait, async };

struct playback_t;
struct audioPlayer_t
{
private:
	playback_t &player;

protected:
	playState_t state;
	std::mutex stateMutex;

	audioPlayer_t(playback_t &_player) noexcept : player{_player},
		state{playState_t::stopped}, stateMutex{} { }
	long refillBuffer() const noexcept;
	uint8_t *buffer() const noexcept;
	uint32_t bufferLength() const noexcept;
	uint8_t bitsPerSample() const noexcept;
	uint32_t bitRate() const noexcept;
	uint8_t channels() const noexcept;
	std::chrono::nanoseconds sleepTime() const noexcept;
	playbackMode_t mode() const noexcept;
	bool isPlaying() const noexcept;

public:
	virtual ~audioPlayer_t() { }
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;
};

struct playback_t final
{
private:
	void *audioFile;
	bufferFillFunc_t fillBuffer;
	uint8_t *buffer;
	uint32_t bufferLength;
	uint8_t bitsPerSample;
	uint32_t bitRate;
	uint8_t channels;
	std::chrono::nanoseconds sleepTime;
	playbackMode_t playbackMode;
	std::unique_ptr<audioPlayer_t> player;

protected:
	long refillBuffer() noexcept;
	friend struct audioPlayer_t;

public:
	playback_t(void *const audioFile, const bufferFillFunc_t fillBuffer, uint8_t *const buffer,
		const uint32_t bufferLength, const fileInfo_t &fileInfo);
	void mode(playbackMode_t mode) noexcept;
	void play();
	void pause();
	void stop();
};

#endif /*PLAYBACK__HXX*/