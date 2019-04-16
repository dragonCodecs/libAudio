#ifndef __libAudio_Common_H__
#define __libAudio_Common_H__

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <al.h>
#include <alc.h>
/*!
 * @internal
 * Definition of \c __cdecl which is compatible with the keyword
 * not existing on non-Windows platforms due to them not having
 * multiple calling conventions which is a dumb microsoft-ism
 */
#define __CDECL__ __cdecl
/*!
 * @internal
 * Definition of \c __fastcall which is compatible with the keyword
 * not existing on non-Windows platforms due to them not having
 * multiple calling conventions which is a dumb microsoft-ism
 */
#define __FASTCALL__ __fastcall
#define strcasecmp stricmp
#define strncasecmp strnicmp
#define snprintf _snprintf
#else
#include <AL/al.h>
#include <AL/alc.h>
/*!
 * @internal
 * Null definition which is compatible with the \c __cdecl keyword
 * not existing on non-Windows platforms
 */
#define __CDECL__
/*!
 * @internal
 * Definition which is compatible with the \c __cdecl keyword
 * not existing on non-Windows platforms. This ensures that
 * calls that are meant to be "fastcall" on Windows are instead
 * hopefully inlined on Linux, making them even faster still by
 * eliminating the call completely
 */
#define __FASTCALL__ inline
#endif

#include "fileInfo.hxx"

extern int fseek_wrapper(void *p_file, int64_t offset, int origin);
//extern int64_t ftell_wrapper(void *p_file);

/*!
 * @internal
 * This structure holds what all low-level APIs must expose
 * and is used to optimise performance of the high-level API
 * by removing the need to if-check what format we're using
 * on each call - the pointers in this structure say it all
 */
typedef struct _API_Functions
{
	void *(__CDECL__ *OpenR)(const char *FileName);
	FileInfo *(__CDECL__ *GetFileInfo)(void *p_File);
	long (__CDECL__ *FillBuffer)(void *p_File, uint8_t *OutBuffer, int nOutBufferLen);
	int (__CDECL__ *CloseFileR)(void *p_File);
	void (__CDECL__ *Play)(void *p_File);
	void (__CDECL__ *Pause)(void *p_File);
	void (__CDECL__ *Stop)(void *p_File);
} API_Functions;

FileInfo *audioFileInfo(void *audioFile);
void audioFileInfo(void *audioFile, const FileInfo *const fileInfo);
long audioFillBuffer(void *audioFile, uint8_t *buffer, int length);
int audioCloseFileR(void *audioFile);
void audioPlay(void *audioFile);
void audioPause(void *audioFile);
void audioStop(void *audioFile);

/*!
 * @internal
 * This structure is what is returned in place of the internal
 * \c p_AudioFile member when using the high-level API to utilise
 * audio files in a reading capacity. It allows for a far more
 * flexible, and faster library by removing a lot of if-checks
 * from the high-level API.
 */
typedef struct _AudioPointer
{
	/*!
	 * @internal
	 * The real file context returned from a low-level \c *_OpenR()
	 * call
	 */
	void *p_AudioFile;
	/*!
	 * @internal
	 * a pointer to the API_Functions structure for the low-level
	 * decoder
	 */
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
extern API_Functions S3MDecoder;
extern API_Functions STMDecoder;
extern API_Functions AONDecoder;
extern API_Functions FC1xDecoder;
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
