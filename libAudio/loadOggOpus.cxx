#include <opus/opus.h>

#include "libAudio.h"
#include "libAudio.hxx"
#include "oggCommon.hxx"

/*!
 * @internal
 * @file loadOggOpus.cxx
 * @brief The implementation of the Ogg|Opus decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2019
 */

/*!
 * @internal
 * Internal structure for holding the decoding context for a given Ogg|Opus file
 */
struct oggOpus_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The decoder context handle and handle to the Ogg|Opus
	 * file being decoded
	 */
	OpusDecoder *decoder;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	bool eof;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
};
