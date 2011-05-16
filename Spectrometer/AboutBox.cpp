#ifdef _WINDOWS
#include <windows.h>
#endif
#include <GTK++.h>
#include "AboutBox.h"

static const char *Authors[] =
{
	"Richard Mant",
	"Pjotr",
	NULL
};

AboutBox::AboutBox(GTKWindow *Parent)
{
	About = new GTKAboutDialog(Parent);
	About->SetProgram("libAudio Spectrometer");
	About->SetCopyright("Program copyright \xC2\xA9 Richard Mant 2010\n"
		"FFT code copyright \xC2\xA9 Pjotr 1987\n"
		"Icon copyright \xC2\xA9 Marie-Christine Paquette 2011 <cricri440@hotmail.com>");
	About->SetComments("FFT code cleaned up and made into a more managable source-base by Richard Mant");
	About->SetVersion("0.1");
	About->SetAuthors(Authors);
}

AboutBox::~AboutBox()
{
	delete About;
}

void AboutBox::Run()
{
	About->Run();
}
