#ifndef OGG_VORBIS__HXX
#define OGG_VORBIS__HXX

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * Internal structure for holding the decoding context for a given Ogg|Vorbis file
 */
struct oggVorbis_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The decoder context handle and handle to the Ogg|Vorbis
	 * file being decoded
	 */
	OggVorbis_File decoder;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	bool eof;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
};

struct oggVorbis_t::encoderContext_t final
{
	vorbis_info vorbisInfo;

	encoderContext_t() noexcept;
	~encoderContext_t() noexcept;
};

#endif /*OGG_VORBIS__HXX*/