#define _USE_MATH_DEFINES
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "../ProTracker.h"
#include "moduleMixer.h"

typedef struct _Channel
{
} Channel;

typedef struct _MixerState
{
	UINT MixRate;
	UINT Channels, Samples;
	UINT TickCount;
	UINT Row, NextRow;
	UINT MusicSpeed, MusicTempo;
	UINT Pattern, CurrentPattern, NextPattern, RestartPos;
	BOOL PatternTransitionOccured;
	BYTE *Orders, MaxOrder;
	MODSample *Samp;
	UINT RowsPerBeat, SamplesPerTick;
	Channel *Chns;
	UINT MixChannels, *ChnMix;
} MixerState;

void CreateMixer(MOD_Intern *p_MF)
{
	MixerState *p_Mixer = NULL;
	p_Mixer = (MixerState *)malloc(sizeof(MixerState));
	if (p_Mixer == NULL)
		return;
	memset(p_Mixer, 0x00, sizeof(MixerState));
	p_MF->p_Mixer = p_Mixer;

	p_Mixer->Channels = p_MF->nChannels;
	p_Mixer->Samples = p_MF->nSamples;
	p_Mixer->MaxOrder = p_MF->p_Header->nOrders;
	p_Mixer->Orders = p_MF->p_Header->Orders;
	p_Mixer->MusicSpeed = 6;
	p_Mixer->MusicTempo = 125;
	p_Mixer->TickCount = p_Mixer->MusicSpeed;
	p_Mixer->Samp = p_MF->p_Samples;
	p_Mixer->MixRate = p_MF->p_FI->BitRate;
	p_Mixer->SamplesPerTick = (p_Mixer->MixRate * 640) / (p_Mixer->MusicTempo << 8);
	p_Mixer->RestartPos = p_MF->p_Header->RestartPos;
	if (p_Mixer->RestartPos > p_Mixer->MaxOrder)
		p_Mixer->RestartPos = 0;
	p_Mixer->Chns = (Channel *)malloc(sizeof(Channel) * p_Mixer->Channels);
	p_Mixer->ChnMix = (UINT *)malloc(sizeof(UINT) * p_Mixer->Channels);
}

void DestroyMixer(void *Mixer)
{
	MixerState *p_Mixer = (MixerState *)Mixer;
	if (p_Mixer == NULL)
		return;
	free(p_Mixer->Chns);
	free(p_Mixer->ChnMix);
	free(p_Mixer);
}
