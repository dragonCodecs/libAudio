// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2009-2023 Rachel Mant <git@dragonmux.network>
#ifndef LIB_AUDIO_H
#define LIB_AUDIO_H

/*!
 * @file libAudio.h
 * @brief C API header for use by external programs
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2009-2020
 */

#include <stdint.h> // NOLINT(modernize-deprecated-headers,hicpp-deprecated-headers)
#include "libAudioConfig.h"

#ifdef _WINDOWS
	#ifdef libAUDIO
		// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
		#define libAUDIO_DEFAULT_VISIBILITY __declspec(dllexport)
		#pragma warning (disable : 4996)
	#else
		// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
		#define libAUDIO_DEFAULT_VISIBILITY __declspec(dllimport)
	#endif
	// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
	#define libAUDIO_API extern "C" libAUDIO_DEFAULT_VISIBILITY
	// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
	#define libAUDIO_CLS_API libAUDIO_DEFAULT_VISIBILITY
	// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
	#define libAUDIO_CLSMAYBE_API
	// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
	#define libAUDIO_CXX_API extern libAUDIO_DEFAULT_VISIBILITY
#else
	#if __GNUC__ >= 4
		// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
		#define libAUDIO_DEFAULT_VISIBILITY __attribute__ ((visibility("default")))
	#else
		// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
		#define libAUDIO_DEFAULT_VISIBILITY
	#endif
	#ifdef __cplusplus
		// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
		#define libAUDIO_API extern "C" libAUDIO_DEFAULT_VISIBILITY
		// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
		#define libAUDIO_CLS_API libAUDIO_DEFAULT_VISIBILITY
		// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
		#define libAUDIO_CLSMAYBE_API libAUDIO_CLS_API
		// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
		#define libAUDIO_CXX_API extern libAUDIO_CLS_API
	#else
		// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
		#define libAUDIO_API extern libAUDIO_DEFAULT_VISIBILITY
	#endif
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define libAudioVersion "0.5.2"
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define libAudioVersion_Major 0
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define libAudioVersion_Minor 5
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define libAudioVersion_Rev 2

struct fileInfo_t;

#ifdef ENABLE_VORBIS
// Ogg|Vorbis API
libAUDIO_API bool isOggVorbis(const char *fileName);
libAUDIO_API void *oggVorbisOpenR(const char *fileName);
libAUDIO_API void *oggVorbisOpenW(const char *fileName);
#endif

#ifdef ENABLE_OPUS
// Ogg|Opus API
libAUDIO_API bool isOggOpus(const char *fileName);
libAUDIO_API void *oggOpusOpenR(const char *fileName);
libAUDIO_API void *oggOpusOpenW(const char *fileName);
#endif

#ifdef ENABLE_FLAC
// FLAC API
libAUDIO_API bool isFLAC(const char *fileName);
libAUDIO_API void *flacOpenR(const char *fileName);
libAUDIO_API void *flacOpenW(const char *fileName);
#endif

// WAV(E) API
libAUDIO_API bool isWAV(const char *fileName);
libAUDIO_API void *wavOpenR(const char *fileName);

#ifdef ENABLE_M4A
// M4A API
libAUDIO_API bool isM4A(const char *fileName);
libAUDIO_API void *m4aOpenR(const char *fileName);
libAUDIO_API void *m4aOpenW(const char *fileName);
#endif

#ifdef ENABLE_AAC
// AAC API
libAUDIO_API bool isAAC(const char *fileName);
libAUDIO_API void *aacOpenR(const char *fileName);
#endif

#ifdef ENABLE_MP3
// MP3 API
libAUDIO_API bool isMP3(const char *fileName);
libAUDIO_API void *mp3OpenR(const char *fileName);
libAUDIO_API void *mp3OpenW(const char *fileName);
#endif

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

#ifdef ENABLE_MUSEPACK
// MPC API
libAUDIO_API bool isMPC(const char *fileName);
libAUDIO_API void *mpcOpenR(const char *fileName);
#endif

#ifdef ENABLE_WAVPACK
// WavPack API
libAUDIO_API bool isWavPack(const char *fileName);
libAUDIO_API void *wavPackOpenR(const char *fileName);
#endif

#ifdef ENABLE_SNDH
// SNDH API
libAUDIO_API bool isSNDH(const char *fileName);
libAUDIO_API void *sndhOpenR(const char *fileName);
#endif

#ifdef ENABLE_SID
// SID API
libAUDIO_API bool isSID(const char *fileName);
libAUDIO_API void *sidOpenR(const char *fileName);
#endif

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
libAUDIO_API void *audioOpenR(const char *fileName);
libAUDIO_API const fileInfo_t *audioGetFileInfo(void *audioFile);
libAUDIO_API int64_t audioFillBuffer(void *audioFile, void *buffer, uint32_t length);

// Playback
libAUDIO_API void audioPlay(void *audioFile);
libAUDIO_API void audioPause(void *audioFile);
libAUDIO_API void audioStop(void *audioFile);
libAUDIO_API bool isAudio(const char *fileName);

libAUDIO_API void audioDefaultLevel(float level);

// Write (Encode)
libAUDIO_API void *audioOpenW(const char *fileName, uint32_t audioType);
libAUDIO_API bool audioSetFileInfo(void *audioFile, const fileInfo_t *fileInfo);
libAUDIO_API int64_t audioWriteBuffer(void *audioFile, const void *buffer, int64_t length);

libAUDIO_API uint64_t audioFileTotalTime(const fileInfo_t *fileInfo);
libAUDIO_API uint32_t audioFileBitsPerSample(const fileInfo_t *fileInfo);
libAUDIO_API uint32_t audioFileBitRate(const fileInfo_t *fileInfo);
libAUDIO_API uint8_t audioFileChannels(const fileInfo_t *fileInfo);

// Set this to a non-zero value if using your own payback routines. This must be set before any API calls.
// cppcoreguidelines-avoid-non-const-global-variables
libAUDIO_API uint8_t ExternalPlayback;

// Set this to zero if you do not want the genericModule/moduleMixer
// powered decoders to initialise a mixer instance
// cppcoreguidelines-avoid-non-const-global-variables
libAUDIO_API uint8_t ToPlayback;

// Master Audio API Defines

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_OGG_VORBIS	1
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_FLAC			2
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_WAVE			3
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_MP4			4
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_M4A			AUDIO_MP4
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_AAC			5
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_MP3			6
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_IT			7
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_MUSEPACK		8
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_WAVPACK		9
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_OPTIMFROG		10
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_REALAUDIO		11
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_WMA			12
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_MOD			13
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_S3M			14
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_STM			15
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_AON			16
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_FC1x			17
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AUDIO_OGG_OPUS		18

#endif /*LIB_AUDIO_H*/
