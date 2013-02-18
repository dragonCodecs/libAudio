#ifndef __FutureComposer_H__
#define __FutureComposer_H__

typedef struct _FC1x_Intern
{
	FILE *f_FC1x;
	FileInfo *p_FI;
	Playback *p_Playback;
	uint8_t buffer[8192];
	ModuleFile *p_File;
} FC1x_Intern;

#endif /*__FutureComposer_H__*/
