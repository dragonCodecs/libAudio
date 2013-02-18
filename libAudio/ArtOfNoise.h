#ifndef __ArtOfNoise_H__
#define __ArtOfNoise_H__

typedef struct _AON_Intern
{
	FILE *f_AON;
	FileInfo *p_FI;
	Playback *p_Playback;
	uint8_t buffer[8192];
	ModuleFile *p_File;
} AON_Intern;

#endif /*__ArtOfNoise_H__*/
