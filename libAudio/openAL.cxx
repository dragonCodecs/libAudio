#include "openAL.hxx"

std::unique_ptr<alContext_t> alContext;

alContext_t *alContext_t::ensure() noexcept
{
	if (!alContext)
		alContext = makeUniqueT<alContext_t>();
	alContext->makeCurrent();
	return alContext.get();
}

alContext_t::alContext_t() noexcept : device{alcOpenDevice(nullptr)},
	context{alcCreateContext(device, nullptr)} { }

alContext_t::~alContext_t()
{
	if (alcGetCurrentContext() == context)
		alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

void alContext_t::makeCurrent() noexcept
	{ alcMakeContextCurrent(context); }

alSource_t::alSource_t() noexcept : source{AL_NONE}
{
	alGenSources(1, &source);
	if (!source)
		return;
	alSourcef(source, AL_GAIN, 1);
	alSourcef(source, AL_PITCH, 1);
	alSource3f(source, AL_POSITION, 0, 0, 0);
	alSource3f(source, AL_VELOCITY, 0, 0, 0);
	alSource3f(source, AL_DIRECTION, 0, 0, 0);
	alSourcef(source, AL_ROLLOFF_FACTOR, 0);
}

alSource_t::~alSource_t() noexcept
{
	if (source != AL_NONE)
		alDeleteSources(1, &source);
}

void alSource_t::queue(alBuffer_t &buffer) const noexcept
{
	ALuint _buffer = buffer;
	alSourceQueueBuffers(source, 1, &_buffer);
	buffer.isQueued(true);
}

ALuint alSource_t::dequeueOne() const noexcept
{
	ALuint buffer = AL_NONE;
	alSourceUnqueueBuffers(source, 1, &buffer);
	return buffer;
}

void alSource_t::play() const noexcept { alSourcePlay(source); }
void alSource_t::pause() const noexcept { alSourcePause(source); }
void alSource_t::stop() const noexcept { alSourceStop(source); }

int alSource_t::processedBuffers() const noexcept
{
	int result = 0;
	alGetSourcei(source, AL_BUFFERS_PROCESSED, &result);
	return result;
}

int alSource_t::state() const noexcept
{
	int result = 0;
	alGetSourcei(source, AL_SOURCE_STATE, &result);
	return result;
}

alBuffer_t::alBuffer_t() noexcept : buffer{AL_NONE}
	{ alGenBuffers(1, &buffer); }

alBuffer_t::~alBuffer_t() noexcept
{
	if (buffer != AL_NONE)
		alDeleteBuffers(1, &buffer);
}

bool alBuffer_t::operator ==(const ALuint value) const noexcept
	{ return buffer == value; }
void alBuffer_t::fill(const void *const data, const uint32_t dataLength, const ALenum format,
	uint32_t frequency) const noexcept { alBufferData(buffer, format, data, dataLength, frequency); }
