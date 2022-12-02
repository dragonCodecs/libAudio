// SPDX-License-Identifier: BSD-3-Clause
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
/*!
 * @internal
 * @file libAudio_Common.cpp
 * @brief libAudio's old common routines
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2010-2019
 */
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "libAudio.h"
#include "libAudio.hxx"

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
