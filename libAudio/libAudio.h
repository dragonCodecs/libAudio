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

#define libAudioVersion "0.4.0"
#define libAudioVersion_Major 0
#define libAudioVersion_Minor 4
#define libAudioVersion_Rev 0

// Ogg|Vorbis API

// Read/Playback
libAUDIO_API void *OggVorbis_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *OggVorbis_GetFileInfo(void *p_VorbisFile);
libAUDIO_API long OggVorbis_FillBuffer(void *p_VorbisFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int OggVorbis_CloseFileR(void *p_VorbisFile);
libAUDIO_API void OggVorbis_Play(void *p_VorbisFile);
libAUDIO_API void OggVorbis_Pause(void *p_VorbisFile);
libAUDIO_API void OggVorbis_Stop(void *p_VorbisFile);
libAUDIO_API bool Is_OggVorbis(const char *fileName);

// Write/Encode
libAUDIO_API void *OggVorbis_OpenW(const char *fileName);
libAUDIO_API bool OggVorbis_SetFileInfo(void *p_VorbisFile, const fileInfo_t *const p_FI);
libAUDIO_API long OggVorbis_WriteBuffer(void *p_VorbisFile, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int OggVorbis_CloseFileW(void *p_VorbisFile);

// Ogg|Opus API
libAUDIO_API void *OggOpus_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *OggOpus_GetFileInfo(void *opusFile);
libAUDIO_API long OggOpus_FillBuffer(void *opusFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int OggOpus_CloseFileR(void *opusFile);
libAUDIO_API void OggOpus_Play(void *opusFile);
libAUDIO_API void OggOpus_Pause(void *opusFile);
libAUDIO_API void OggOpus_Stop(void *opusFile);
libAUDIO_API bool Is_OggOpus(const char *fileName);

// FLAC API

// Read/Playback
libAUDIO_API void *FLAC_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *FLAC_GetFileInfo(void *flacFile);
libAUDIO_API long FLAC_FillBuffer(void *flacFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int FLAC_CloseFileR(void *flacFile);
libAUDIO_API void FLAC_Play(void *flacFile);
libAUDIO_API void FLAC_Pause(void *flacFile);
libAUDIO_API void FLAC_Stop(void *flacFile);
libAUDIO_API bool Is_FLAC(const char *fileName);

// Write/Encode
libAUDIO_API void *FLAC_OpenW(const char *fileName);
libAUDIO_API bool FLAC_SetFileInfo(void *flacFile, const fileInfo_t *const p_FI);
libAUDIO_API long FLAC_WriteBuffer(void *flacFile, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int FLAC_CloseFileW(void *flacFile);

// WAV(E) API

libAUDIO_API void *WAV_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *WAV_GetFileInfo(void *wavFile);
libAUDIO_API long WAV_FillBuffer(void *wavFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int WAV_CloseFileR(void *wavFile);
libAUDIO_API void WAV_Play(void *wavFile);
libAUDIO_API void WAV_Pause(void *wavFile);
libAUDIO_API void WAV_Stop(void *wavFile);
libAUDIO_API bool Is_WAV(const char *fileName);

// M4A API

// Read/Playback
libAUDIO_API void *M4A_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *M4A_GetFileInfo(void *m4aFile);
libAUDIO_API long M4A_FillBuffer(void *m4aFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int M4A_CloseFileR(void *m4aFile);
libAUDIO_API void M4A_Play(void *m4aFile);
libAUDIO_API void M4A_Pause(void *m4aFile);
libAUDIO_API void M4A_Stop(void *m4aFile);
libAUDIO_API bool Is_M4A(const char *fileName);

// Write/Encode
libAUDIO_API void *M4A_OpenW(const char *fileName);
libAUDIO_API bool M4A_SetFileInfo(void *m4aFile, const fileInfo_t *const p_FI);
libAUDIO_API long M4A_WriteBuffer(void *m4aFile, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int M4A_CloseFileW(void *m4aFile);

// AAC API
libAUDIO_API void *AAC_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *AAC_GetFileInfo(void *p_AACFile);
libAUDIO_API long AAC_FillBuffer(void *p_AACFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int AAC_CloseFileR(void *p_AACFile);
libAUDIO_API void AAC_Play(void *p_AACFile);
libAUDIO_API void AAC_Pause(void *p_AACFile);
libAUDIO_API void AAC_Stop(void *p_AACFile);
libAUDIO_API bool Is_AAC(const char *fileName);

// MP3 API

libAUDIO_API void *MP3_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *MP3_GetFileInfo(void *p_MP3File);
libAUDIO_API long MP3_FillBuffer(void *p_MP3File, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int MP3_CloseFileR(void *p_MP3File);
libAUDIO_API void MP3_Play(void *p_MP3File);
libAUDIO_API void MP3_Pause(void *p_MP3File);
libAUDIO_API void MP3_Stop(void *p_MP3File);
libAUDIO_API bool Is_MP3(const char *fileName);

// IT API

libAUDIO_API void *IT_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *IT_GetFileInfo(void *p_ITFile);
libAUDIO_API long IT_FillBuffer(void *p_ITFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int IT_CloseFileR(void *p_ITFile);
libAUDIO_API void IT_Play(void *p_ITFile);
libAUDIO_API void IT_Pause(void *p_ITFile);
libAUDIO_API void IT_Stop(void *p_ITFile);
libAUDIO_API bool Is_IT(const char *fileName);

// MOD API

libAUDIO_API void *MOD_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *MOD_GetFileInfo(void *p_MODFile);
libAUDIO_API long MOD_FillBuffer(void *p_MODFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int MOD_CloseFileR(void *p_MODFile);
libAUDIO_API void MOD_Play(void *p_MODFile);
libAUDIO_API void MOD_Pause(void *p_MODFile);
libAUDIO_API void MOD_Stop(void *p_MODFile);
libAUDIO_API bool Is_MOD(const char *fileName);

// S3M API

libAUDIO_API void *S3M_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *S3M_GetFileInfo(void *p_S3MFile);
libAUDIO_API long S3M_FillBuffer(void *p_S3MFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int S3M_CloseFileR(void *p_S3MFile);
libAUDIO_API void S3M_Play(void *p_S3MFile);
libAUDIO_API void S3M_Pause(void *p_S3MFile);
libAUDIO_API void S3M_Stop(void *p_S3MFile);
libAUDIO_API bool Is_S3M(const char *fileName);

// STM API

libAUDIO_API void *STM_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *STM_GetFileInfo(void *p_STMFile);
libAUDIO_API long STM_FillBuffer(void *p_STMFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int STM_CloseFileR(void *p_STMFile);
libAUDIO_API void STM_Play(void *p_STMFile);
libAUDIO_API void STM_Pause(void *p_STMFile);
libAUDIO_API void STM_Stop(void *p_STMFile);
libAUDIO_API bool Is_STM(const char *fileName);

// AON API

libAUDIO_API void *AON_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *AON_GetFileInfo(void *p_AONFile);
libAUDIO_API long AON_FillBuffer(void *p_AONFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int AON_CloseFileR(void *p_AONFile);
libAUDIO_API void AON_Play(void *p_AONFile);
libAUDIO_API void AON_Pause(void *p_AONFile);
libAUDIO_API void AON_Stop(void *p_AONFile);
libAUDIO_API bool Is_AON(const char *fileName);

#ifdef ENABLE_FC1x
// FC1x API

libAUDIO_API void *FC1x_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *FC1x_GetFileInfo(void *p_FC1xFile);
libAUDIO_API long FC1x_FillBuffer(void *p_FC1xFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int FC1x_CloseFileR(void *p_FC1xFile);
libAUDIO_API void FC1x_Play(void *p_FC1xFile);
libAUDIO_API void FC1x_Pause(void *p_FC1xFile);
libAUDIO_API void FC1x_Stop(void *p_FC1xFile);
libAUDIO_API bool Is_FC1x(const char *fileName);
#endif

// MPC API

libAUDIO_API void *MPC_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *MPC_GetFileInfo(void *p_MPCFile);
libAUDIO_API long MPC_FillBuffer(void *p_MPCFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int MPC_CloseFileR(void *p_MPCFile);
libAUDIO_API void MPC_Play(void *p_MPCFile);
libAUDIO_API void MPC_Pause(void *p_MPCFile);
libAUDIO_API void MPC_Stop(void *p_MPCFile);
libAUDIO_API bool Is_MPC(const char *fileName);

// WavPack API

libAUDIO_API void *WavPack_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *WavPack_GetFileInfo(void *p_WVPFile);
libAUDIO_API long WavPack_FillBuffer(void *p_WVPFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int WavPack_CloseFileR(void *p_WVPFile);
libAUDIO_API void WavPack_Play(void *p_WVPFile);
libAUDIO_API void WavPack_Pause(void *p_WVPFile);
libAUDIO_API void WavPack_Stop(void *p_WVPFile);
libAUDIO_API bool Is_WavPack(const char *fileName);

// SNDH API

libAUDIO_API void *SNDH_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *SNDH_GetFileInfo(void *p_SNDHFile);
libAUDIO_API long SNDH_FillBuffer(void *p_SNDHFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int SNDH_CloseFileR(void *p_SNDHFile);
libAUDIO_API void SNDH_Play(void *p_SNDHFile);
libAUDIO_API void SNDH_Pause(void *p_SNDHFile);
libAUDIO_API void SNDH_Stop(void *p_SNDHFile);
libAUDIO_API bool Is_SNDH(const char *fileName);

// OptimFROG API

libAUDIO_API void *OptimFROG_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *OptimFROG_GetFileInfo(void *p_OFGFile);
libAUDIO_API long OptimFROG_FillBuffer(void *p_OFGFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int OptimFROG_CloseFileR(void *p_OFGFile);
libAUDIO_API void OptimFROG_Play(void *p_OFGFile);
libAUDIO_API void OptimFROG_Pause(void *p_OFGFile);
libAUDIO_API void OptimFROG_Stop(void *p_OFGFile);
libAUDIO_API bool Is_OptimFROG(const char *fileName);

// RealAudio API

libAUDIO_API void *RealAudio_OpenR(const char *fileName);
//
libAUDIO_API bool Is_RealAudio(const char *fileName);

// WMA API

libAUDIO_API void *WMA_OpenR(const char *fileName);
libAUDIO_API const fileInfo_t *WMA_GetFileInfo(void *p_WMAFile);
libAUDIO_API long WMA_FillBuffer(void *p_WMAFile, uint8_t *OutBuffer, int nOutBufferLen);
//
libAUDIO_API void WMA_Play(void *p_WMAFile);
libAUDIO_API void WMA_Pause(void *p_WMAFile);
libAUDIO_API void WMA_Stop(void *p_WMAFile);
libAUDIO_API bool Is_WMA(const char *fileName);

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
libAUDIO_API bool Is_Audio(const char *fileName);

// Write (Encode)
libAUDIO_API void *Audio_OpenW(const char *fileName, int Type);
libAUDIO_API bool Audio_SetFileInfo(void *p_AudioPtr, const fileInfo_t *const p_FI);
libAUDIO_API long Audio_WriteBuffer(void *p_AudioPtr, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int Audio_CloseFileW(void *p_AudioPtr);

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
