#ifndef __libAudio_Common_H__
#define __libAudio_Common_H__

#include <stdio.h>
#include <string.h>
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
typedef struct _API_Functions
{
	void *(__CDECL__ *OpenR)(const char *FileName);
	void *(__CDECL__ *OpenW)(const char *FileName);
	const fileInfo_t *(__CDECL__ *GetFileInfo)(void *p_File);
	bool (__CDECL__ *SetFileInfo)(void *p_File, const fileInfo_t *const p_FI);
	int64_t (__CDECL__ *FillBuffer)(void *p_File, void *const buffer, const uint32_t length);
	long (__CDECL__ *WriteBuffer)(void *p_File, uint8_t *InBuffer, int nInBufferLen);
	int (__CDECL__ *CloseFileR)(void *p_File);
	int (__CDECL__ *CloseFileW)(void *p_File);
	void (__CDECL__ *Play)(void *p_File);
	void (__CDECL__ *Pause)(void *p_File);
	void (__CDECL__ *Stop)(void *p_File);
} API_Functions;

using fileIs_t = bool (*)(const char *);
using fileOpenR_t = void *(*)(const char *);
using fileFillBuffer_t = int64_t (*)(void *audioFile, void *const buffer, const uint32_t length);

const fileInfo_t *audioFileInfo(void *audioFile);
bool audioFileInfo(void *audioFile, const fileInfo_t *const fileInfo);
long audioWriteBuffer(void *audioFile, uint8_t *buffer, int length);
int audioCloseFile(void *audioFile);

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
extern API_Functions ITDecoder;
extern API_Functions MODDecoder;
extern API_Functions S3MDecoder;
extern API_Functions STMDecoder;
extern API_Functions AONDecoder;
#ifdef ENABLE_FC1x
extern API_Functions FC1xDecoder;
#endif
extern API_Functions MPCDecoder;
extern API_Functions WavPackDecoder;
#ifdef ENABLE_OptimFROG
extern API_Functions OptimFROGDecoder;
#endif
#ifdef ENABLE_WMA
extern API_Functions WMADecoder;
#endif

#endif /* __libAudio_Common_H__ */
