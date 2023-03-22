// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2019-2023 Rachel Mant <git@dragonmux.network>
#ifndef PLAYBACK_HXX
#define PLAYBACK_HXX

#include <cstdint>
#include <mutex>
#include <chrono>
#include <substrate/utility>
#include "fileInfo.hxx"

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
	[[nodiscard]] int64_t refillBuffer() const noexcept;
	[[nodiscard]] uint8_t *buffer() const noexcept;
	[[nodiscard]] uint32_t bufferLength() const noexcept;
	[[nodiscard]] uint8_t bitsPerSample() const noexcept;
	[[nodiscard]] uint32_t bitRate() const noexcept;
	[[nodiscard]] uint8_t channels() const noexcept;
	[[nodiscard]] std::chrono::nanoseconds sleepTime() const noexcept;
	[[nodiscard]] playbackMode_t mode() const noexcept;
	[[nodiscard]] bool isPlaying() const noexcept;

public:
	virtual ~audioPlayer_t() = default;
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;
	bool mode(playbackMode_t _mode) noexcept;
	virtual void volume(float level) noexcept = 0;

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
	playback_t(void *audioFile, fileFillBuffer_t fillBuffer, uint8_t *buffer,
		uint32_t bufferLength, const fileInfo_t &fileInfo);
	playback_t(playback_t &&) noexcept = default;
	playback_t &operator =(playback_t &&) noexcept = default;
	~playback_t() noexcept = default;
	bool mode(playbackMode_t mode) noexcept;
	void play();
	void pause();
	void stop();
	void volume(float level) noexcept;

	playback_t(const playback_t &) noexcept = delete;
	playback_t &operator =(const playback_t &) noexcept = delete;
};

#endif /*PLAYBACK_HXX*/
