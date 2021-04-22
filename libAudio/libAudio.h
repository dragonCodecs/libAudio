#ifndef LIB_AUDIO__H
#define LIB_AUDIO__H

/*!
 * @file libAudio.h
 * @brief C API header for use by external programs
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2009-2020
 */

#include <stdint.h> // NOLINT:modernize-deprecated-headers
#include "fileInfo.hxx"

#ifdef _WINDOWS
	#ifdef libAUDIO
		#define libAUDIO_DEFAULT_VISIBILITY __declspec(dllexport)
		#pragma warning (disable : 4996)
	#else
		#define libAUDIO_DEFAULT_VISIBILITY __declspec(dllimport)
	#endif
	#define libAUDIO_API extern "C" libAUDIO_DEFAULT_VISIBILITY
	#define libAUDIO_CLS_API libAUDIO_DEFAULT_VISIBILITY
	#define libAUDIO_CLSMAYBE_API
	#define libAUDIO_CXX_API extern libAUDIO_DEFAULT_VISIBILITY
#else
	#if __GNUC__ >= 4
		#define libAUDIO_DEFAULT_VISIBILITY __attribute__ ((visibility("default")))
	#else
		#define libAUDIO_DEFAULT_VISIBILITY
	#endif
	#ifdef __cplusplus
		#define libAUDIO_API extern "C" libAUDIO_DEFAULT_VISIBILITY
		#define libAUDIO_CLS_API libAUDIO_DEFAULT_VISIBILITY
		#define libAUDIO_CLSMAYBE_API libAUDIO_CLS_API
		#define libAUDIO_CXX_API extern libAUDIO_CLS_API
	#else
		#define libAUDIO_API extern libAUDIO_DEFAULT_VISIBILITY
	#endif
#endif

#define libAudioVersion "0.5.2"
#define libAudioVersion_Major 0
#define libAudioVersion_Minor 5
#define libAudioVersion_Rev 2

// Ogg|Vorbis API
libAUDIO_API bool isOggVorbis(const char *fileName);
libAUDIO_API void *oggVorbisOpenR(const char *fileName);
libAUDIO_API void *oggVorbisOpenW(const char *fileName);

// Ogg|Opus API
libAUDIO_API bool isOggOpus(const char *fileName);
libAUDIO_API void *oggOpusOpenR(const char *fileName);
libAUDIO_API void *oggOpusOpenW(const char *fileName);

// FLAC API
libAUDIO_API bool isFLAC(const char *fileName);
libAUDIO_API void *flacOpenR(const char *fileName);
libAUDIO_API void *flacOpenW(const char *fileName);

// WAV(E) API
libAUDIO_API bool isWAV(const char *fileName);
libAUDIO_API void *wavOpenR(const char *fileName);

// M4A API
libAUDIO_API bool isM4A(const char *fileName);
libAUDIO_API void *m4aOpenR(const char *fileName);
libAUDIO_API void *m4aOpenW(const char *fileName);

// AAC API
libAUDIO_API bool isAAC(const char *fileName);
libAUDIO_API void *aacOpenR(const char *fileName);

// MP3 API
libAUDIO_API bool isMP3(const char *fileName);
libAUDIO_API void *mp3OpenR(const char *fileName);

// IT API
libAUDIO_API bool isIT(const char *fileName);
libAUDIO_API void *itOpenR(const char *fileName);

// MOD API
libAUDIO_API bool isMOD(const char *fileName);
libAUDIO_API void *modOpenR(const char *fileName);

// S3M API
libAUDIO_API bool isS3M(const char *fileName);
libAUDIO_API void *s3mOpenR(const char *fileName);

// STM API
libAUDIO_API bool isSTM(const char *fileName);
libAUDIO_API void *stmOpenR(const char *fileName);

#ifdef ENABLE_AON
// AON API
libAUDIO_API bool isAON(const char *fileName);
libAUDIO_API void *aonOpenR(const char *fileName);
#endif

#ifdef ENABLE_FC1x
// FC1x API
libAUDIO_API bool isFC1x(const char *fileName);
libAUDIO_API void *fc1xOpenR(const char *fileName);
#endif

// MPC API
libAUDIO_API bool isMPC(const char *fileName);
libAUDIO_API void *mpcOpenR(const char *fileName);

// WavPack API
libAUDIO_API bool isWavPack(const char *fileName);
libAUDIO_API void *wavPackOpenR(const char *fileName);

// SNDH API
libAUDIO_API bool isSNDH(const char *fileName);
libAUDIO_API void *sndhOpenR(const char *fileName);

#ifdef ENABLE_OptimFROG
// OptimFROG API
libAUDIO_API bool isOptimFROG(const char *fileName);
libAUDIO_API void *optimFROGOpenR(const char *fileName);
#endif

#ifdef ENABLE_RA
// RealAudio API
libAUDIO_API bool isRealAudio(const char *fileName);
libAUDIO_API void *realAudioOpenR(const char *fileName);
#endif

// WMA API
#ifdef ENABLE_WMA
libAUDIO_API bool isWMA(const char *fileName);
libAUDIO_API void *wmaOpenR(const char *fileName);
#endif

// Master Audio API

// General
libAUDIO_API int audioCloseFile(void *audioFile);

// Read (Decode)
libAUDIO_API void *audioOpenR(const char *const fileName);
libAUDIO_API const fileInfo_t *audioGetFileInfo(void *audioFile);
libAUDIO_API int64_t audioFillBuffer(void *audioFile, void *const buffer, const uint32_t length);

// Playback
libAUDIO_API void audioPlay(void *audioFile);
libAUDIO_API void audioPause(void *audioFile);
libAUDIO_API void audioStop(void *audioFile);
libAUDIO_API bool isAudio(const char *fileName);

libAUDIO_API void audioDefaultLevel(const float level);

// Write (Encode)
libAUDIO_API void *audioOpenW(const char *fileName, const uint32_t audioType);
libAUDIO_API bool audioSetFileInfo(void *audioFile, const fileInfo_t *const fileInfo);
libAUDIO_API int64_t audioWriteBuffer(void *audioFile, void *const buffer, int64_t length);

// Set this to a non-zero value if using your own payback routines. This must be set before any API calls.
libAUDIO_API uint8_t ExternalPlayback;

// Set this to zero if you do not want the genericModule/moduleMixer
// powered decoders to initialise a mixer instance
libAUDIO_API uint8_t ToPlayback;

// Master Audio API Defines

#define AUDIO_OGG_VORBIS	1
#define AUDIO_FLAC			2
#define AUDIO_WAVE			3
#define AUDIO_MP4			4
#define AUDIO_M4A			AUDIO_MP4
#define AUDIO_AAC			5
#define AUDIO_MP3			6
#define AUDIO_IT			7
#define AUDIO_MUSEPACK		8
#define AUDIO_WAVPACK		9
#define AUDIO_OPTIMFROG		10
#define AUDIO_REALAUDIO		11
#define AUDIO_WMA			12
#define AUDIO_MOD			13
#define AUDIO_S3M			14
#define AUDIO_STM			15
#define AUDIO_AON			16
#define AUDIO_FC1x			17
#define AUDIO_OGG_OPUS		18

#endif /*LIB_AUDIO__H*/
