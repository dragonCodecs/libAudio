#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
/*!
 * @internal
 * @file libAudio_Common.cpp
 * @brief libAudio's common routines, including the playback engine
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2012
 */
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "libAudio.h"
#include "libAudio_Common.h"

/*!
 * @internal
 * \c fseek_wrapper() is the internal seek callback for several file decoders.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file The \c FILE handle for the seek to use as a void pointer
 * @param offset A 64-bit integer giving the number of bytes from the \p origin to seek through
 * @param origin The location identifier to seek from
 * @return The return result of \c fseek() indiciating if the seek worked or not
 */
int fseek_wrapper(void *p_file, int64_t offset, int origin)
{
	if (p_file == NULL)
		return -1;
	return fseek((FILE *)p_file, (long)offset, origin);
}

/*!
 * @internal
 * \c ftell_wrapper() is the internal I/O possition callback for several file decoders.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_file The \c FILE handle for the seek to use as a void pointer
 * @return The possition of I/O in the file called for
 * @deprecated Marked as deprecated as nothing currently uses or calls this function,
 *   so I am considering removing it.
 */
int64_t ftell_wrapper(void *p_file)
{
	return ftell((FILE *)p_file);
}

#ifdef __NICE_OUTPUT__
void DoDisplay(int *p_CN, int *p_P, char *Chars)
{
	int lenChars = strlen(Chars);
	if (((*p_P) % lenChars) == 0)
	{
		fprintf(stdout, "%c\b", Chars[(*p_CN)]);
		fflush(stdout);
		(*p_CN)++;
		if (*p_CN == lenChars)
			*p_CN = 0;
	}
	(*p_P)++;
	if (*p_P == lenChars)
		*p_P = 0;
}
#endif

#ifdef __NICE_OUTPUT__
	static char *ProgressChars = "\xB3/\-\\";
	int CharNum = 0, Proc = 0;
#endif

#ifdef __NICE_OUTPUT__
	fprintf(stdout, "Playing: *\b");
	fflush(stdout);
#endif
#ifdef __NICE_OUTPUT__
		DoDisplay(&CharNum, &Proc, ProgressChars);
#endif
#ifdef __NICE_OUTPUT__
	printf(stdout, "*\n");
	fflush(stdout);
#endif
