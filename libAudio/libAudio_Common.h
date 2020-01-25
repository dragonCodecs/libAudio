#ifndef __libAudio_Common_H__
#define __libAudio_Common_H__

#include <cstdio>
#include <cstring>
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/*!
 * @internal
 * Definition of \c __cdecl which is compatible with the keyword
 * not existing on non-Windows platforms due to them not having
 * multiple calling conventions which is a dumb microsoft-ism
 */
#define __CDECL__ __cdecl
#define strcasecmp stricmp
#define strncasecmp strnicmp
#define snprintf _snprintf
#else
/*!
 * @internal
 * Null definition which is compatible with the \c __cdecl keyword
 * not existing on non-Windows platforms
 */
#define __CDECL__
#endif

/*!
 * @internal
 * This structure holds what all low-level APIs must expose
 * and is used to optimise performance of the high-level API
 * by removing the need to if-check what format we're using
 * on each call - the pointers in this structure say it all
 */
struct API_Functions
{
	void *(__CDECL__ *OpenR)(const char *fileName);
	void *(__CDECL__ *OpenW)(const char *fileName);
	const fileInfo_t *(__CDECL__ *GetFileInfo)(void *p_File);
	bool (__CDECL__ *SetFileInfo)(void *p_File, const fileInfo_t *const p_FI);
	int64_t (__CDECL__ *FillBuffer)(void *p_File, void *const buffer, const uint32_t length);
	long (__CDECL__ *WriteBuffer)(void *p_File, void *const buffer, const uint32_t length);
	int (__CDECL__ *CloseFileR)(void *p_File);
	int (__CDECL__ *CloseFileW)(void *p_File);
	void (__CDECL__ *Play)(void *p_File);
	void (__CDECL__ *Pause)(void *p_File);
	void (__CDECL__ *Stop)(void *p_File);
};

using fileIs_t = bool (*)(const char *);
using fileOpenR_t = void *(*)(const char *);
using fileOpenW_t = void *(*)(const char *);
using fileFillBuffer_t = int64_t (*)(void *audioFile, void *const buffer, const uint32_t length);

const fileInfo_t *audioFileInfo(void *audioFile);
bool audioFileInfo(void *audioFile, const fileInfo_t *const fileInfo);

#endif /* __libAudio_Common_H__ */
