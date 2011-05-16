#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <windows.h>
#include <al.h>
#include <alc.h>
#define __CDECL__ __cdecl
#define __FASTCALL__ __fastcall
#define strcasecmp stricmp
#define strncasecmp strnicmp
#define snprintf _snprintf
#else
#include <AL/al.h>
#include <AL/alc.h>
#define __CDECL__
#define __FASTCALL__ inline
#endif

extern int fseek_wrapper(void *p_file, __int64 offset, int origin);
extern int GetBuffFmt(int BPS, int Channels);

#ifndef FB_Func
typedef long (__CDECL__ *FB_Func)(void *p_AudioPtr, BYTE *OutBuffer, int nOutBufferLen);
#endif

class Playback
{
public:
	FB_Func FillBuffer;
	FileInfo *p_FI;
	void *p_AudioPtr;

private:
	static UINT sourceNum;
	UINT buffers[4];
	static ALCdevice *device;
	static ALCcontext *context;
	BYTE *buffer;
	int nBufferLen;
	bool Resuming;
	static bool OpenALInit;

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

private:
	void init();
	void createBuffers();
	static void deinit();
	int getBufferFormat();
};
