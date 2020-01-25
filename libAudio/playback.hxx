#ifndef PLAYBACK__HXX
#define PLAYBACK__HXX

#include <cstdint>
#include <mutex>
#include <chrono>
#include "fileInfo.hxx"
#include "uniquePtr.hxx"

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

using fileFillBuffer_t = int64_t (*)(void *audioFile, void *const buffer, const uint32_t length);

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
	virtual ~audioPlayer_t() = default;
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;

	audioPlayer_t(const audioPlayer_t &) noexcept = delete;
	audioPlayer_t(audioPlayer_t &&) noexcept = delete;
	audioPlayer_t &operator =(const audioPlayer_t &) noexcept = delete;
	audioPlayer_t &operator =(audioPlayer_t &&) noexcept = delete;
};

struct playback_t final
{
private:
	void *audioFile;
	fileFillBuffer_t fillBuffer;
	uint8_t *buffer;
	uint32_t bufferLength;
	uint8_t bitsPerSample;
	uint32_t bitRate;
	uint8_t channels;
	std::chrono::nanoseconds sleepTime;
	playbackMode_t playbackMode;
	std::unique_ptr<audioPlayer_t> player;

protected:
	int64_t refillBuffer() noexcept;
	friend struct audioPlayer_t;

public:
	playback_t(void *const audioFile, const fileFillBuffer_t fillBuffer, uint8_t *const buffer,
		const uint32_t bufferLength, const fileInfo_t &fileInfo);
	playback_t(playback_t &&) noexcept = default;
	playback_t &operator =(playback_t &&) noexcept = default;
	~playback_t() noexcept = default;
	void mode(playbackMode_t mode) noexcept;
	void play();
	void pause();
	void stop();

	playback_t(const playback_t &) noexcept = delete;
	playback_t &operator =(const playback_t &) noexcept = delete;
};

#endif /*PLAYBACK__HXX*/
