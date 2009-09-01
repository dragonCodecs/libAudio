#include <vector>

typedef struct FileInfo
{
	double TotalTime;
	long SampleRate;
	long BitRate;
	int Channels;
	int BitStream;
	char *Title;
	char *Artist;
	char *Album;
	std::vector<char *> OtherComments;
	int nOtherComments;
} FileInfo;

enum SampSize
{
	EightBit = 1,
	SixteenBit = 2
};

enum Sign
{
	Unsigned = 0,
	Signed = 1
};

#ifdef _OGG_H
#define OGGVORBIS_API __declspec(dllexport)
#else
#define OGGVORBIS_API extern
#endif

OGGVORBIS_API void *OggVorbis_Open(char *FileName);
OGGVORBIS_API FileInfo *OggVorbis_GetFileInfo(void *p_VorbisFile);
OGGVORBIS_API long OggVorbis_FillBuffer(void *p_VorbisFile, char *OutBuffer, int nOutBufferLen, SampSize SampleSize, Sign Signed);
OGGVORBIS_API int OggVorbis_CloseFile(void *p_VorbisFile);