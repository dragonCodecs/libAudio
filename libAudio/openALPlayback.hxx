#ifndef OPEN_AL_PLAYBACK__HXX
#define OPEN_AL_PLAYBACK__HXX

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
	~openALPlayback_t() final override { stop(); }
	void play() final override;
	void pause() final override;
	void stop() final override;
};

#endif /*OPEN_AL_PLAYBACK__HXX*/
