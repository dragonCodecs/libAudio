#ifndef OPEN_AL__HXX
#define OPEN_AL__HXX

#include <cstdint>
#ifdef _WINDOWS
#include <al.h>
#include <alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <atomic>
#include "uniquePtr.hxx"

struct alContext_t final
{
private:
	ALCdevice *device;
	ALCcontext *context;

	void makeCurrent() noexcept;

protected:
	alContext_t() noexcept;
	friend std::unique_ptr<alContext_t> makeUniqueT<alContext_t>();

public:
	static alContext_t *ensure() noexcept;
	~alContext_t() noexcept;

	alContext_t(const alContext_t &) noexcept = delete;
	alContext_t(alContext_t &&) noexcept = delete;
	alContext_t &operator =(const alContext_t &) noexcept = delete;
	alContext_t &operator =(alContext_t &&) noexcept = delete;
};

struct alBuffer_t;

struct alSource_t final
{
private:
	ALuint source;

public:
	alSource_t() noexcept;
	alSource_t(alSource_t &&) noexcept = default;
	alSource_t &operator =(alSource_t &&) noexcept = default;
	~alSource_t() noexcept;
	void queue(alBuffer_t &buffer) const noexcept;
	ALuint dequeueOne() const noexcept;
	void play() const noexcept;
	void pause() const noexcept;
	void stop() const noexcept;
	int processedBuffers() const noexcept;
	int queuedBuffers() const noexcept;
	int state() const noexcept;
	void level(const float gain) const noexcept;

	alSource_t(const alSource_t &) = delete;
	alSource_t &operator =(const alSource_t &) = delete;
};

struct alBuffer_t final
{
private:
	ALuint buffer;
	bool queued;

protected:
	operator ALuint() const noexcept { return buffer; }
	friend struct alSource_t;

public:
	alBuffer_t() noexcept;
	alBuffer_t(alBuffer_t &&) noexcept = default;
	alBuffer_t &operator =(alBuffer_t &&) noexcept = default;
	~alBuffer_t() noexcept;
	bool operator ==(const ALuint value) const noexcept;
	void fill(const void *const data, const uint32_t dataLength, const ALenum format,
		uint32_t frequency) const noexcept;
	bool isQueued() const noexcept { return queued; }
	void isQueued(const bool _queued) noexcept { queued = _queued; }

	alBuffer_t(const alBuffer_t &) = delete;
	alBuffer_t &operator =(const alBuffer_t &) = delete;
};

extern std::unique_ptr<alContext_t> alContext;
extern std::atomic<float> defaultLevel_;

#endif /*OPEN_AL__HXX*/
