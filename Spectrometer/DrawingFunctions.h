#ifndef __DRAWINGFUNCTIONS_H__
#define __DRAWINGFUNCTIONS_H__ 1

typedef void (__cdecl *DrawFn)(short *, int);

#define nFunctions 4;

#ifdef __DRAWINGFUNCTIONS_CPP__
#include <libAudio.h>
FileInfo *p_FI;

void SideBySideHor_Osc(short *Data, int lenData);
void SideBySideVer_Osc(short *Data, int lenData);
void SideBySideVer_Spe(short *Data, int lenData);
void SideBySideVer_logSpe(short *Data, int lenData);

extern const DrawFn Functions[] =
{
	SideBySideHor_Osc,
	SideBySideVer_Osc,
	SideBySideVer_Spe,
	SideBySideVer_logSpe,
	NULL,
};
#else
extern FileInfo *p_FI;
extern const DrawFn Functions[];
#endif

#endif /*__DRAWINGFUNCTIONS_H__*/