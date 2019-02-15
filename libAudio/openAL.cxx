#include "openAL.hxx"

alBuffer_t::alBuffer_t() noexcept : buffer{AL_NONE}
	{ alGenBuffers(1, &buffer); }

alBuffer_t::~alBuffer_t() noexcept
{
	if (buffer != AL_NONE)
		alDeleteBuffers(1, &buffer);
}

void alBuffer_t::fill(const void *const data, const uint32_t dataLength, const ALenum format, uint32_t frequency)
	{ alBufferData(buffer, format, data, dataLength, frequency); }
