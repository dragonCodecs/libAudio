#define MIXPLUG_MIXREADY	0x01

class IMixPlugin
{
public:
	virtual int AddRef() = 0;
	virtual int Release() = 0;
	virtual void SaveAllParameters() = 0;
	virtual void RestoreAllParameters(long Prog = -1) = 0;
	virtual void Process(float *OutL, float *OutR, unsigned long Samples) = 0;
	virtual void Init(unsigned long Freq, int Reset) = 0;
	virtual bool MidiSend(DWORD MidiCode) = 0;
	virtual void MidiCC(UINT MidiCh, UINT Controler, UINT Param, UINT trackChannel) = 0;
	virtual void MidiPitchBend(UINT MidiCh, int Param, UINT trackChannel) = 0;
	virtual void MidiCommand(UINT MidiCh, UINT MidiProg, WORD MidiBank, UINT note, UINT vol, UINT trackChan) = 0;
	virtual void HardAllNotesOff() = 0;
	virtual void RecalculateGain() = 0;
	virtual bool isPlaying(UINT note, UINT midiChn, UINT trackerChn) = 0;
	virtual bool MoveNote(UINT note, UINT midiChn, UINT sourceTrackerChn, UINT destTrackerChn) = 0;
	virtual void SetZxxParameter(UINT Param, UINT Value) = 0;
	virtual UINT GetZxxParameter(UINT Param) = 0;
	virtual long Dispatch(long opCode, long index, long value, void *ptr, float opt) = 0;
	virtual void NotifySongPlaying(bool) = 0;
	virtual bool IsSongPlaying() = 0;
	virtual bool IsResumed() = 0;
	virtual void Resume() = 0;
	virtual void Suspend() = 0;
	virtual BOOL isInstrument() = 0;
	virtual BOOL CanRecieveMidiEvents() = 0;
};

class ISoundSource
{
public:
	virtual ULONG AudioRead(void *Data, ULONG Size) = 0;
	virtual void AudioDone(ULONG BytesWriten) = 0;//, ULONG nLatency) = 0;
};

typedef struct _MixPluginState
{
	DWORD Flags;
	long VolDecayL, VolDecayR;
	int *MixBuffer;
	float *OutBufferL;
	float *OutBufferR;
} MixPluginState;

typedef struct _MixPluginInfo
{
	DWORD PluginID1;
	DWORD PluginID2;
	DWORD InputRouting;
	DWORD OutputRouting;
	DWORD Reserved[4];
	char Name[32];
	char LibraryName[64];
} MixPluginInfo;

typedef struct _MixPlugin
{
	IMixPlugin *MixPlugin;
	MixPluginState *MixState;
	ULONG PluginDataSize;
	void *PluginData;
	MixPluginInfo Info;
	float DryRatio;
	long defaultProgram;
} MixPlugin;