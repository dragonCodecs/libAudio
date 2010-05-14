#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <vector>

typedef struct FileInfo
{
	double TotalTime;
	long BitsPerSample;
	long BitRate;
	int Channels;
	//int BitStream;
	char *Title;
	char *Artist;
	char *Album;
	std::vector<char *> OtherComments;
	int nOtherComments;
} FileInfo;

#ifdef _WINDOWS
	#ifdef libAUDIO
		#define libAUDIO_API extern
		#pragma warning (disable : 4996)
	#else
		#define libAUDIO_API __declspec(dllimport)
	#endif
#else
	#ifdef libAUDIO
		#define libAUDIO_API __attribute__((dllexport)) extern
	#else
		#define libAUDIO_API __attribute__((dllimport)) extern
	#endif
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef UINT
typedef unsigned int UINT;
#endif

#define libAudioVersion "0.0.1.0"
#define libAudioVersion_Major 0
#define libAudioVersion_Minor 0
#define libAudioVersion_Rev_Major 1
#define libAudioVersion_Rev_Minor 0

// OggVorbis API

// Read/Playback
libAUDIO_API void *OggVorbis_OpenR(char *FileName);
libAUDIO_API FileInfo *OggVorbis_GetFileInfo(void *p_VorbisFile);
libAUDIO_API long OggVorbis_FillBuffer(void *p_VorbisFile, BYTE *OutBuffer, int nOutBufferLen);
libAUDIO_API int OggVorbis_CloseFileR(void *p_VorbisFile);
libAUDIO_API void OggVorbis_Play(void *p_VorbisFile);
libAUDIO_API bool Is_OggVorbis(char *FileName);

// Write/Encode
libAUDIO_API void *OggVorbis_OpenW(char *FileName);
libAUDIO_API void OggVorbis_SetFileInfo(void *p_VorbisFile, FileInfo *p_FI);
libAUDIO_API long OggVorbis_WriteBuffer(void *p_VorbisFile, BYTE *InBuffer, int nInBufferLen);
libAUDIO_API int OggVorbis_CloseFileW(void *p_VorbisFile);

// FLAC API

// Read/Playback
libAUDIO_API void *FLAC_OpenR(char *FileName);
libAUDIO_API FileInfo *FLAC_GetFileInfo(void *p_FLACFile);
libAUDIO_API long FLAC_FillBuffer(void *p_FLACFile, BYTE *OutBuffer, int nOutBufferLen);
libAUDIO_API int FLAC_CloseFileR(void *p_FLACFile);
libAUDIO_API void FLAC_Play(void *p_FLACFile);
libAUDIO_API bool Is_FLAC(char *FileName);

// Write/Encode
libAUDIO_API void *FLAC_OpenW(char *FileName);
libAUDIO_API void FLAC_SetFileInfo(void *p_FLACFile, FileInfo *p_FI);
libAUDIO_API long FLAC_WriteBuffer(void *p_FLACFile, BYTE *InBuffer, int nInBufferLen);
libAUDIO_API int FLAC_CloseFileW(void *p_FLACFile);

// WAV(E) API

libAUDIO_API void *WAV_OpenR(char *FileName);
libAUDIO_API FileInfo *WAV_GetFileInfo(void *p_WAVFile);
libAUDIO_API long WAV_FillBuffer(void *p_WAVFile, BYTE *OutBuffer, int nOutBufferLen);
libAUDIO_API int WAV_CloseFileR(void *p_WAVFile);
libAUDIO_API void WAV_Play(void *p_WAVFile);
libAUDIO_API bool Is_WAV(char *FileName);

// M4A API

// Read/Playback
libAUDIO_API void *M4A_OpenR(char *FileName);
libAUDIO_API FileInfo *M4A_GetFileInfo(void *p_M4AFile);
libAUDIO_API long M4A_FillBuffer(void *p_M4AFile, BYTE *OutBuffer, int nOutBufferLen);
libAUDIO_API int M4A_CloseFileR(void *p_M4AFile);
libAUDIO_API void M4A_Play(void *p_M4AFile);
libAUDIO_API bool Is_M4A(char *FileName);

// Write/Encode
libAUDIO_API void *M4A_OpenW(char *FileName);
libAUDIO_API void M4A_SetFileInfo(void *p_M4AFile, FileInfo *p_FI);
libAUDIO_API long M4A_WriteBuffer(void *p_M4AFile, BYTE *InBuffer, int nInBufferLen);
libAUDIO_API int M4A_CloseFileW(void *p_M4AFile);

