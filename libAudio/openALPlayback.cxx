#include "openALPlayback.hxx"

openALPlayback_t::openALPlayback_t(playback_t &_player) : audioPlayer_t{_player},
	source{}, buffers{{}}, eof{false}, playerThread{} { }

long openALPlayback_t::fillBuffer(alBuffer_t &_buffer) noexcept
{
	const long result = refillBuffer();
	if (result > 0)
	{
		_buffer.fill(buffer(), bufferLength(), format(), bitRate());
		source.queue(_buffer);
	}
	else
		eof = true;
	return result;
}

ALenum openALPlayback_t::format() const noexcept
{
	const uint8_t bits = bitsPerSample();
	const uint8_t _channels = channels();
	if (bits == 8)
	{
		if (_channels == 1)
			return AL_FORMAT_MONO8;
		else if (_channels == 2)
			return AL_FORMAT_STEREO16;
	}
	else if (bits == 16)
	{
		if (_channels == 1)
			return AL_FORMAT_MONO16;
		else if (_channels == 2)
			return AL_FORMAT_STEREO16;
	}
	return 0;
}

bool openALPlayback_t::haveQueued() const noexcept
{
	for (const alBuffer_t &buffer : buffers)
	{
		if (buffer.isQueued())
			return true;
	}
	return false;
}

void openALPlayback_t::refill() noexcept
{
	for (alBuffer_t &buffer : buffers)
	{
		if (buffer.isQueued())
			continue;
		else if (eof)
			return;
		fillBuffer(buffer);
	}
}

void openALPlayback_t::play()
{
	if (!isPlaying())
	{
		playerThread = std::thread{[this]() { player(); }};
		if (mode() == playbackMode_t::wait)
			playerThread.join();
	}
}

void openALPlayback_t::pause()
{
	std::unique_lock<std::mutex> lock{stateMutex};
	if (isPlaying())
	{
		state = playState_t::pause;
		lock.unlock();
		if (mode() == playbackMode_t::async)
			playerThread.join();
	}
}

void openALPlayback_t::stop()
{
	std::unique_lock<std::mutex> lock{stateMutex};
	if (isPlaying())
	{
		state = playState_t::stop;
		lock.unlock();
		if (mode() == playbackMode_t::async)
			playerThread.join();
	}
}

void openALPlayback_t::player() noexcept
{
	std::unique_lock<std::mutex> lock{stateMutex};
	refill();
	source.play();
	state = playState_t::playing;
	while (state == playState_t::playing)
	{
		lock.unlock();

		std::this_thread::sleep_for(sleepTime());
		lock.lock();
	}

	if (state == playState_t::pause)
	{
		source.pause();
		state = playState_t::paused;
	}
	else
	{
		source.stop();
		state = playState_t::stopped;
	}
}
