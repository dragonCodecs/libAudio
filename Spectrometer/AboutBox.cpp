#include <windows.h>
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
	About->SetCopyright("Copyright \xC2\xA9 Richard Mant 2010\nFFT code copyright \xC2\xA9 Pjotr 1987");
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