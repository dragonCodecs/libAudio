#ifndef FLAC__HXX
#define FLAC__HXX

#include <FLAC/all.h>

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

	decoderContext_t();
	~decoderContext_t() noexcept;
	bool finish() noexcept;
	FLAC__StreamDecoderState nextFrame() noexcept;
};

/*!
 * @internal
 * Internal structure for holding the encoding context for a given FLAC file
 */
struct flac_t::encoderContext_t final
{
	encoderContext_t() noexcept;
	~encoderContext_t() noexcept;
};

#endif /*FLAC__HXX*/
