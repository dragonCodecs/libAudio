#include "libAudio.h"

BYTE ExternalPlayback = 0;

void *Audio_OpenR(const char *FileName, int *Type)
{
	if (Is_OggVorbis(FileName) == true)
	{
		*Type = AUDIO_OGG_VORBIS;
		return OggVorbis_OpenR(FileName);
	}
	else if (Is_FLAC(FileName) == true)
	{
		*Type = AUDIO_FLAC;
		return FLAC_OpenR(FileName);
	}
	else if (Is_WAV(FileName) == true)
	{
		*Type = AUDIO_WAVE;
		return WAV_OpenR(FileName);
	}
	else if (Is_M4A(FileName) == true)
	{
		*Type = AUDIO_M4A;
		return M4A_OpenR(FileName);
	}
	else if (Is_AAC(FileName) == true)
	{
		*Type = AUDIO_AAC;
		return AAC_OpenR(FileName);
	}
	else if (Is_MP3(FileName) == true)
	{
		*Type = AUDIO_MP3;
		return MP3_OpenR(FileName);
	}
#ifndef __NO_IT__
	else if (Is_IT(FileName) == true)
	{
		*Type = AUDIO_IT;
		return IT_OpenR(FileName);
	}
#endif
	else if (Is_MOD(FileName) == true)
	{
		*Type = AUDIO_MOD;
		return MOD_OpenR(FileName);
	}
#ifndef __NO_MPC__
	else if (Is_MPC(FileName) == true)
	{
		*Type = AUDIO_MUSEPACK;
		return MPC_OpenR(FileName);
	}
#endif
	else if (Is_WavPack(FileName) == true)
	{
		*Type = AUDIO_WAVPACK;
		return WavPack_OpenR(FileName);
	}
#ifndef __NO_OptimFROG__
	else if (Is_OptimFROG(FileName) == true)
	{
		*Type = AUDIO_OPTIMFROG;
		return OptimFROG_OpenR(FileName);
	}
#endif
	// Add RealAudio call here once decoder is complete
#ifdef _WINDOWS
	else if (Is_WMA(FileName) == true)
	{
		*Type = AUDIO_WMA;
		return WMA_OpenR(FileName);
	}
#endif

	return NULL;
}

FileInfo *Audio_GetFileInfo(void *p_AudioPtr, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_GetFileInfo(p_AudioPtr);
	else if (Type == AUDIO_FLAC)
		return FLAC_GetFileInfo(p_AudioPtr);
	else if (Type == AUDIO_WAVE)
		return WAV_GetFileInfo(p_AudioPtr);
	else if (Type == AUDIO_M4A)
		return M4A_GetFileInfo(p_AudioPtr);
	else if (Type == AUDIO_AAC)
		return AAC_GetFileInfo(p_AudioPtr);
	else if (Type == AUDIO_MP3)
		return MP3_GetFileInfo(p_AudioPtr);
#ifndef __NO_IT__
	else if (Type == AUDIO_IT)
		return IT_GetFileInfo(p_AudioPtr);
#endif
	else if (Type == AUDIO_MOD)
		return MOD_GetFileInfo(p_AudioPtr);
#ifndef __NO_MPC__
	else if (Type == AUDIO_MUSEPACK)
		return MPC_GetFileInfo(p_AudioPtr);
#endif
	else if (Type == AUDIO_WAVPACK)
		return WavPack_GetFileInfo(p_AudioPtr);
#ifndef __NO_OptimFROG__
	else if (Type == AUDIO_OPTIMFROG)
		return OptimFROG_GetFileInfo(p_AudioPtr);
#endif
	// Add RealAudio call here once decoder is complete
#ifdef _WINDOWS
	else if (Type == AUDIO_WMA)
		return WMA_GetFileInfo(p_AudioPtr);
#endif

	return NULL;
}

