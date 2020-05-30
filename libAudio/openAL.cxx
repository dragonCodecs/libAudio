#include "openAL.hxx"
#include "openALShim.hxx"

std::unique_ptr<alContext_t> alContext;
std::atomic<float> defaultLevel_{1.f};

alContext_t *alContext_t::ensure() noexcept
{
	alcGetError(nullptr);
	if (!alContext)
		alContext = makeUniqueT<alContext_t>();
	alContext->makeCurrent();
	alGetError();
	return alContext.get();
}

alContext_t::alContext_t() noexcept : device{al::alcOpenDevice(al::alcGetString(nullptr,
	ALC_DEFAULT_DEVICE_SPECIFIER))}, context{al::alcCreateContext(device, nullptr)} { }

alContext_t::~alContext_t() noexcept
{
	if (al::alcGetCurrentContext() == context)
		al::alcMakeContextCurrent(nullptr);
	al::alcDestroyContext(context);
	al::alcCloseDevice(device);
}

void alContext_t::makeCurrent() noexcept
	{ al::alcMakeContextCurrent(context); }

bool alContext_t::haveExtension(const char *const extensionName) noexcept
	{ return al::alcIsExtensionPresent(nullptr, extensionName) == AL_TRUE; }

const char *alContext_t::devices() noexcept
	{ return al::alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER); }

alSource_t::alSource_t() noexcept : source{AL_NONE}
{
	al::alGenSources(1, &source);
	if (!source)
		return;
	al::alSourcef(source, AL_GAIN, defaultLevel_);
	al::alSourcef(source, AL_PITCH, 1.f);
	al::alListener3f(AL_POSITION, 0.f, 0.f, 0.f);
	al::alSource3f(source, AL_POSITION, 0.f, 0.f, 0.f);
	al::alSource3f(source, AL_VELOCITY, 0.f, 0.f, 0.f);
	al::alSource3f(source, AL_DIRECTION, 0.f, 0.f, 0.f);
	al::alSourcef(source, AL_ROLLOFF_FACTOR, 0.f);
}

alSource_t::~alSource_t() noexcept
{
	if (source != AL_NONE)
		al::alDeleteSources(1, &source);
}

void alSource_t::queue(alBuffer_t &buffer) const noexcept
{
	ALuint _buffer = buffer;
	al::alSourceQueueBuffers(source, 1, &_buffer);
	buffer.isQueued(true);
}

ALuint alSource_t::dequeueOne() const noexcept
{
	ALuint buffer = AL_NONE;
	al::alSourceUnqueueBuffers(source, 1, &buffer);
	return buffer;
}

void alSource_t::play() const noexcept { al::alSourcePlay(source); }
void alSource_t::pause() const noexcept { al::alSourcePause(source); }
void alSource_t::stop() const noexcept { al::alSourceStop(source); }

int alSource_t::processedBuffers() const noexcept
{
	int result = 0;
	al::alGetSourcei(source, AL_BUFFERS_PROCESSED, &result);
	return result;
}

int alSource_t::queuedBuffers() const noexcept
{
	int result = 0;
	al::alGetSourcei(source, AL_BUFFERS_QUEUED, &result);
	return result;
}

int alSource_t::state() const noexcept
{
	int result = 0;
	al::alGetSourcei(source, AL_SOURCE_STATE, &result);
	return result;
}

void alSource_t::level(const float gain) const noexcept
	{ al::alSourcef(source, AL_GAIN, gain); }

alBuffer_t::alBuffer_t() noexcept : buffer{AL_NONE}, queued{false}
	{ al::alGenBuffers(1, &buffer); }

alBuffer_t::~alBuffer_t() noexcept
{
	if (buffer != AL_NONE)
		al::alDeleteBuffers(1, &buffer);
}

bool alBuffer_t::operator ==(const ALuint value) const noexcept
	{ return buffer == value; }
void alBuffer_t::fill(const void *const data, const uint32_t dataLength, const ALenum format,
	uint32_t frequency) const noexcept { al::alBufferData(buffer, format, data, dataLength, frequency); }
