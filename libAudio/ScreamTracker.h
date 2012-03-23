#ifndef __ScreamTracker_H__
#define __ScreamTracker_H__ 1

typedef struct _S3M_Intern
{
	FILE *f_S3M;
	FileInfo *p_FI;
	Playback *p_Playback;
	uint8_t buffer[8192];
	ModuleFile *p_File;
} S3M_Intern;

typedef struct _STM_Intern
{
	FILE *f_STM;
	FileInfo *p_FI;
	Playback *p_Playback;
	uint8_t buffer[8192];
	ModuleFile *p_File;
} STM_Intern;

#endif /*__ScreamTracker_H__*/
