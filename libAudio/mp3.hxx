// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2022-2023 Rachel Mant <git@dragonmux.network>
#ifndef MP3_HXX
#define MP3_HXX

#include <mad.h>
#include <id3tag.h>
#include <lame/lame.h>

#include <substrate/utility>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * Internal structure for holding the decoding context for a given MP3 file
 */
struct mp3_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The MP3 stream handle
	 */
	mad_stream stream;
	/*!
	 * @internal
	 * The MP3 frame handle
	 */
	mad_frame frame;
	/*!
	 * @internal
	 * The MP3 audio synthesis handle
	 */
	mad_synth synth;
	/*!
	 * @internal
	 * The internal input data buffer
	 */
	std::array<uint8_t, 8192> inputBuffer;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t playbackBuffer[8192];
	/*!
	 * @internal
	 * A flag indicating if we have yet to decode this MP3's initial frame
	 */
	bool initialFrame;
	/*!
	 * @internal
	 * A count giving the amount of decoded PCM which has been used with
	 * the current values stored in buffer
	 */
	uint16_t samplesUsed;
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
	libAUDIO_NO_DISCARD(bool readData(const fd_t &fd) noexcept);
	libAUDIO_NO_DISCARD(int32_t decodeFrame(const fd_t &fd) noexcept);
	libAUDIO_NO_DISCARD(uint32_t parseXingHeader() noexcept);
};

/*!
 * @internal
 * Internal structure for holding the encoding context for a given MP3 file
 */
struct mp3_t::encoderContext_t final
{
	/*!
	 * @internal
	 * The MP3 LAME encoder state
	 */
	lame_t encoder;
	/*!
	 * @internal
	 * Offset for start of the LAME frame
	 */
	substrate::off_t lameFrameOffset;

	encoderContext_t() noexcept;
	~encoderContext_t() noexcept;
};

#endif /*OGG_OPUS_HXX*/
