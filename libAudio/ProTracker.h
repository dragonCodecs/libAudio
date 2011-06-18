#ifndef __ProTracker_H__
#define __ProTracker_H__ 1

typedef struct _MOD_Intern
{
	FILE *f_MOD;
	FileInfo *p_FI;
	Playback *p_Playback;
	uint8_t buffer[8192];
	ModuleFile *p_File;
} MOD_Intern;

#endif /*__ProTracker_H__*/
