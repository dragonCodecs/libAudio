#ifndef M4A__HXX
#define M4A__HXX

#include <cstdint>
#include <neaacdec.h>
#include <faac.h>
#include <mp4v2/mp4v2.h>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * Internal structure for holding the decoding context for a given M4A/MP4 file
 */
struct m4a_t::decoderContext_t final
{
	/*!
	 * @internal
	 * The decoder context handle
	 */
	NeAACDecHandle decoder;
	/*!
	 * @internal
	 * The MP4v2 handle for the MP4 file being read from
	 */
	MP4FileHandle mp4Stream;
	/*!
	 * @internal
	 * The MP4v2 track from which the decoded audio data is being read
	 */
	MP4TrackId track;
	/*!
	 * @internal
	 * @var int frameCount
	 * The number of frames decoded relative to the total number
	 * @var int currentFrame
	 * The total number of frames to decode
	 * @var int samplesUsed
	 * The number of samples used so far from the current sample buffer
	 * @var int sampleCount
	 * The total number of samples in the current sample buffer
	 */
	uint32_t frameCount, currentFrame;
	uint64_t sampleCount, samplesUsed;
	/*!
	 * @internal
	 * Pointer to the static return result of the call to \c NeAACDecDecode()
	 */
	uint8_t *samples;
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;

	uint8_t playbackBuffer[8192];

	decoderContext_t();
	~decoderContext_t() noexcept;
	void finish() noexcept;
	void aacTrack(fileInfo_t &fileInfo) noexcept;
};

/*!
 * @internal
 * Internal structure for holding the encoding context for a given M4A/MP4 file
 */
struct m4a_t::encoderContext_t final
{
	/*!
	 * @internal
	 * The encoder context handle
	 */
	faacEncHandle encoder;
};

#endif /*M4A__HXX*/
