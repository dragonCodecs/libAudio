#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
/*!
 * @internal
 * @file libAudio_Common.cpp
 * @brief libAudio's common routines, including the playback engine
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2012
 */
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <chrono>
#include <thread>
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
int fseek_wrapper(void *p_file, int64_t offset, int origin)
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
int64_t ftell_wrapper(void *p_file)
{
	return ftell((FILE *)p_file);
}

bool Playback::OpenALInit = false;
uint32_t Playback::sourceNum = 0;
ALCdevice *Playback::device = NULL;
ALCcontext *Playback::context = NULL;

/*!
 * @internal
 * Initialises the OpenAL context and devices and the source to which
 * we attach buffers
 */
void Playback::init()
{
#ifndef _WINDOWS
	if (OpenALInit == false)
#endif
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
#ifndef _WINDOWS
		OpenALInit = true;
		atexit(deinit);
#endif
	}
}

/*!
 * @internal
 * Generates the OpenAL buffers and sets up the various properties
 * for them for the instance of \c Playback this is called for
 */
void Playback::createBuffers()
{
	alGenBuffers(4, buffers);

	for (uint32_t i = 0; i < 4; i++)
	{
		alBufferi(buffers[i], AL_SIZE, nBufferLen);
		alBufferi(buffers[i], AL_CHANNELS, p_FI->Channels);
	}
}

/*!
 * @internal
 * Deinitialises the OpenAL context and devices and the source to which
 * we attach buffers (invoked at the end of the program using libAudio's lifetime)
 */
void Playback::deinit()
{
	alDeleteSources(1, &sourceNum);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

/*!
 * @internal
 * Called at the start of \c Play() to determine the format the playback buffers are in
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

/*!
 * @internal
 * The constructor for \c Playback which makes sure that OpenAL has been initialised and
 * which prepares buffers for the singular source held internally
 * @param p_FI The \c FileInfo instance for the file to be played
 * @param DataCallback The function to use to load more data into the buffer
 * @param BuffPtr The buffer to load data into (which is typically the buffer internal to the decoder)
 * @param nBuffLen The length of the buffer decoded into
 * @param p_AudioPtr The typeless pointer to the file's decoding context
 * @note \p DataCallback should be removed in future versions of this function and in place
 * \c Audio_FillBuffer() should be called as it does not really have overhead now
 */
Playback::Playback(FileInfo *p_FI, FB_Func DataCallback, uint8_t *BuffPtr, int nBuffLen, void *p_AudioPtr)
{
	if (p_FI == NULL)
		return;
	if (DataCallback == NULL)
		return;
	if (BuffPtr == NULL)
		return;
	if (p_AudioPtr == NULL)
		return;

	std::chrono::seconds bufferSize(nBuffLen);

	this->p_FI = p_FI;
	FillBuffer = DataCallback;
	buffer = BuffPtr;
	nBufferLen = nBuffLen;
	this->p_AudioPtr = p_AudioPtr;

	bufferSize /= p_FI->Channels * (p_FI->BitsPerSample / 8);
	sleepTime = std::chrono::duration_cast<std::chrono::nanoseconds>(bufferSize).count() / p_FI->BitRate;

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
		alBufferData(buffers[0], Fmt, buffer, bufret, p_FI->BitRate);
		alSourceQueueBuffers(sourceNum, 1, &buffers[0]);
		nBuffs++;
	}
#ifdef __NICE_OUTPUT__
	DoDisplay(&CharNum, &Proc, ProgressChars);
#endif

	bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
	if (bufret > 0)
	{
		alBufferData(buffers[1], Fmt, buffer, bufret, p_FI->BitRate);
		alSourceQueueBuffers(sourceNum, 1, &buffers[1]);
		nBuffs++;
	}
#ifdef __NICE_OUTPUT__
	DoDisplay(&CharNum, &Proc, ProgressChars);
#endif

	bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
	if (bufret > 0)
	{
		alBufferData(buffers[2], Fmt, buffer, bufret, p_FI->BitRate);
		alSourceQueueBuffers(sourceNum, 1, &buffers[2]);
		nBuffs++;
	}
#ifdef __NICE_OUTPUT__
	DoDisplay(&CharNum, &Proc, ProgressChars);
#endif

	bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
	if (bufret > 0)
	{
		alBufferData(buffers[3], Fmt, buffer, bufret, p_FI->BitRate);
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
			uint32_t buffer;

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
			alBufferData(buffer, Fmt, this->buffer, bufret, p_FI->BitRate);
			alSourceQueueBuffers(sourceNum, 1, &buffer);

		}

		if (Playing != AL_PLAYING)
			alSourcePlay(sourceNum);
		std::this_thread::sleep_for(std::chrono::nanoseconds(sleepTime));
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
			uint32_t buffer;
			alSourceUnqueueBuffers(sourceNum, 1, &buffer);
			nBuffs--;
		}

		if (Playing != AL_PLAYING)
			alSourcePlay(sourceNum);
		std::this_thread::sleep_for(std::chrono::nanoseconds(sleepTime));
	}
}

void Playback::Pause()
{
}

void Playback::Stop()
{
}

/*!
 * @internal
 * The deconstructor for \c Playback which makes sure that the OpenAL buffers
 * are all freed and which frees the \c FileInfo instance created for a file
 * @note The \c FileInfo handling code is technically in the wrong place and
 * should be in the care of the format decoder instead, which must be fixed
 * in future versions of the library so that memory leaks don't happen
 * when people do not free the \c FileInfo instance themselves when using
 * the library with external playback
 */
Playback::~Playback()
{
	alDeleteBuffers(4, buffers);
	free(p_FI);
#ifdef _WINDOWS
	deinit();
#endif
}
