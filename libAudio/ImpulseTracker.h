#ifndef __ImpulseTracker_H__
#define __ImpulseTracker_H__

typedef struct _IT_Intern
{
	FILE *f_IT;
	FileInfo *p_FI;
	Playback *p_Playback;
	uint8_t buffer[8192];
	ModuleFile *p_File;
} IT_Intern;

#endif /*__ImpulseTracker_H__*/