// AAC API
libAUDIO_API void *AAC_OpenR(char *FileName);
libAUDIO_API FileInfo *AAC_GetFileInfo(void *p_AACFile);
libAUDIO_API long AAC_FillBuffer(void *p_AACFile, BYTE *OutBuffer, int nOutBufferLen);
libAUDIO_API int AAC_CloseFileR(void *p_AACFile);
libAUDIO_API void AAC_Play(void *p_AACFile);
libAUDIO_API bool Is_AAC(char *FileName);

// MP3 API

libAUDIO_API void *MP3_OpenR(char *FileName);
libAUDIO_API FileInfo *MP3_GetFileInfo(void *p_MP3File);
libAUDIO_API long MP3_FillBuffer(void *p_MP3File, BYTE *OutBuffer, int nOutBufferLen);
libAUDIO_API int MP3_CloseFileR(void *p_MP3File);
libAUDIO_API void MP3_Play(void *p_MP3File);
libAUDIO_API bool Is_MP3(char *FileName);

// IT API

libAUDIO_API void *IT_OpenR(char *FileName);
libAUDIO_API FileInfo *IT_GetFileInfo(void *p_ITFile);
libAUDIO_API long IT_FillBuffer(void *ITFile, BYTE *OutBuffer, int nOutBufferLen);
libAUDIO_API int IT_CloseFileR(void *ITFile);
libAUDIO_API void IT_Play(void *IT_File);
libAUDIO_API bool Is_IT(char *FileName);

// MPC API

libAUDIO_API void *MPC_OpenR(char *FileName);
libAUDIO_API FileInfo *MPC_GetFileInfo(void *p_MPCFile);
libAUDIO_API long MPC_FillBuffer(void *p_MPCFile, BYTE *OutBuffer, int nOutBufferLen);
//
libAUDIO_API void MPC_Play(void *p_MPCFile);
libAUDIO_API bool Is_MPC(char *FileName);

// WavPack API

libAUDIO_API void *WavPack_OpenR(char *FileName);
libAUDIO_API FileInfo *WavPack_GetFileInfo(void *p_WVPFile);
libAUDIO_API long WavPack_FillBuffer(void *p_WVPFile, BYTE *OutBuffer, int nOutBufferLen);
libAUDIO_API int WavPack_CloseFileR(void *p_WVPFile);
libAUDIO_API void WavPack_Play(void *p_WVPFile);
libAUDIO_API bool Is_WavPack(char *FileName);

// OptimFROG API

libAUDIO_API void *OptimFROG_OpenR(char *FileName);
libAUDIO_API FileInfo *OptimFROG_GetFileInfo(void *p_OFGFile);
libAUDIO_API long OptimFROG_FillBuffer(void *p_OFGFile, BYTE *OutBuffer, int nOutBufferLen);
libAUDIO_API int OptimFROG_CloseFileR(void *p_OFGFile);
libAUDIO_API void OptimFROG_Play(void *p_OFGFile);
libAUDIO_API bool Is_OptimFROG(char *FileName);

// RealAudio API

libAUDIO_API void *RealAudio_OpenR(char *FileName);
//
libAUDIO_API bool Is_RealAudio(char *FileName);

// WMA API

libAUDIO_API void *WMA_OpenR(char *FileName);
libAUDIO_API FileInfo *WMA_GetFileInfo(void *p_WMAFile);
libAUDIO_API long WMA_FillBuffer(void *p_WMAFile, BYTE *OutBuffer, int nOutBufferLen);
//
libAUDIO_API void WMA_Play(void *p_WMAFile);
libAUDIO_API bool Is_WMA(char *FileName);

// Master Audio API

// Read/Playback
libAUDIO_API void *Audio_OpenR(char *FileName, int *Type);
libAUDIO_API FileInfo *Audio_GetFileInfo(void *p_AudioPtr, int Type);
libAUDIO_API long Audio_FillBuffer(void *p_AudioPtr, BYTE *OutBuffer, int nOutBufferLen, int Type);
libAUDIO_API int Audio_CloseFileR(void *p_AudioPtr, int Type);
libAUDIO_API void Audio_Play(void *p_AudioPtr, int Type);
libAUDIO_API bool Is_Audio(char *FileName);

// Write/Encode
libAUDIO_API void *Audio_OpenW(char *FileName, int Type);
libAUDIO_API void Audio_SetFileInfo(void *p_AudioPtr, FileInfo *p_FI, int Type);
libAUDIO_API long Audio_WriteBuffer(void *p_AudioPtr, BYTE *InBuffer, int nInBufferLen, int Type);
libAUDIO_API int Audio_CloseFileW(void *p_AudioPtr, int Type);

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