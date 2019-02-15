#ifndef OPEN_AL__HXX
#define OPEN_AL__HXX

#include <stdint.h>
#ifdef _WINDOWS
#include <al.h>
#include <alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

struct alBuffer_t;

struct alSource_t final
{
private:
	ALuint source;

public:
	alSource_t() noexcept;
	alSource_t(alSource_t &&_source) noexcept;
	~alSource_t() noexcept;
	void queue(alBuffer_t &buffer) const noexcept;

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
	void isQueued(const bool _queued) noexcept { queued = _queued; }
	friend struct alSource_t;

public:
	alBuffer_t() noexcept;
	alBuffer_t(alBuffer_t &&_buffer) noexcept;
	~alBuffer_t() noexcept;
	void fill(const void *const data, const uint32_t dataLength, const ALenum format,
		uint32_t frequency) const noexcept;
	bool isQueued() const noexcept { return queued; }

	alBuffer_t(const alBuffer_t &) = delete;
	alBuffer_t &operator =(const alBuffer_t &) = delete;
};

#endif /*OPEN_AL__HXX*/
