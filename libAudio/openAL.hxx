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

struct alBuffer_t final
{
private:
	ALuint buffer;

public:
	alBuffer_t() noexcept;
	alBuffer_t(alBuffer_t &&_buffer) noexcept;
	~alBuffer_t() noexcept;
	void fill(const void *const data, const uint32_t dataLength, const ALenum format, uint32_t frequency);

	alBuffer_t(const alBuffer_t &) = delete;
	alBuffer_t &operator =(const alBuffer_t &) = delete;
};

#endif /*OPEN_AL__HXX*/
