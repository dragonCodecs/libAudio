// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2019-2023 Rachel Mant <git@dragonmux.network>
#ifndef OPEN_AL_PLAYBACK_HXX
#define OPEN_AL_PLAYBACK_HXX

#include <array>
#include <thread>
#include "playback.hxx"
#include "openAL.hxx"

struct openALPlayback_t final : audioPlayer_t
{
private:
	alContext_t *context;
	alSource_t source;
	std::array<alBuffer_t, 4> buffers;
	ALenum bufferFormat;
	bool eof;
	std::thread playerThread;

	bool fillBuffer(alBuffer_t &buffer) noexcept;
	ALenum format() const noexcept;
	bool haveQueued() const noexcept;
	void refill() noexcept;
	void refill(const uint32_t count) noexcept;
	alBuffer_t &find(const ALuint buffer);
	void player() noexcept;

public:
	openALPlayback_t(playback_t &_player);
	~openALPlayback_t() final;
	void play() final;
	void pause() final;
	void stop() final;
	void volume(float level) noexcept final;

	openALPlayback_t(const openALPlayback_t &) noexcept = delete;
	openALPlayback_t(openALPlayback_t &&) noexcept = delete;
	openALPlayback_t &operator =(const openALPlayback_t &) noexcept = delete;
	openALPlayback_t &operator =(openALPlayback_t &&) noexcept = delete;
};

#endif /*OPEN_AL_PLAYBACK_HXX*/
