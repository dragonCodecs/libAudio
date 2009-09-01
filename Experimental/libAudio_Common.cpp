#include <stdio.h>
#include <windows.h>
#include <al.h>
#include <alc.h>
#include "libAudio.h"
#include "libAudio_Common.h"

int strncasecmp(const char *s1, const char *s2, unsigned int n)
{
	if (n == 0)
	return 0;

	while ((n-- != 0) && (tolower(*(unsigned char *) s1) == tolower(*(unsigned char *) s2)))
	{
		if (n == 0 || *s1 == '\0' || *s2 == '\0')
			return 0;
		s1++;
		s2++;
	}

	return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

int fseek_wrapper(void *p_file, __int64 offset, int origin)
{
	if (p_file == NULL)
		return -1;
	return fseek((FILE *)p_file, (long)offset, origin);
}

__int64 ftell_wrapper(void *p_file)
{
	return ftell((FILE *)p_file);
}

UINT Initialize_OpenAL(ALCdevice **pp_Dev, ALCcontext **pp_Ctx)
{
	float orient[6] = {0, 0, -1, 0, 1, 0};
	UINT ret;

	*pp_Dev = alcOpenDevice(NULL);
	*pp_Ctx = alcCreateContext(*pp_Dev, NULL);
	alcMakeContextCurrent(*pp_Ctx);

	alGenSources(1, &ret);

	alSourcef(ret, AL_GAIN, 1);
	alSourcef(ret, AL_PITCH, 1);
	alSource3f(ret, AL_POSITION, 0, 0, 0);
	alSource3f(ret, AL_VELOCITY, 0, 0, 0);
	alSource3f(ret, AL_DIRECTION, 0, 0, 0);
	alSourcef(ret, AL_ROLLOFF_FACTOR, 0);

	return ret;
}

UINT *CreateBuffers(UINT n, UINT nSize, UINT nChannels)
{
	UINT bufNum = 0;
	UINT *ret = (UINT *)malloc(sizeof(UINT) * n);
	alGenBuffers(n, ret);

	for (bufNum = 0; bufNum < n; bufNum++)
	{
		alBufferi(ret[bufNum], AL_SIZE, nSize);
		alBufferi(ret[bufNum], AL_CHANNELS, nChannels);
	}

	return ret;
}

void DestroyBuffers(UINT **buffs, UINT n)
{
	alDeleteBuffers(n, *buffs);
	*buffs = NULL;
}

void Deinitialize_OpenAL(ALCdevice **pp_Dev, ALCcontext **pp_Ctx, UINT Source)
{
	alDeleteSources(1, &Source);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(*pp_Ctx);
	alcCloseDevice(*pp_Dev);
	*pp_Ctx = NULL;
	*pp_Dev = NULL;
}

void QueueBuffer(UINT Source, UINT *p_BuffNum, int format, BYTE *Buffer, int nBuffSize, int BitRate)
{
	alBufferData(*p_BuffNum, format, Buffer, nBuffSize, BitRate);
	alSourceQueueBuffers(Source, 1, p_BuffNum);
}

void UnqueueBuffer(UINT Source, UINT *p_BuffNum)
{
	alSourceUnqueueBuffers(Source, 1, p_BuffNum);
}

int GetBuffFmt(int BPS, int Channels)
{
	if (Channels == 2)
		return AL_FORMAT_STEREO16;
	else if (Channels == 1)
		return AL_FORMAT_MONO16;
	/*else if (BPS == 8 && Channels == 2)
		return AL_FORMAT_STEREO8;
	else if (BPS == 8 && Channels == 1)
		return AL_FORMAT_MONO8;*/
	else
		return AL_FORMAT_STEREO16;
}

Playback::Playback(FileInfo *p_FI, FB_Func DataCallback, BYTE *BuffPtr, int nBuffLen, void *p_AudioPtr)
{
	float orient[6] = {0, 0, -1, 0, 1, 0};

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

	buffers = (UINT *)malloc(sizeof(UINT) * 4);
	alGenBuffers(4, buffers);

	for (UINT i = 0; i < 4; i++)
	{
		alBufferi(buffers[i], AL_SIZE, nBufferLen);
		alBufferi(buffers[i], AL_CHANNELS, p_FI->Channels);
	}
	Resuming = false;
}

void Playback::Play()
{
	long bufret = 1;
	int nBuffs = 0, Playing;
	int Fmt = GetBuffFmt(p_FI->BitsPerSample, p_FI->Channels);
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
	Resuming = false;

	while (bufret > 0)
	{
		int Processed;
		alGetSourcei(sourceNum, AL_BUFFERS_PROCESSED, &Processed);
		alGetSourcei(sourceNum, AL_SOURCE_STATE, &Playing);

		while (Processed--)
		{
			UINT buffer;

			if (this->Playing == true && this->Paused == false)
			{
				alSourceUnqueueBuffers(sourceNum, 1, &buffer);
				bufret = FillBuffer(p_AudioPtr, this->buffer, nBufferLen);
				if (bufret <= 0)
				{
					nBuffs -= (Processed + 1);
					goto finish;
				}
				alBufferData(buffer, Fmt, this->buffer, nBufferLen, p_FI->BitRate);
				alSourceQueueBuffers(sourceNum, 1, &buffer);
			}
		}

		if (this->Playing == true && Playing != AL_PLAYING && this->Paused == false)
			alSourcePlay(sourceNum);
		Sleep(40);
		if (this->Paused == true)
			break;
	}

	//nBuffs = 4;

	if (this->Playing == true && this->Paused == false)
	{
finish:
		alGetSourcei(sourceNum, AL_SOURCE_STATE, &Playing);
		if (this->Playing == true && Playing != AL_PLAYING)
			alSourcePlay(sourceNum);
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

			Sleep(40);
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
	Playing = Paused = false;
}

void Playback::Pause()
{
	alSourcePause(sourceNum);
	Playing = false;
	Paused = true;
	Resuming = true;
}

Playback::~Playback()
{
	alDeleteBuffers(4, buffers);

	alDeleteSources(1, &sourceNum);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);

	free(p_FI);
}