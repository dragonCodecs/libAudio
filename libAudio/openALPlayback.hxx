#ifndef OPEN_AL_PLAYBACK__HXX
#define OPEN_AL_PLAYBACK__HXX

#include <array>
#include "playback.hxx"
#include "openAL.hxx"

struct openALPlayback_t final : audioPlayer_t
{
private:
	std::array<alBuffer_t, 4> buffers;
	bool eof;

	long fillBuffer(alBuffer_t &buffer) noexcept;
	ALenum format() const noexcept;
	bool haveQueued() const noexcept;
	void refill() noexcept;

public:
	openALPlayback_t(playback_t &_player);
	~openALPlayback_t() final override;
	void play() final override;
	void pause() final override;
	void stop() final override;
};

#endif /*OPEN_AL_PLAYBACK__HXX*/
