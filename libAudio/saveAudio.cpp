#include "libAudio.h"
#include "libAudio_Common.h"

libAUDIO_API void *Audio_OpenW(const char *FileName, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_OpenW(FileName);
	else if (Type == AUDIO_FLAC)
		return FLAC_OpenW(FileName);
	else if (Type == AUDIO_MP4)
		return M4A_OpenW(FileName);
	else
		return NULL;
}

libAUDIO_API void Audio_SetFileInfo(void *p_AudioPtr, FileInfo *p_FI, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		OggVorbis_SetFileInfo(p_AudioPtr, p_FI);
	else if (Type == AUDIO_FLAC)
		FLAC_SetFileInfo(p_AudioPtr, p_FI);
	else if (Type == AUDIO_MP4)
		M4A_SetFileInfo(p_AudioPtr, p_FI);
}

libAUDIO_API long Audio_WriteBuffer(void *p_AudioPtr, BYTE *InBuffer, int nInBufferLen, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_WriteBuffer(p_AudioPtr, InBuffer, nInBufferLen);
	else if (Type == AUDIO_FLAC)
		return FLAC_WriteBuffer(p_AudioPtr, InBuffer, nInBufferLen);
	else if (Type == AUDIO_MP4)
		return M4A_WriteBuffer(p_AudioPtr, InBuffer, nInBufferLen);
	else
		return -2;
}

libAUDIO_API int Audio_CloseFileW(void *p_AudioPtr, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_CloseFileW(p_AudioPtr);
	else if (Type == AUDIO_FLAC)
		return FLAC_CloseFileW(p_AudioPtr);
	else if (Type == AUDIO_MP4)
		return M4A_CloseFileW(p_AudioPtr);
	else
		return 0;
}
