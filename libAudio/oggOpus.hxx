// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2021-2023 Rachel Mant <git@dragonmux.network>
#ifndef OGG_OPUS_HXX
#define OGG_OPUS_HXX

#include <opusfile.h>
#include <opusenc.h>

#include <substrate/utility>

#include "libAudio.h"
#include "libAudio.hxx"
#include "oggCommon.hxx"

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
	OggOpusFile *decoder;
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
 * Internal structure for holding the encoding context for a given Ogg|Opus file
 */
struct oggOpus_t::encoderContext_t final
{
	/*!
	 * @internal
	 * The Ogg|Opus encoder state
	 */
	OggOpusEnc *encoder;

	encoderContext_t() noexcept;
	~encoderContext_t() noexcept;
};

#endif /*OGG_OPUS_HXX*/
