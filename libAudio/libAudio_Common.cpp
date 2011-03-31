#include <stdio.h>
#include <stdlib.h>
#ifdef _WINDOWS
#include <windows.h>
/*!
 * @internal
 * The Windows definition of a cross-platform sleep function
 */
#define _usleep _sleep
#else
#include <ctype.h>
#include <time.h>
/*!
 * @internal
 * The number of Miliseconds in one Second
 */
#define MSECS_IN_SEC 1000
/*!
 * @internal
 * The number of Nanoseconds in one Milisecond
 */
#define NSECS_IN_MSEC 1000000
/*!
 * @internal
 * The non-Windows definition of a cross-platform sleep function
 */
#define _usleep(milisec) \
	{\
		struct timespec req = {milisec / MSECS_IN_SEC, (milisec % MSECS_IN_SEC) * NSECS_IN_MSEC}; \
		nanosleep(&req, NULL); \
	}
#endif
#include "libAudio.h"
#include "libAudio_Common.h"

/*!
 * @internal
 * \c fseek_wrapper() is the internal seek callback for several file decoders.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file The \c FILE handle for the seek to use as a void pointer
 * @param offset A 64-bit integer giving the number of bytes from the \p origin to seek through
 * @param origin The location identifier to seek from
 * @return The return result of \c fseek() indiciating if the seek worked or not
 */
int fseek_wrapper(void *p_file, __int64 offset, int origin)
{
	if (p_file == NULL)
		return -1;
	return fseek((FILE *)p_file, (long)offset, origin);
}

/*!
 * @internal
 * \c ftell_wrapper() is the internal I/O possition callback for several file decoders.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file The \c FILE handle for the seek to use as a void pointer
 * @return The possition of I/O in the file called for
 * @deprecated Marked as deprecated as nothing currently uses or calls this function,
 *   so I am considering removing it.
 */
__int64 ftell_wrapper(void *p_file)
{
	return ftell((FILE *)p_file);
}

bool Playback::OpenALInit = false;
UINT Playback::sourceNum = 0;
ALCdevice *Playback::device = NULL;
ALCcontext *Playback::context = NULL;

void Playback::init()
{
	if (OpenALInit == false)
	{
		device = alcOpenDevice(NULL);
		context = alcCreateContext(device, NULL);
		alcMakeContextCurrent(context);

		alGenSources(1, &sourceNum);

		alSourcef(sourceNum, AL_GAIN, 1);
		alSourcef(sourceNum, AL_PITCH, 1);
		alSource3f(sourceNum, AL_POSITION, 0, 0, 0);
		alSource3f(sourceNum, AL_VELOCITY, 0, 0, 0);
		alSource3f(sourceNum, AL_DIRECTION, 0, 0, 0);
		alSourcef(sourceNum, AL_ROLLOFF_FACTOR, 0);
		OpenALInit = true;
		atexit(deinit);
	}
}

void Playback::createBuffers()
{
	alGenBuffers(4, buffers);

	for (UINT i = 0; i < 4; i++)
	{
		alBufferi(buffers[i], AL_SIZE, nBufferLen);
		alBufferi(buffers[i], AL_CHANNELS, p_FI->Channels);
	}
}

