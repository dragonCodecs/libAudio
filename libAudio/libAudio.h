#ifndef LIB_AUDIO__HXX
#define LIB_AUDIO__HXX

#include <stdint.h>
#include "fileInfo.hxx"

#ifdef _WINDOWS
	#ifdef libAUDIO
		#define libAUDIO_API extern
		#pragma warning (disable : 4996)
	#else
		#define libAUDIO_API __declspec(dllimport)
	#endif
#else
	#if __GNUC__ >= 4
		#define DEFAULT_VISIBILITY __attribute__ ((visibility("default")))
	#else
		#define DEFAULT_VISIBILITY
	#endif
	#ifdef __cplusplus
		#define libAUDIO_API extern "C" DEFAULT_VISIBILITY
	#else
		#define libAUDIO_API extern DEFAULT_VISIBILITY
	#endif
#endif

#define libAudioVersion "0.3.1"
#define libAudioVersion_Major 0
#define libAudioVersion_Minor 3
#define libAudioVersion_Rev 1

// OggVorbis API

// Read/Playback
libAUDIO_API void *OggVorbis_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *OggVorbis_GetFileInfo(void *p_VorbisFile);
libAUDIO_API long OggVorbis_FillBuffer(void *p_VorbisFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int OggVorbis_CloseFileR(void *p_VorbisFile);
libAUDIO_API void OggVorbis_Play(void *p_VorbisFile);
libAUDIO_API void OggVorbis_Pause(void *p_VorbisFile);
libAUDIO_API void OggVorbis_Stop(void *p_VorbisFile);
libAUDIO_API bool Is_OggVorbis(const char *FileName);

// Write/Encode
libAUDIO_API void *OggVorbis_OpenW(const char *FileName);
libAUDIO_API bool OggVorbis_SetFileInfo(void *p_VorbisFile, const fileInfo_t *const p_FI);
libAUDIO_API long OggVorbis_WriteBuffer(void *p_VorbisFile, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int OggVorbis_CloseFileW(void *p_VorbisFile);

// FLAC API

// Read/Playback
libAUDIO_API void *FLAC_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *FLAC_GetFileInfo(void *p_FLACFile);
libAUDIO_API long FLAC_FillBuffer(void *p_FLACFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int FLAC_CloseFileR(void *p_FLACFile);
libAUDIO_API void FLAC_Play(void *p_FLACFile);
libAUDIO_API void FLAC_Pause(void *p_FLACFile);
libAUDIO_API void FLAC_Stop(void *p_FLACFile);
libAUDIO_API bool Is_FLAC(const char *FileName);

// Write/Encode
libAUDIO_API void *FLAC_OpenW(const char *FileName);
libAUDIO_API bool FLAC_SetFileInfo(void *p_FLACFile, const fileInfo_t *const p_FI);
libAUDIO_API long FLAC_WriteBuffer(void *p_FLACFile, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int FLAC_CloseFileW(void *p_FLACFile);

// WAV(E) API

libAUDIO_API void *WAV_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *WAV_GetFileInfo(void *p_WAVFile);
libAUDIO_API long WAV_FillBuffer(void *p_WAVFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int WAV_CloseFileR(void *p_WAVFile);
libAUDIO_API void WAV_Play(void *p_WAVFile);
libAUDIO_API void WAV_Pause(void *p_WAVFile);
libAUDIO_API void WAV_Stop(void *p_WAVFile);
libAUDIO_API bool Is_WAV(const char *FileName);

// M4A API

// Read/Playback
libAUDIO_API void *M4A_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *M4A_GetFileInfo(void *p_M4AFile);
libAUDIO_API long M4A_FillBuffer(void *p_M4AFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int M4A_CloseFileR(void *p_M4AFile);
libAUDIO_API void M4A_Play(void *p_M4AFile);
libAUDIO_API void M4A_Pause(void *p_M4AFile);
libAUDIO_API void M4A_Stop(void *p_M4AFile);
libAUDIO_API bool Is_M4A(const char *FileName);

// Write/Encode
libAUDIO_API void *M4A_OpenW(const char *FileName);
libAUDIO_API bool M4A_SetFileInfo(void *p_M4AFile, const fileInfo_t *const p_FI);
libAUDIO_API long M4A_WriteBuffer(void *p_M4AFile, uint8_t *InBuffer, int nInBufferLen);
libAUDIO_API int M4A_CloseFileW(void *p_M4AFile);

// AAC API
libAUDIO_API void *AAC_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *AAC_GetFileInfo(void *p_AACFile);
libAUDIO_API long AAC_FillBuffer(void *p_AACFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int AAC_CloseFileR(void *p_AACFile);
libAUDIO_API void AAC_Play(void *p_AACFile);
libAUDIO_API void AAC_Pause(void *p_AACFile);
libAUDIO_API void AAC_Stop(void *p_AACFile);
libAUDIO_API bool Is_AAC(const char *FileName);

// MP3 API

libAUDIO_API void *MP3_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *MP3_GetFileInfo(void *p_MP3File);
libAUDIO_API long MP3_FillBuffer(void *p_MP3File, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int MP3_CloseFileR(void *p_MP3File);
libAUDIO_API void MP3_Play(void *p_MP3File);
libAUDIO_API void MP3_Pause(void *p_MP3File);
libAUDIO_API void MP3_Stop(void *p_MP3File);
libAUDIO_API bool Is_MP3(const char *FileName);

// IT API

libAUDIO_API void *IT_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *IT_GetFileInfo(void *p_ITFile);
libAUDIO_API long IT_FillBuffer(void *p_ITFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int IT_CloseFileR(void *p_ITFile);
libAUDIO_API void IT_Play(void *p_ITFile);
libAUDIO_API void IT_Pause(void *p_ITFile);
libAUDIO_API void IT_Stop(void *p_ITFile);
libAUDIO_API bool Is_IT(const char *FileName);

// MOD API

libAUDIO_API void *MOD_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *MOD_GetFileInfo(void *p_MODFile);
libAUDIO_API long MOD_FillBuffer(void *p_MODFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int MOD_CloseFileR(void *p_MODFile);
libAUDIO_API void MOD_Play(void *p_MODFile);
libAUDIO_API void MOD_Pause(void *p_MODFile);
libAUDIO_API void MOD_Stop(void *p_MODFile);
libAUDIO_API bool Is_MOD(const char *FileName);

// S3M API

libAUDIO_API void *S3M_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *S3M_GetFileInfo(void *p_S3MFile);
libAUDIO_API long S3M_FillBuffer(void *p_S3MFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int S3M_CloseFileR(void *p_S3MFile);
libAUDIO_API void S3M_Play(void *p_S3MFile);
libAUDIO_API void S3M_Pause(void *p_S3MFile);
libAUDIO_API void S3M_Stop(void *p_S3MFile);
libAUDIO_API bool Is_S3M(const char *FileName);

// STM API

libAUDIO_API void *STM_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *STM_GetFileInfo(void *p_STMFile);
libAUDIO_API long STM_FillBuffer(void *p_STMFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int STM_CloseFileR(void *p_STMFile);
libAUDIO_API void STM_Play(void *p_STMFile);
libAUDIO_API void STM_Pause(void *p_STMFile);
libAUDIO_API void STM_Stop(void *p_STMFile);
libAUDIO_API bool Is_STM(const char *FileName);

// AON API

libAUDIO_API void *AON_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *AON_GetFileInfo(void *p_AONFile);
libAUDIO_API long AON_FillBuffer(void *p_AONFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int AON_CloseFileR(void *p_AONFile);
libAUDIO_API void AON_Play(void *p_AONFile);
libAUDIO_API void AON_Pause(void *p_AONFile);
libAUDIO_API void AON_Stop(void *p_AONFile);
libAUDIO_API bool Is_AON(const char *FileName);

// FC1x API

libAUDIO_API void *FC1x_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *FC1x_GetFileInfo(void *p_FC1xFile);
libAUDIO_API long FC1x_FillBuffer(void *p_FC1xFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int FC1x_CloseFileR(void *p_FC1xFile);
libAUDIO_API void FC1x_Play(void *p_FC1xFile);
libAUDIO_API void FC1x_Pause(void *p_FC1xFile);
libAUDIO_API void FC1x_Stop(void *p_FC1xFile);
libAUDIO_API bool Is_FC1x(const char *FileName);

// MPC API

libAUDIO_API void *MPC_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *MPC_GetFileInfo(void *p_MPCFile);
libAUDIO_API long MPC_FillBuffer(void *p_MPCFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int MPC_CloseFileR(void *p_MPCFile);
libAUDIO_API void MPC_Play(void *p_MPCFile);
libAUDIO_API void MPC_Pause(void *p_MPCFile);
libAUDIO_API void MPC_Stop(void *p_MPCFile);
libAUDIO_API bool Is_MPC(const char *FileName);

// WavPack API

libAUDIO_API void *WavPack_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *WavPack_GetFileInfo(void *p_WVPFile);
libAUDIO_API long WavPack_FillBuffer(void *p_WVPFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int WavPack_CloseFileR(void *p_WVPFile);
libAUDIO_API void WavPack_Play(void *p_WVPFile);
libAUDIO_API void WavPack_Pause(void *p_WVPFile);
libAUDIO_API void WavPack_Stop(void *p_WVPFile);
libAUDIO_API bool Is_WavPack(const char *FileName);

// OptimFROG API

libAUDIO_API void *OptimFROG_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *OptimFROG_GetFileInfo(void *p_OFGFile);
libAUDIO_API long OptimFROG_FillBuffer(void *p_OFGFile, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int OptimFROG_CloseFileR(void *p_OFGFile);
libAUDIO_API void OptimFROG_Play(void *p_OFGFile);
libAUDIO_API void OptimFROG_Pause(void *p_OFGFile);
libAUDIO_API void OptimFROG_Stop(void *p_OFGFile);
libAUDIO_API bool Is_OptimFROG(const char *FileName);

// RealAudio API

libAUDIO_API void *RealAudio_OpenR(const char *FileName);
//
libAUDIO_API bool Is_RealAudio(const char *FileName);

// WMA API

libAUDIO_API void *WMA_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *WMA_GetFileInfo(void *p_WMAFile);
libAUDIO_API long WMA_FillBuffer(void *p_WMAFile, uint8_t *OutBuffer, int nOutBufferLen);
//
libAUDIO_API void WMA_Play(void *p_WMAFile);
libAUDIO_API void WMA_Pause(void *p_WMAFile);
libAUDIO_API void WMA_Stop(void *p_WMAFile);
libAUDIO_API bool Is_WMA(const char *FileName);

// Master Audio API

// Read/Playback
libAUDIO_API void *Audio_OpenR(const char *FileName);
libAUDIO_API const fileInfo_t *Audio_GetFileInfo(void *p_AudioPtr);
libAUDIO_API long Audio_FillBuffer(void *p_AudioPtr, uint8_t *OutBuffer, int nOutBufferLen);
libAUDIO_API int Audio_CloseFileR(void *p_AudioPtr);
libAUDIO_API void Audio_Play(void *p_AudioPtr);
libAUDIO_API void Audio_Pause(void *p_AudioPtr);
libAUDIO_API void Audio_Stop(void *p_AudioPtr);
libAUDIO_API bool Is_Audio(const char *FileName);

// Write/Encode
libAUDIO_API void *Audio_OpenW(const char *FileName, int Type);
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

#endif /*LIB_AUDIO__HXX*/
