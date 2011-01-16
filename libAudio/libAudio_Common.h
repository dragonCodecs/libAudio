#ifndef __libAudio_Common_H__
#define __libAudio_Common_H__

#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <windows.h>
#include <al.h>
#include <alc.h>
#define __CDECL__ __cdecl
#define __FASTCALL__ __fastcall
#else
#include <AL/al.h>
#include <AL/alc.h>
#define __CDECL__
#define __FASTCALL__ inline
#endif

//extern int strncasecmp(const char *s1, const char *s2, unsigned int n);
extern int fseek_wrapper(void *p_file, __int64 offset, int origin);
//extern __int64 ftell_wrapper(void *p_file);
extern UINT Initialize_OpenAL(ALCdevice **pp_device, ALCcontext **pp_context);
extern UINT *CreateBuffers(UINT n, UINT nSize, UINT nChannels);
extern void DestroyBuffers(UINT **buffs, UINT n);
extern void Deinitialize_OpenAL(ALCdevice **pp_Dev, ALCcontext **pp_Ctx, UINT Source);
/*extern void QueueBuffer(UINT Source, UINT *p_BuffNum, int format, BYTE *Buffer, int nBuffSize, int BitRate);
extern void UnqueueBuffer(UINT Source, UINT *BuffNum);*/
extern int GetBuffFmt(int BPS, int Channels);

typedef long (__CDECL__ *FB_Func)(void *p_AudioPtr, BYTE *OutBuffer, int nOutBufferLen);

class Playback
{
public:
	FB_Func FillBuffer;
	FileInfo *p_FI;
	void *p_AudioPtr;

private:
	UINT sourceNum;
	UINT *buffers;
	ALCdevice *device;
	ALCcontext *context;
	BYTE *buffer;
	int nBufferLen;

public:
	Playback(FileInfo *p_FI, FB_Func DataCallback, BYTE *BuffPtr, int nBuffLen, void *p_AudioPtr);
	void Play();
	~Playback();
};

typedef struct _API_Functions
{
	void *(__CDECL__ *OpenR)(const char *FileName);
	FileInfo *(__CDECL__ *GetFileInfo)(void *p_File);
	long (__CDECL__ *FillBuffer)(void *p_File, BYTE *OutBuffer, int nOutBufferLen);
	int (__CDECL__ *CloseFileR)(void *p_File);
	void (__CDECL__ *Play)(void *p_File);
} API_Functions;

typedef struct _AudioPointer
{
	void *p_AudioFile;
	API_Functions *API;
} AudioPointer;

extern API_Functions OggVorbisDecoder;
extern API_Functions FLACDecoder;
extern API_Functions WAVDecoder;
extern API_Functions M4ADecoder;
extern API_Functions AACDecoder;
extern API_Functions MP3Decoder;
#ifndef __NO_IT__
extern API_Functions ITDecoder;
#endif
extern API_Functions MODDecoder;
#ifndef __NO_MPC__
extern API_Functions MPCDecoder;
#endif
extern API_Functions WavPackDecoder;
#ifndef __NO_OptimFROG__
extern API_Functions OptimFROGDecoder;
#endif
#ifdef _WINDOWS
extern API_Functions WMADecoder;
#endif

#endif /* __libAudio_Common_H__ */
