#ifndef OGG_VORBIS__HXX
#define OGG_VORBIS__HXX

#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>

#include "libAudio.h"
#include "libAudio.hxx"
#include "oggCommon.hxx"

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

/*!
 * @internal
 * Internal structure for holding the encoding context for a given Ogg/Vorbis file
 */
struct oggVorbis_t::encoderContext_t final
{
	/*!
	 * @internal
	 * The Vorbis Digital Signal Processing state
	 */
	vorbis_dsp_state encoderState;
	/*!
	 * @internal
	 * The Vorbis encoding context
	 */
	vorbis_block blockState;
	/*!
	 * @internal
	 * The Ogg Stream state
	 */
	ogg_stream_state streamState;
	/*!
	 * @internal
	 * Sturcture describing info about the Vorbis stream being encoded
	 */
	vorbis_info vorbisInfo;
	bool eos;

	encoderContext_t() noexcept;
	~encoderContext_t() noexcept;
	bool writePage(const fd_t &fd, const bool force) noexcept;
};

#endif /*OGG_VORBIS__HXX*/
