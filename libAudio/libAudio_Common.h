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

using fileIs_t = bool (*)(const char *);
using fileOpenR_t = void *(*)(const char *);
using fileOpenW_t = void *(*)(const char *);
using fileFillBuffer_t = int64_t (*)(void *audioFile, void *const buffer, const uint32_t length);

const fileInfo_t *audioFileInfo(void *audioFile);
bool audioFileInfo(void *audioFile, const fileInfo_t *const fileInfo);

#endif /* __libAudio_Common_H__ */
