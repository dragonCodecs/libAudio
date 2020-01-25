#ifndef __libAudio_Common_H__
#define __libAudio_Common_H__

#include <cstdio>
#include <cstring>
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define strcasecmp stricmp
#define strncasecmp strnicmp
#define snprintf _snprintf
#endif

using fileIs_t = bool (*)(const char *);
using fileOpenR_t = void *(*)(const char *);
using fileOpenW_t = void *(*)(const char *);
using fileFillBuffer_t = int64_t (*)(void *audioFile, void *const buffer, const uint32_t length);

const fileInfo_t *audioFileInfo(void *audioFile);
bool audioFileInfo(void *audioFile, const fileInfo_t *const fileInfo);

#endif /* __libAudio_Common_H__ */
