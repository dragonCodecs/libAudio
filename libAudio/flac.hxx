// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2019-2023 Rachel Mant <git@dragonmux.network>
#ifndef FLAC_HXX
#define FLAC_HXX

#include <FLAC/all.h>

#include <substrate/utility>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * Internal structure for holding the decoding context for a given FLAC file
 */
struct flac_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The decoder context handle
	 */
	FLAC__StreamDecoder *streamDecoder;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	std::unique_ptr<uint8_t []> buffer;
	uint32_t bufferLen;
	uint8_t playbackBuffer[16384];
	/*!
	 * @internal
	 * The amount to shift the sample data by to convert it
	 */
	uint8_t sampleShift;
	/*!
	 * @internal
	 * The count of the number of bytes left to process
	 * (also thinkable as the number of bytes left to read)
	 */
	uint32_t bytesRemain;
	uint32_t bytesAvail;

	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
	bool finish() noexcept;
	libAUDIO_NO_DISCARD(FLAC__StreamDecoderState nextFrame() noexcept);
};

/*!
 * @internal
 * Internal structure for holding the encoding context for a given FLAC file
 */
struct flac_t::encoderContext_t final
{
	/*!
	 * @internal
	 * The encoder context handle
	 */
	FLAC__StreamEncoder *streamEncoder;
	std::array<int32_t, 1024> encoderBuffer;
	std::array<FLAC__StreamMetadata *, 2> metadata;

	encoderContext_t() noexcept;
	~encoderContext_t() noexcept;
	bool finish() noexcept;
	void fillFrame(const int8_t *const buffer, const uint32_t samples) noexcept;
	void fillFrame(const int16_t *const buffer, const uint32_t samples) noexcept;
};

#endif /*FLAC_HXX*/
