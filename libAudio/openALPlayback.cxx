#include <algorithm>
#include <exception>
#include "libAudio.h"
#include "openALPlayback.hxx"

openALPlayback_t::openALPlayback_t(playback_t &_player) : audioPlayer_t{_player},
	context{alContext_t::ensure()}, source{}, buffers{{}}, bufferFormat{format()},
	eof{false}, playerThread{} { }

openALPlayback_t::~openALPlayback_t()
{
	stop();
	auto queued = std::count_if(buffers.begin(), buffers.end(),
		[](const alBuffer_t &buffer) { return buffer.isQueued(); });
	while (queued--)
		source.dequeueOne();
}

bool openALPlayback_t::fillBuffer(alBuffer_t &_buffer) noexcept
{
	const int64_t result = refillBuffer();
	if (result > 0)
	{
		_buffer.fill(buffer(), uint32_t(result), bufferFormat, bitRate());
		source.queue(_buffer);
		eof = uint32_t(result) < bufferLength();
	}
	else
		eof = true;
	return result > 0;
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
			return AL_FORMAT_STEREO8;
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
		else if (eof || !fillBuffer(buffer))
			return;
	}
}

void openALPlayback_t::refill(const uint32_t count) noexcept
{
	for (uint32_t i = 0; i < count; ++i) try
	{
		ALuint _buffer = source.dequeueOne();
		alBuffer_t &buffer = find(_buffer);
		if (eof)
			buffer.isQueued(false);
		else
			buffer.isQueued(fillBuffer(buffer));
	}
	catch (std::invalid_argument &error)
		{ puts(error.what()); }
}

alBuffer_t &openALPlayback_t::find(const ALuint _buffer)
{
	for (alBuffer_t &buffer : buffers)
	{
		if (buffer == _buffer)
			return buffer;
	}
	throw std::invalid_argument{"Requested buffer ID does not exist"};
}

void openALPlayback_t::play()
{
	std::unique_lock<std::mutex> lock{stateMutex};
	if (!isPlaying())
	{
		playerThread = std::thread{[this]() noexcept { player(); }};
		lock.unlock();
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

void openALPlayback_t::volume(float level) noexcept
{
	if (level > 1.f)
		level = 1.f;
	else if (level < 0.f)
		level = 0.f;
	source.level(level);
}

void openALPlayback_t::player() noexcept
{
	std::unique_lock<std::mutex> lock{stateMutex};
	refill();
	if (haveQueued())
	{
		source.play();
		state = playState_t::playing;
	}
	lock.unlock();

	while (state == playState_t::playing)
	{
		const int processed = source.processedBuffers();
		refill(processed);
		if (source.state() != AL_PLAYING)
		{
			if (haveQueued())
				source.play();
			else
				break;
		}
		std::this_thread::sleep_for(sleepTime());
	}

	lock.lock();
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

void audioDefaultLevel(const float level)
	{ defaultLevel_ = level; }

std::vector<std::string> audioOutputDevices()
{
	if (!alContext_t::haveExtension("ALC_ENUMERATION_EXT"))
		return {};
	std::vector<std::string> result{};
	const auto *devices = alContext_t::devices();
	while (*devices)
	{
		const auto length = std::strlen(devices);
		result.emplace_back(devices, length);
		devices += length + 1;
	}
	return result;
}
