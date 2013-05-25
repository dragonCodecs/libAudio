#ifndef _WINDOWS
#include <inttypes.h>
#else
typedef unsigned int uint32_t;
#include <windows.h>
#endif
#include <GTK++.h>
#include <math.h>

#include "VUMeter.h"
#include "img/black_bar.h"
#include "img/red_bar.h"
#include "img/amber_bar.h"
#include "img/green_bar.h"

VUMeter::VUMeter()
{
	uint8_t i;
	GTKFixed *fixed;

	Widget = new GTKVBox(FALSE, 0);
	fixed = new GTKFixed(Widget, 104, 242);
	fixed->SetBackgroundColour(0, 0, 0);

	Black = new GDKPixbuf(black_bar);
	Red = new GDKPixbuf(red_bar);
	Amber = new GDKPixbuf(amber_bar);
	Green = new GDKPixbuf(green_bar);

	for (i = 0; i < 40; i++)
	{
		Bars[i] = new GTKImage(100, 4, true);
		fixed->SetLocation(Bars[i], 2, 2 + (i * 6));
	}
	ResetMeter();
}

VUMeter::~VUMeter()
{
}

void VUMeter::ResetMeter()
{
	uint8_t i;
	value = 0;

	for (i = 0; i < 40; i++)
		Bars[i]->SetImage(Black);
}

void VUMeter::SetValue(uint32_t Value)
{
	uint8_t i;
	double comp = 0;
	//20 * log10(32767) ~= 90
	//90 / 40 = 2.25, so each gap is about 2.25dB
	value += (Value == 0 ? -100.0 : 20.0 * log10((double)Value / 32767.0));
	value /= 2.0;

	for (i = 0; i < 40; i++)
	{
		comp -= 2.25;
		if (value > comp)
		{
			if (i < 4)
				Bars[i]->SetImage(Red);
			else if (i < 12)
				Bars[i]->SetImage(Amber);
			else
				Bars[i]->SetImage(Green);
		}
		else
			Bars[i]->SetImage(Black);
	}
}

GTKVBox *VUMeter::GetWidget()
{
	return Widget;
}
