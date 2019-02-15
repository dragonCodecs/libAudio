#ifndef OPEN_AL__HXX
#define OPEN_AL__HXX

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
	operator ALuint() const noexcept { return buffer; }

	alBuffer_t(const alBuffer_t &) = delete;
	alBuffer_t &operator =(const alBuffer_t &) = delete;
};

#endif /*OPEN_AL__HXX*/
