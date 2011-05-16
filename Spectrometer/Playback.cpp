#include <stdio.h>
#include <stdlib.h>
#ifdef _WINDOWS
#include <windows.h>
#define _usleep _sleep
#else
#include <ctype.h>
#include <time.h>
#define _usleep(milisec) \
	{\
		struct timespec req = {0, milisec}; \
		nanosleep(&req, NULL); \
	}
#endif
#include <libAudio.h>
#include <pthread.h>
#include "Playback.h"

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
	Resuming = false;
}

void Playback::Play()
{
	long bufret = 1;
	int nBuffs = 0, Playing;
	int Fmt = getBufferFormat();
	this->Playing = true;
	this->Paused = false;

	if (Resuming == false)
	{
		bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
		if (bufret > 0)
		{
			alBufferData(buffers[0], Fmt, buffer, nBufferLen, p_FI->BitRate);
			alSourceQueueBuffers(sourceNum, 1, &buffers[0]);
			nBuffs++;
		}

		bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
		if (bufret > 0)
		{
			alBufferData(buffers[1], Fmt, buffer, nBufferLen, p_FI->BitRate);
			alSourceQueueBuffers(sourceNum, 1, &buffers[1]);
			nBuffs++;
		}

		bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
		if (bufret > 0)
		{
			alBufferData(buffers[2], Fmt, buffer, nBufferLen, p_FI->BitRate);
			alSourceQueueBuffers(sourceNum, 1, &buffers[2]);
			nBuffs++;
		}

		bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
		if (bufret > 0)
		{
			alBufferData(buffers[3], Fmt, buffer, nBufferLen, p_FI->BitRate);
			alSourceQueueBuffers(sourceNum, 1, &buffers[3]);
			nBuffs++;
		}

		if (this->Playing == true)
			alSourcePlay(sourceNum);
	}
	Resuming = true;

	while (bufret > 0)
	{
		int Processed;
		alGetSourcei(sourceNum, AL_BUFFERS_PROCESSED, &Processed);
		alGetSourcei(sourceNum, AL_SOURCE_STATE, &Playing);

		while (Processed--)
		{
			UINT Buff;

			if (this->Playing == true && this->Paused == false)
			{
				alSourceUnqueueBuffers(sourceNum, 1, &Buff);
				bufret = FillBuffer(p_AudioPtr, buffer, nBufferLen);
				if (bufret <= 0)
				{
					nBuffs -= (Processed + 1);
					goto finish;
				}
				alBufferData(Buff, Fmt, buffer, nBufferLen, p_FI->BitRate);
				alSourceQueueBuffers(sourceNum, 1, &Buff);
			}
		}

		if (this->Playing == true && Playing != AL_PLAYING && this->Paused == false)
			alSourcePlay(sourceNum);
		_usleep(40);
		if (this->Paused == true)
			break;
	}

finish:
	if (this->Playing == true && this->Paused == false)
	{
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
			if (this->Playing == true && Playing != AL_PLAYING)
				alSourcePlay(sourceNum);

			_usleep(40);
		}
	}
}

bool Playback::IsPlaying()
{
	return Playing;
}

bool Playback::IsPaused()
{
	return Paused;
}

void Playback::Stop()
{
	alSourceStop(sourceNum);
	Playing = Paused = Resuming = false;
}

void Playback::Pause()
{
	alSourcePause(sourceNum);
	Playing = false;
	Paused = true;
}

Playback::~Playback()
{
	UINT Buff[4];
	alSourceUnqueueBuffers(sourceNum, 4, Buff);
	alDeleteBuffers(4, buffers);
	free(p_FI);
}