void Playback::deinit()
{
	alDeleteSources(1, &sourceNum);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

/*!
 * @internal
 * Called at the start of Play() to determine the format the playback buffers are in
 * as a result of the processing that occurs in the buffer filling callback
 */
int Playback::getBufferFormat()
{
	if (p_FI->Channels == 2)
		return AL_FORMAT_STEREO16;
	else if (p_FI->Channels == 1)
		return AL_FORMAT_MONO16;
	/*else if (p_FI->BitsPerSample == 8 && p_FI->Channels == 2)
		return AL_FORMAT_STEREO8;
	else if (p_FI->BitsPerSample == 8 && p_FI->Channels == 1)
		return AL_FORMAT_MONO8;*/
	else
		return AL_FORMAT_STEREO16;
}

Playback::Playback(FileInfo *p_FI, FB_Func DataCallback, BYTE *BuffPtr, int nBuffLen, void *p_AudioPtr)
{
//	float orient[6] = {0, 0, -1, 0, 1, 0};

	if (p_FI == NULL)
		return;
	if (DataCallback == NULL)
		return;
	if (BuffPtr == NULL)
		return;
	if (p_AudioPtr == NULL)
		return;

	this->p_FI = p_FI;
	FillBuffer = DataCallback;
	buffer = BuffPtr;
	nBufferLen = nBuffLen;
	this->p_AudioPtr = p_AudioPtr;

	// Initialize OpenAL ready
	init();
	createBuffers();
}

#ifdef __NICE_OUTPUT__
void DoDisplay(int *p_CN, int *p_P, char *Chars)
{
	int lenChars = strlen(Chars);
	if (((*p_P) % lenChars) == 0)
	{
		fprintf(stdout, "%c\b", Chars[(*p_CN)]);
		fflush(stdout);
		(*p_CN)++;
		if (*p_CN == lenChars)
			*p_CN = 0;
	}
	(*p_P)++;
	if (*p_P == lenChars)
		*p_P = 0;
}
#endif

void Playback::Play()
{
	long bufret = 1;
	int nBuffs = 0, Playing;
	int Fmt = getBufferFormat();
#ifdef __NICE_OUTPUT__
	static char *ProgressChars = "\xB3/\-\\";
	int CharNum = 0, Proc = 0;
#endif

#ifdef __NICE_OUTPUT__
	fprintf(stdout, "Playing: *\b");
	fflush(stdout);
#endif
	bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
	if (bufret > 0)
	{
		alBufferData(buffers[0], Fmt, buffer, nBufferLen, p_FI->BitRate);
		alSourceQueueBuffers(sourceNum, 1, &buffers[0]);
		nBuffs++;
	}
#ifdef __NICE_OUTPUT__
	DoDisplay(&CharNum, &Proc, ProgressChars);
#endif

	bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
	if (bufret > 0)
	{
		alBufferData(buffers[1], Fmt, buffer, nBufferLen, p_FI->BitRate);
		alSourceQueueBuffers(sourceNum, 1, &buffers[1]);
		nBuffs++;
	}
#ifdef __NICE_OUTPUT__
	DoDisplay(&CharNum, &Proc, ProgressChars);
#endif

	bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
	if (bufret > 0)
	{
		alBufferData(buffers[2], Fmt, buffer, nBufferLen, p_FI->BitRate);
		alSourceQueueBuffers(sourceNum, 1, &buffers[2]);
		nBuffs++;
	}
#ifdef __NICE_OUTPUT__
	DoDisplay(&CharNum, &Proc, ProgressChars);
#endif

	bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
	if (bufret > 0)
	{
		alBufferData(buffers[3], Fmt, buffer, nBufferLen, p_FI->BitRate);
		alSourceQueueBuffers(sourceNum, 1, &buffers[3]);
		nBuffs++;
	}
#ifdef __NICE_OUTPUT__
	DoDisplay(&CharNum, &Proc, ProgressChars);
#endif

	alSourcePlay(sourceNum);

	while (bufret > 0)
	{
		int Processed;
		alGetSourcei(sourceNum, AL_BUFFERS_PROCESSED, &Processed);
		alGetSourcei(sourceNum, AL_SOURCE_STATE, &Playing);

		while (Processed--)
		{
			UINT buffer;

			alSourceUnqueueBuffers(sourceNum, 1, &buffer);
			bufret = FillBuffer(p_AudioPtr, this->buffer, nBufferLen);
			if (bufret <= 0)
			{
				nBuffs -= (Processed + 1);
				goto finish;
			}
#ifdef __NICE_OUTPUT__
			DoDisplay(&CharNum, &Proc, ProgressChars);
#endif
			alBufferData(buffer, Fmt, this->buffer, nBufferLen, p_FI->BitRate);
			alSourceQueueBuffers(sourceNum, 1, &buffer);

		}

		if (Playing != AL_PLAYING)
			alSourcePlay(sourceNum);
		_usleep(40);
	}

finish:
#ifdef __NICE_OUTPUT__
	fprintf(stdout, "*\n");
	fflush(stdout);
#endif
	alGetSourcei(sourceNum, AL_SOURCE_STATE, &Playing);
	while (nBuffs > 0)
	{
		int Processed;
		alGetSourcei(sourceNum, AL_BUFFERS_PROCESSED, &Processed);

		while (Processed--)
		{
			UINT buffer;
			alSourceUnqueueBuffers(sourceNum, 1, &buffer);
			nBuffs--;
		}

		if (Playing != AL_PLAYING)
			alSourcePlay(sourceNum);
		_usleep(40);
	}
}

Playback::~Playback()
{
	alDeleteBuffers(4, buffers);
	free(p_FI);
}
