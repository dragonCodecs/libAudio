#include "libAudio.h"
#include "libAudio_Common.h"

BYTE ExternalPlayback = 0;

void *Audio_OpenR(const char *FileName)
{
	AudioPointer *ret = (AudioPointer *)malloc(sizeof(AudioPointer));
	if (ret == NULL)
		return NULL;
	if (Is_OggVorbis(FileName) == true)
	{
		ret->API = &OggVorbisDecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
	else if (Is_FLAC(FileName) == true)
	{
		ret->API = &FLACDecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
	else if (Is_WAV(FileName) == true)
	{
		ret->API = &WAVDecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
	else if (Is_M4A(FileName) == true)
	{
		ret->API = &M4ADecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
	else if (Is_AAC(FileName) == true)
	{
		ret->API = &AACDecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
	else if (Is_MP3(FileName) == true)
	{
		ret->API = &MP3Decoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
#ifndef __NO_IT__
	else if (Is_IT(FileName) == true)
	{
		ret->API = &ITDecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
#endif
	else if (Is_MOD(FileName) == true)
	{
		ret->API = &MODDecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
	else if (Is_MPC(FileName) == true)
	{
		ret->API = &MPCDecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
	else if (Is_WavPack(FileName) == true)
	{
		ret->API = &WavPackDecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
#ifndef __NO_OptimFROG__
	else if (Is_OptimFROG(FileName) == true)
	{
		ret->API = &OptimFROGDecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
#endif
	// Add RealAudio call here once decoder is complete
#ifdef __WMA__
	else if (Is_WMA(FileName) == true)
	{
		ret->API = &WMADecoder;
		ret->p_AudioFile = ret->API->OpenR(FileName);
		return ret;
	}
#endif
	free(ret);

	return NULL;
}

FileInfo *Audio_GetFileInfo(void *p_AudioPtr)
{
	AudioPointer *p_AP = (AudioPointer *)p_AudioPtr;
	if (p_AP == NULL || p_AP->p_AudioFile == NULL)
		return NULL;
	return p_AP->API->GetFileInfo(p_AP->p_AudioFile);
}

long Audio_FillBuffer(void *p_AudioPtr, BYTE *OutBuffer, int nOutBufferLen)
{
	AudioPointer *p_AP = (AudioPointer *)p_AudioPtr;
	if (p_AP == NULL || OutBuffer == NULL || p_AP->p_AudioFile == NULL)
		return -3;
	return p_AP->API->FillBuffer(p_AP->p_AudioFile, OutBuffer, nOutBufferLen);
}

int Audio_CloseFileR(void *p_AudioPtr)
{
	AudioPointer *p_AP = (AudioPointer *)p_AudioPtr;
	if (p_AP == NULL || p_AP->p_AudioFile == NULL)
		return 0;
	return p_AP->API->CloseFileR(p_AP->p_AudioFile);
}

void Audio_Play(void *p_AudioPtr)
{
	AudioPointer *p_AP = (AudioPointer *)p_AudioPtr;
	if (p_AP != NULL && p_AP->p_AudioFile != NULL)
		p_AP->API->Play(p_AP->p_AudioFile);
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
