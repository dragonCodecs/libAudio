#ifndef LIB_AUDIO__HXX
#define LIB_AUDIO__HXX

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
	#define libAUDIO_CXX_API extern
#else
	#if __GNUC__ >= 4
		#define libAUDIO_DEFAULT_VISIBILITY __attribute__ ((visibility("default")))
	#else
		#define libAUDIO_DEFAULT_VISIBILITY
	#endif
	#ifdef __cplusplus
		#define libAUDIO_API extern "C" libAUDIO_DEFAULT_VISIBILITY
		#define libAUDIO_CLS_API libAUDIO_DEFAULT_VISIBILITY
		#define libAUDIO_CXX_API extern libAUDIO_CLS_API
	#else
		#define libAUDIO_API extern libAUDIO_DEFAULT_VISIBILITY
	#endif
#endif

#define libAudioVersion "0.5.0"
#define libAudioVersion_Major 0
#define libAudioVersion_Minor 5
#define libAudioVersion_Rev 0

// Ogg|Vorbis API
libAUDIO_API bool Is_OggVorbis(const char *fileName);
libAUDIO_API void *OggVorbis_OpenR(const char *fileName);
libAUDIO_API void *OggVorbis_OpenW(const char *fileName);

libAUDIO_API bool OggVorbis_SetFileInfo(void *p_VorbisFile, const fileInfo_t *const p_FI);
libAUDIO_API long OggVorbis_WriteBuffer(void *p_VorbisFile, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int OggVorbis_CloseFileW(void *p_VorbisFile);

// Ogg|Opus API
libAUDIO_API bool Is_OggOpus(const char *fileName);
libAUDIO_API void *OggOpus_OpenR(const char *fileName);

// FLAC API
libAUDIO_API bool Is_FLAC(const char *fileName);
libAUDIO_API void *FLAC_OpenR(const char *fileName);
libAUDIO_API void *FLAC_OpenW(const char *fileName);

libAUDIO_API bool FLAC_SetFileInfo(void *flacFile, const fileInfo_t *const p_FI);
libAUDIO_API long FLAC_WriteBuffer(void *flacFile, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int FLAC_CloseFileW(void *flacFile);

// WAV(E) API
libAUDIO_API bool Is_WAV(const char *fileName);
libAUDIO_API void *WAV_OpenR(const char *fileName);

// M4A API
libAUDIO_API bool Is_M4A(const char *fileName);
libAUDIO_API void *M4A_OpenR(const char *fileName);
libAUDIO_API void *M4A_OpenW(const char *fileName);

libAUDIO_API bool M4A_SetFileInfo(void *m4aFile, const fileInfo_t *const p_FI);
libAUDIO_API long M4A_WriteBuffer(void *m4aFile, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int M4A_CloseFileW(void *m4aFile);

// AAC API
libAUDIO_API bool Is_AAC(const char *fileName);
libAUDIO_API void *AAC_OpenR(const char *fileName);

// MP3 API
libAUDIO_API bool Is_MP3(const char *fileName);
libAUDIO_API void *MP3_OpenR(const char *fileName);

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
libAUDIO_API bool Is_MPC(const char *fileName);
libAUDIO_API void *MPC_OpenR(const char *fileName);

// WavPack API
libAUDIO_API bool Is_WavPack(const char *fileName);
libAUDIO_API void *WavPack_OpenR(const char *fileName);

// SNDH API
libAUDIO_API bool Is_SNDH(const char *fileName);
libAUDIO_API void *SNDH_OpenR(const char *fileName);

#ifdef ENABLE_OptimFROG
// OptimFROG API
libAUDIO_API bool Is_OptimFROG(const char *fileName);
libAUDIO_API void *OptimFROG_OpenR(const char *fileName);
#endif

#ifdef ENABLE_RA
// RealAudio API
libAUDIO_API bool Is_RealAudio(const char *fileName);
libAUDIO_API void *RealAudio_OpenR(const char *fileName);
#endif

// WMA API
#ifdef ENABLE_WMA
libAUDIO_API bool Is_WMA(const char *fileName);
libAUDIO_API void *WMA_OpenR(const char *fileName);
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

// Write (Encode)
libAUDIO_API void *Audio_OpenW(const char *fileName, int Type);
libAUDIO_API bool Audio_SetFileInfo(void *audioFile, const fileInfo_t *const p_FI);
libAUDIO_API long Audio_WriteBuffer(void *audioFile, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int Audio_CloseFileW(void *audioFile);

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

#endif /*LIB_AUDIO__HXX*/
