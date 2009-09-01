#include <stdio.h>
#include <malloc.h>
#include <windows.h>
#include <al.h>
#include <alc.h>

extern int strncasecmp(const char *s1, const char *s2, unsigned int n);
extern int fseek_wrapper(void *p_file, __int64 offset, int origin);
//extern __int64 ftell_wrapper(void *p_file);
extern UINT Initialize_OpenAL(ALCdevice **pp_device, ALCcontext **pp_context);
extern UINT *CreateBuffers(UINT n, UINT nSize, UINT nChannels);
extern void DestroyBuffers(UINT **buffs, UINT n);
extern void Deinitialize_OpenAL(ALCdevice **pp_Dev, ALCcontext **pp_Ctx, UINT Source);
/*extern void QueueBuffer(UINT Source, UINT *p_BuffNum, int format, BYTE *Buffer, int nBuffSize, int BitRate);
extern void UnqueueBuffer(UINT Source, UINT *BuffNum);*/
extern int GetBuffFmt(int BPS, int Channels);

#ifndef FB_Func
typedef long (__cdecl *FB_Func)(void *p_AudioPtr, BYTE *OutBuffer, int nOutBufferLen);
#endif

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
	bool Resuming;

public:
	Playback(FileInfo *p_FI, FB_Func DataCallback, BYTE *BuffPtr, int nBuffLen, void *p_AudioPtr);
	void Play();
	void Stop();
	void Pause();
	bool IsPlaying();
	bool IsPaused();
	~Playback();

protected:
	bool Playing, Paused;
};