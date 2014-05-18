#define __DRAWINGFUNCTIONS_CPP__ 1
#ifdef _WINDOWS
#include <windows.h>
#endif
#if defined(__MACOSX__)
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#elif defined(__MACOS__)
	#include <gl.h>
	#include <glu.h>
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif
#define _USE_MATH_DEFINES 1
#include <math.h>
#include "FFT.h"
#include "DrawingFunctions.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

void SideBySideHor_Osc(short *Data, int lenData)
{
	int m, n, BPS, Channel, o, width, Channels;

	// Work  out how many samples there are per channel in the output buffer.
	Channels = p_FI->Channels;
	BPS = (p_FI->BitsPerSample / 8);
	m = (lenData / Channels) / BPS;

	width = ((m * Channels) + Channels) - 1;
	glLoadIdentity();
	glOrtho(0.0, (double)width, -(0x7FFF), 0x7FFF, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	o = 0;
	for (Channel = 0; Channel < Channels; Channel++)
	{
		glBegin(GL_LINE_STRIP);
			glColor4f(1.0, 1.0, 1.0, 1.0);
		for (n = 0; n < m; n++, o++)
			glVertex2d(o, Data[n * Channels + Channel]);
		glEnd();
		if (o < width)
		{
			glBegin(GL_LINES);
				glColor4f(0.5, 0.5, 0.5, 1.0);
				glVertex2d(o, -(0x7FFF));
				glVertex2d(o, 0x7FFF);
			glEnd();
			o++;
		}
	}
}

void SideBySideVer_Osc(short *Data, int lenData)
{
	int m, n, BPS, Channel, Channels;

	Channels = p_FI->Channels;
	BPS = (p_FI->BitsPerSample / 8);
	m = (lenData / p_FI->Channels) / BPS;

	glLoadIdentity();
	glOrtho(0.0, m, 0, 0xFFFE * Channels, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	for (Channel = 0; Channel < Channels; Channel++)
	{
		glBegin(GL_LINE_STRIP);
			glColor4f(1.0, 1.0, 1.0, 1.0);
		for (n = 0; n < m; n++)
			glVertex2d(n, Data[n * Channels + Channel] + 0x7FFF + (0xFFFE * Channel));
		glEnd();
	}
}

void SideBySideVer_Spe(short *Data, int lenData)
{
	static COMPLEX points[4096], opoints[4096];
	static double log128 = log(128.0);
	int m, n, BPS, Channel, Channels;
	double m_d;

	Channels = p_FI->Channels;
	BPS = (p_FI->BitsPerSample / 8);
	m = (lenData / Channels) / BPS;
	m_d = (double)m / 2;

	glLoadIdentity();
	glOrtho(0.0, m_d, 0.0, log128 * p_FI->Channels, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	for (Channel = 0; Channel < Channels; Channel++)
	{
		double base = log128 * Channel;
		for (n = 0; n < m; n++)
		{
			points[n].re = Data[n * Channels + Channel];
			points[n].im = 0;
		}

		fft(points, m, opoints);

		glBegin(GL_LINES);
			glColor4f(0.5, 0.5, 0.5, 1.0);
			glVertex2d(0, base);
			glVertex2d(m / 2.0, base);
		glEnd();
		glBegin(GL_LINE_STRIP);
			glColor4f(1.0, 1.0, 1.0, 1.0);
		for (n = 0; n < m / 2; n++)
			glVertex2d(n, base + log(max(sqrt((opoints[n].re * opoints[n].re) + (opoints[n].im * opoints[n].im)) / 128.0, 1.0)));
		glEnd();
	}
}

void SideBySideVer_logSpe(short *Data, int lenData)
{
	static COMPLEX points[4096], opoints[4096];
	static double log128 = log(128.0);
	int m, n, BPS, Channel, Channels;
	double m_d, logm_d;

	Channels = p_FI->Channels;
	BPS = (p_FI->BitsPerSample / 8);
	m = (lenData / Channels) / BPS;
	m_d = (double)m / 2;
	logm_d = log(m_d);

	glLoadIdentity();
	glOrtho(0.0, logm_d, 0.0, log128 * Channels, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	for (Channel = 0; Channel < Channels; Channel++)
	{
		double base = log128 * Channel;
		for (n = 0; n < m; n++)
		{
			points[n].re = Data[n * Channels + Channel];
			points[n].im = 0;
		}

		fft(points, m, opoints);

		glBegin(GL_LINES);
			glColor4f(0.5, 0.5, 0.5, 1.0);
			glVertex2d(0, base);
			glVertex2d(logm_d, base);
		glEnd();
		glBegin(GL_LINE_STRIP);
			glColor4f(1.0, 1.0, 1.0, 1.0);
		for (n = 0; n < m / 2; n++)
			glVertex2d(log(n + 1.0), base + log(max(sqrt((opoints[n].re * opoints[n].re) + (opoints[n].im * opoints[n].im)) / 64.0, 1.0)));
		glEnd();
	}
}