long Audio_FillBuffer(void *p_AudioPtr, BYTE *OutBuffer, int nOutBufferLen, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	else if (Type == AUDIO_FLAC)
		return FLAC_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	else if (Type == AUDIO_WAVE)
		return WAV_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	else if (Type == AUDIO_M4A)
		return M4A_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	else if (Type == AUDIO_AAC)
		return AAC_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	else if (Type == AUDIO_MP3)
		return MP3_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
#ifndef __NO_IT__
	else if (Type == AUDIO_IT)
		return IT_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
#endif
	else if (Type == AUDIO_MOD)
		return MOD_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
#ifndef __NO_MPC__
	else if (Type == AUDIO_MUSEPACK)
		return MPC_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
#endif
	else if (Type == AUDIO_WAVPACK)
		return WavPack_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
#ifndef __NO_OptimFROG__
	else if (Type == AUDIO_OPTIMFROG)
		return OptimFROG_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
#endif
	// Add RealAudio call here once decoder is complete
#ifdef _WINDOWS
	else if (Type == AUDIO_WMA)
		return WMA_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
#endif

	return 0;
}

int Audio_CloseFileR(void *p_AudioPtr, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_CloseFileR(p_AudioPtr);
	else if (Type == AUDIO_FLAC)
		return FLAC_CloseFileR(p_AudioPtr);
	else if (Type == AUDIO_WAVE)
		return WAV_CloseFileR(p_AudioPtr);
	else if (Type == AUDIO_M4A)
		return M4A_CloseFileR(p_AudioPtr);
	else if (Type == AUDIO_AAC)
		return AAC_CloseFileR(p_AudioPtr);
	else if (Type == AUDIO_MP3)
		return MP3_CloseFileR(p_AudioPtr);
#ifndef __NO_IT__
	else if (Type == AUDIO_IT)
		return IT_CloseFileR(p_AudioPtr);
#endif
	else if (Type == AUDIO_MOD)
		return MOD_CloseFileR(p_AudioPtr);
#ifndef __NO_MPC__
	if (Type == AUDIO_MUSEPACK)
		return MPC_CloseFileR(p_AudioPtr);
#endif
	else if (Type == AUDIO_WAVPACK)
		return WavPack_CloseFileR(p_AudioPtr);
#ifndef __NO_OptimFROG__
	else if (Type == AUDIO_OPTIMFROG)
		return OptimFROG_CloseFileR(p_AudioPtr);
#endif

	return 0;
}

void Audio_Play(void *p_AudioPtr, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_Play(p_AudioPtr);
	else if (Type == AUDIO_FLAC)
		return FLAC_Play(p_AudioPtr);
	else if (Type == AUDIO_WAVE)
		return WAV_Play(p_AudioPtr);
	else if (Type == AUDIO_M4A)
		return M4A_Play(p_AudioPtr);
	else if (Type == AUDIO_AAC)
		return AAC_Play(p_AudioPtr);
	else if (Type == AUDIO_MP3)
		return MP3_Play(p_AudioPtr);
#ifndef __NO_IT__
	else if (Type == AUDIO_IT)
		return IT_Play(p_AudioPtr);
#endif
	else if (Type == AUDIO_MOD)
		return MOD_Play(p_AudioPtr);
#ifndef __NO_MPC__
	else if (Type == AUDIO_MUSEPACK)
		return MPC_Play(p_AudioPtr);
#endif
	else if (Type == AUDIO_WAVPACK)
		return WavPack_Play(p_AudioPtr);
#ifndef __NO_OptimFROG__
	else if (Type == AUDIO_OPTIMFROG)
		return OptimFROG_Play(p_AudioPtr);
#endif
#ifdef _WINDOWS
	// Add RealAudio here once decoder is complete
	else if (Type == AUDIO_WMA)
		return WMA_Play(p_AudioPtr);
#endif

	return;
}

bool Is_Audio(const char *FileName)
{
	if (Is_OggVorbis(FileName) == true)
		return true;
	else if (Is_FLAC(FileName) == true)
		return true;
	else if (Is_WAV(FileName) == true)
		return true;
	else if (Is_M4A(FileName) == true)
		return true;
	else if (Is_AAC(FileName) == true)
		return true;
	else if (Is_MP3(FileName) == true)
		return true;
#ifndef __NO_IT__
	else if (Is_IT(FileName) == true)
		return true;
#endif
	else if (Is_MOD(FileName) == true)
		return true;
#ifndef __NO_MPC__
	else if (Is_MPC(FileName) == true)
		return true;
#endif
	else if (Is_WavPack(FileName) == true)
		return true;
#ifndef __NO_OptimFROG__
	else if (Is_OptimFROG(FileName) == true)
		return true;
#endif
#ifdef _WINDOWS
	// Add RealAudio call here when decoder is complete
	else if (Is_WMA(FileName) == true)
		return true;
#endif

	return false;
}
