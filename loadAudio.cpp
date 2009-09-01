#include "libAudio.h"

void *Audio_OpenR(char *FileName, int *Type)
{
	if (Is_OggVorbis(FileName) == true)
	{
		*Type = AUDIO_OGG_VORBIS;
		return OggVorbis_OpenR(FileName);
	}
	if (Is_FLAC(FileName) == true)
	{
		*Type = AUDIO_FLAC;
		return FLAC_OpenR(FileName);
	}
	if (Is_WAV(FileName) == true)
	{
		*Type = AUDIO_WAVE;
		return WAV_OpenR(FileName);
	}
	if (Is_AAC(FileName) == true)
	{
		*Type = AUDIO_AAC;
		return AAC_OpenR(FileName);
	}
	if (Is_MP3(FileName) == true)
	{
		*Type = AUDIO_MP3;
		return MP3_OpenR(FileName);
	}
	if (Is_IT(FileName) == true)
	{
		*Type = AUDIO_IT;
		return IT_OpenR(FileName);
	}
	if (Is_MPC(FileName) == true)
	{
		*Type = AUDIO_MUSEPACK;
		return MPC_OpenR(FileName);
	}
	if (Is_WavPack(FileName) == true)
	{
		*Type = AUDIO_WAVPACK;
		return WavPack_OpenR(FileName);
	}
	if (Is_OptimFROG(FileName) == true)
	{
		*Type = AUDIO_OPTIMFROG;
		return OptimFROG_OpenR(FileName);
	}
	// Add RealAudio call here once decoder is complete
	if (Is_WMA(FileName) == true)
	{
		*Type = AUDIO_WMA;
		return WMA_OpenR(FileName);
	}

	return NULL;
}

FileInfo *Audio_GetFileInfo(void *p_AudioPtr, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_GetFileInfo(p_AudioPtr);
	if (Type == AUDIO_FLAC)
		return FLAC_GetFileInfo(p_AudioPtr);
	if (Type == AUDIO_WAVE)
		return WAV_GetFileInfo(p_AudioPtr);
	if (Type == AUDIO_AAC)
		return AAC_GetFileInfo(p_AudioPtr);
	if (Type == AUDIO_MP3)
		return MP3_GetFileInfo(p_AudioPtr);
	if (Type == AUDIO_IT)
		return IT_GetFileInfo(p_AudioPtr);
	if (Type == AUDIO_MUSEPACK)
		return MPC_GetFileInfo(p_AudioPtr);
	if (Type == AUDIO_WAVPACK)
		return WavPack_GetFileInfo(p_AudioPtr);
	if (Type == AUDIO_OPTIMFROG)
		return OptimFROG_GetFileInfo(p_AudioPtr);
	// Add RealAudio call here once decoder is complete
	if (Type == AUDIO_WMA)
		return WMA_GetFileInfo(p_AudioPtr);

	return NULL;
}

long Audio_FillBuffer(void *p_AudioPtr, BYTE *OutBuffer, int nOutBufferLen, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	if (Type == AUDIO_FLAC)
		return FLAC_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	if (Type == AUDIO_WAVE)
		return WAV_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	if (Type == AUDIO_AAC)
		return AAC_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	if (Type == AUDIO_MP3)
		return MP3_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	if (Type == AUDIO_IT)
		return IT_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	if (Type == AUDIO_MUSEPACK)
		return MPC_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	if (Type == AUDIO_WAVPACK)
		return WavPack_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	if (Type == AUDIO_OPTIMFROG)
		return OptimFROG_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
	// Add RealAudio call here once decoder is complete
	if (Type == AUDIO_WMA)
		return WMA_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);

	return 0;
}

int Audio_CloseFileR(void *p_AudioPtr, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_CloseFileR(p_AudioPtr);
	if (Type == AUDIO_FLAC)
		return FLAC_CloseFileR(p_AudioPtr);
	if (Type == AUDIO_WAVE)
		return WAV_CloseFileR(p_AudioPtr);
	if (Type == AUDIO_AAC)
		return AAC_CloseFileR(p_AudioPtr);
	if (Type == AUDIO_MP3)
		return MP3_CloseFileR(p_AudioPtr);
	if (Type == AUDIO_IT)
		return IT_CloseFileR(p_AudioPtr);
	//if (Type == AUDIO_MUSEPACK)
	//	return MPC_CloseFileR(p_AudioPtr);
	if (Type == AUDIO_WAVPACK)
		return WavPack_CloseFileR(p_AudioPtr);
	if (Type == AUDIO_OPTIMFROG)
		return OptimFROG_CloseFileR(p_AudioPtr);

	return 0;
}

void Audio_Play(void *p_AudioPtr, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_Play(p_AudioPtr);
	if (Type == AUDIO_FLAC)
		return FLAC_Play(p_AudioPtr);
	if (Type == AUDIO_WAVE)
		return WAV_Play(p_AudioPtr);
	if (Type == AUDIO_AAC)
		return AAC_Play(p_AudioPtr);
	if (Type == AUDIO_MP3)
		return MP3_Play(p_AudioPtr);
	if (Type == AUDIO_IT)
		return IT_Play(p_AudioPtr);
	if (Type == AUDIO_MUSEPACK)
		return MPC_Play(p_AudioPtr);
	if (Type == AUDIO_WAVPACK)
		return WavPack_Play(p_AudioPtr);
	if (Type == AUDIO_OPTIMFROG)
		return OptimFROG_Play(p_AudioPtr);
	// Add RealAudio here once decoder is complete
	if (Type == AUDIO_WMA)
		return WMA_Play(p_AudioPtr);

	return;
}

bool Is_Audio(char *FileName)
{
	if (Is_OggVorbis(FileName) == true)
		return true;
	if (Is_FLAC(FileName) == true)
		return true;
	if (Is_WAV(FileName) == true)
		return true;
	if (Is_AAC(FileName) == true)
		return true;
	if (Is_MP3(FileName) == true)
		return true;
	if (Is_MPC(FileName) == true)
		return true;
	if (Is_WavPack(FileName) == true)
		return true;
	if (Is_OptimFROG(FileName) == true)
		return true;
	// Add RealAudio call here when decoder is complete
	if (Is_WMA(FileName) == true)
		return true;

	return false;
}