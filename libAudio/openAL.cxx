#include "openAL.hxx"

alBuffer_t::alBuffer_t() noexcept : buffer{AL_NONE}
	{ alGenBuffers(1, &buffer); }

alBuffer_t::~alBuffer_t() noexcept
{
	if (buffer != AL_NONE)
		alDeleteBuffers(1, &buffer);
}
