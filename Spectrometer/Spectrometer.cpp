#ifdef _WINDOWS
#include <windows.h>
#define _usleep _sleep
// The following hack is because M$ have a nasty habit of leaving this function out the C RunTime which is bad.
inline int round(double a)
{
	int b = int(a);
	double c = a - double(b);
	if (c >= 0.5)
		return b + 1;
	else if (c <= -0.5)
		return b - 1;
	else
		return b;
}
#else
#include <ctype.h>
#include <time.h>
#define MSECS_IN_SEC 1000
#define NSECS_IN_MSEC 1000000
#define _usleep(milisec) \
	{\
		struct timespec req = {milisec / MSECS_IN_SEC, (milisec % MSECS_IN_SEC) * NSECS_IN_MSEC}; \
		nanosleep(&req, NULL); \
	}
#include <inttypes.h>
#endif
#include <GTK++.h>
#include <libAudio.h>
#include "Playback.h"
#include "AboutBox.h"
#include <pthread.h>
#include "DrawingFunctions.h"
#include "VUMeter.h"
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
#ifdef __linux__
#include <sys/stat.h>
#endif
#include <math.h>
#include "Icon.h"

class Spectrometer;

Playback *p_Playback;
uint8_t *Buff;
void *p_AudioFile;
pthread_t SoundThread;
pthread_attr_t ThreadAttr;
Spectrometer *Interface;
pthread_mutex_t DrawMutex;
pthread_mutexattr_t MutexAttr;
#ifdef __linux__
char **files;
uint32_t fileCount;
#endif

class Spectrometer
{
private:
	GTKWindow *hMainWnd;
	GTKButton *btnOpen, *btnPlay, *btnPause, *btnNext;
	GTKGLDrawingArea *Spectr;
	short *Data;
	uint32_t lenData, fnNo;
	GdkWindow *Surface;
	DrawFn Function;
#ifdef __linux__
	uint32_t fileNo;
#endif
	bool FirstBuffer, PostValues;
	double VolL, VolR;
	VUMeter *LeftMeter, *RightMeter;

	static void btnOpen_Click(GtkWidget *widget, void *data)
	{
		Spectrometer *self = (Spectrometer *)data;
#ifndef __linux__
		std::vector<const char *> FileTypes, FileTypeNames;
		FileTypes.push_back("*.*");
		FileTypeNames.push_back("Any file type");
		char *FN = self->hMainWnd->FileOpen("Please select a file to open..", FileTypes, FileTypeNames);
#else
		char *FN;
		if (self->fileNo < fileCount)
		{
			FN = files[self->fileNo];
			self->fileNo++;
		}
		else
			FN = NULL;
#endif
		if (FN != NULL)
		{
			self->Data = NULL;
			if (p_Playback != NULL)
			{
				if (p_Playback->IsPlaying() == true)
				{
					p_Playback->Stop();
					pthread_mutex_lock(&DrawMutex);
					pthread_cancel(SoundThread);
					pthread_mutex_unlock(&DrawMutex);
				}
				delete p_Playback;
				p_Playback = NULL;
			}
			if (p_AudioFile != NULL)
			{
				Audio_CloseFileR(p_AudioFile);
				p_AudioFile = NULL;
			}
			if (Buff != NULL)
			{
				delete Buff;
				Buff = NULL;
			}

			p_AudioFile = Audio_OpenR(FN);
			if (p_AudioFile == NULL)
			{
				self->hMainWnd->MessageBox(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error, the file you requested could not be used for playback, please try again with another.",
					"libAudio Spectrometer");
#ifndef __linux__
				g_free(FN);
				FN = NULL;
#endif
				return;
			}
			p_FI = Audio_GetFileInfo(p_AudioFile);
			Buff = new uint8_t[8192];
			p_Playback = new Playback(p_FI, Callback, Buff, 8192, p_AudioFile);
			self->LeftMeter->ResetMeter();
			self->RightMeter->ResetMeter();

			self->btnPause->Disable();
			self->btnPlay->Enable();
		}
#ifndef __linux__
		g_free(FN);
		FN = NULL;
#endif
	}

	static bool Draw(GtkWidget *widget, GdkEventExpose *event, void *data)
	{
		Spectrometer *self = (Spectrometer *)data;
		pthread_mutex_lock(&DrawMutex);
		self->Spectr->glBegin();

		if (self->Data != NULL && self->Function != NULL)
		{
			self->Function(self->Data, self->lenData);
			self->UpdateVUs();
		}

		self->Spectr->glSwapBuffers();
		self->Spectr->glEnd();

		pthread_mutex_unlock(&DrawMutex);
		return false;
	}

	static long Callback(void *p_AudioPtr, uint8_t *OutBuffer, int nOutBufferLen)
	{
		long ret;
		static GdkRectangle rect = {0, 0, 456, 214};
		pthread_mutex_lock(&DrawMutex);
		ret = Audio_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);

		if (ret > 0)
		{
			Interface->Data = (short *)OutBuffer;
			Interface->lenData = ret;
		}

		pthread_mutex_unlock(&DrawMutex);
		return ret;
	}

	static void *PlaybackThread(void *)
	{
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

		Interface->Spectr->AddTimeout();
		p_Playback->Play();
		Interface->Spectr->RemoveTimeout();
		return 0;
	}

	static void btnPlay_Click(GtkWidget *, void *data)
	{
		Spectrometer *self = (Spectrometer *)data;
		pthread_attr_init(&ThreadAttr);
		pthread_attr_setdetachstate(&ThreadAttr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setscope(&ThreadAttr, PTHREAD_SCOPE_PROCESS);
		pthread_create(&SoundThread, &ThreadAttr, PlaybackThread, NULL);
		pthread_attr_destroy(&ThreadAttr);
		_usleep(10);
		self->btnPlay->Disable();
		self->btnPause->Enable();
	}

	static void btnPause_Click(GtkWidget *, void *data)
	{
		Spectrometer *self = (Spectrometer *)data;
		p_Playback->Pause();
		pthread_mutex_lock(&DrawMutex);
		pthread_cancel(SoundThread);
		pthread_mutex_unlock(&DrawMutex);
		self->btnPause->Disable();
		self->btnPlay->Enable();
	}

	static void btnNext_Click(GtkWidget *, void *data)
	{
		Spectrometer *self = (Spectrometer *)data;
		pthread_mutex_lock(&DrawMutex);
		self->fnNo = (self->fnNo + 1) % nFunctions;
		self->Function = Functions[self->fnNo];
		pthread_mutex_unlock(&DrawMutex);
	}

	static void btnAbout_Click(GtkWidget *, void *data)
	{
		AboutBox *about;
		Spectrometer *self = (Spectrometer *)data;
		about = new AboutBox(self->hMainWnd);
		about->Run();
		delete about;
	}

	void UpdateVUs()
	{
		if (p_FI->Channels == 1)
			UpdateMonoVUs();
		else
			UpdateStereoVUs();
		if (PostValues == false)
			PostValues = true;
		else
		{
			LeftMeter->SetValue(VolL);
			RightMeter->SetValue(VolR);
			if (p_FI->Channels != 1)
				PostValues = false;
		}
	}

	void UpdateMonoVUs()
	{
		uint32_t i;
		double Vol;
		if (p_FI->BitsPerSample == 8)
		{
			signed char *Data = (signed char *)this->Data;
			if (FirstBuffer == true)
			{
				Vol = Data[0] * Data[0];
				FirstBuffer = false;
			}
			else
				Vol = VolL * VolL;
			for (i = 0; i < lenData; i++)
			{
				Vol += (Data[i] * Data[i]) << 8;
				Vol /= 2;
			}
		}
		else
		{
			if (FirstBuffer == true)
			{
				Vol = Data[0] * Data[0];
				FirstBuffer = false;
			}
			else
				Vol = VolL * VolL;
			for (i = 0; i < lenData / 2; i++)
			{
				Vol += Data[i] * Data[i];
				Vol /= 2;
			}
		}
		VolR = VolL = round(sqrt(Vol));
	}

	void UpdateStereoVUs()
	{
		uint32_t i;
		double L, R;
		if (p_FI->BitsPerSample == 8)
		{
			signed char *Data = (signed char *)this->Data;
			if (FirstBuffer == true)
			{
				L = Data[0] * Data[0];
				R = Data[1] * Data[1];
				FirstBuffer = false;
			}
			else
			{
				L = VolL * VolL;
				R = VolR * VolR;
			}
			for (i = 0; i < lenData / 2; i++)
			{
				uint32_t j = i * 2;
				L += (Data[j + 0] * Data[j + 0]) << 8;
				R += (Data[j + 1] * Data[j + 1]) << 8;
				L /= 2;
				R /= 2;
			}
		}
		else
		{
			if (FirstBuffer == true)
			{
				L = Data[0] * Data[0];
				R = Data[1] * Data[1];
				FirstBuffer = false;
			}
			else
			{
				L = VolL * VolL;
				R = VolR * VolR;
			}
			for (i = 0; i < lenData / 4; i++)
			{
				uint32_t j = i * 2;
				L += (Data[j + 0] * Data[j + 0]);
				R += (Data[j + 1] * Data[j + 1]);
				L /= 2;
				R /= 2;
			}
		}
		VolL = sqrt(L);
		VolR = sqrt(R);
	}

public:
	Spectrometer() : FirstBuffer(true), PostValues(false), VolL(0), VolR(0), Data(NULL), LeftMeter(new VUMeter()), RightMeter(new VUMeter())
	{
		GTKFrame *frame;
		GTKButton *btnAbout;
		GTKHBox *horBox;
		GTKVBox *verBox;

		Icon::SetIcons();
		hMainWnd = new GTKWindow(GTK_WINDOW_TOPLEVEL);
		hMainWnd->SetLocation(GTK_WIN_POS_CENTER);
		hMainWnd->SetTitle("libAudio Spectrometer");
		hMainWnd->SetResizable(FALSE);
		hMainWnd->SetBorder(7);
		verBox = new GTKVBox(FALSE, 7);
		horBox = new GTKHBox(FALSE, 7);
		horBox->AddChild(LeftMeter->GetWidget());
		horBox->AddChild(verBox);
		horBox->AddChild(RightMeter->GetWidget());
		hMainWnd->AddChild(horBox);
		horBox = new GTKHBox(FALSE, 7);
		btnOpen = new GTKButton("_Open file");
		btnOpen->SetOnClicked((void *)btnOpen_Click, this);
		horBox->AddChild(btnOpen);
		btnPlay = new GTKButton("_Play");
		btnPlay->Disable();
		btnPlay->SetOnClicked((void *)btnPlay_Click, this);
		horBox->AddChild(btnPlay);
		btnPause = new GTKButton("_Pause");
		btnPause->Disable();
		btnPause->SetOnClicked((void *)btnPause_Click, this);
		horBox->AddChild(btnPause);
		btnNext = new GTKButton("_Next Visualisation");
		btnNext->SetOnClicked((void *)btnNext_Click, this);
		horBox->AddChild(btnNext);
		btnAbout = new GTKButton("_About");
		btnAbout->SetOnClicked((void *)btnAbout_Click, this);
		horBox->AddChild(btnAbout);
		frame = new GTKFrame("Visualisation");
		verBox->AddChild(frame);
		verBox->AddChild(horBox);
		horBox = new GTKHBox(FALSE);
		horBox->SetBorder(2);
		frame->AddChild(horBox);
		Spectr = new GTKGLDrawingArea(456, 214, GLBase::MakeStandardConfig());
		Spectr->SetHandler("expose_event", (void *)Draw, this);
		horBox->AddChild(Spectr);

		p_AudioFile = NULL;
		Buff = NULL;
		p_Playback = NULL;
		ExternalPlayback = TRUE;
#ifdef __linux__
		fileNo = 0;
#endif

		pthread_mutexattr_init(&MutexAttr);
		pthread_mutexattr_settype(&MutexAttr, PTHREAD_MUTEX_ERRORCHECK);
		pthread_mutex_init(&DrawMutex, &MutexAttr);
		pthread_mutexattr_destroy(&MutexAttr);
		fnNo = 1;
		Function = Functions[fnNo];
	}

	~Spectrometer()
	{
		//delete hMainWnd;
		Spectr = NULL;
		Surface = NULL;
		btnPause = NULL;
		btnPlay = NULL;
		btnOpen= NULL;
		hMainWnd = NULL;
	}

	void Run()
	{
		hMainWnd->ShowWindow();

		Surface = Interface->Spectr->GetWidget()->window;
		g_object_ref(G_OBJECT(Surface));
		Spectr->glBegin();

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glShadeModel(GL_SMOOTH);
		glRenderMode(GL_RENDER);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glLineWidth(1.0F);
//		glFrontFace(GL_CCW);
//		glCullFace(GL_BACK);

		glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
		glViewport(0, 0, 456, 214);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glOrtho(0.0, 456.0, 0.0, 214.0, 0.0, 1.0);

		glClear(GL_COLOR_BUFFER_BIT);

		Spectr->glSwapBuffers();
		Spectr->glEnd();

		hMainWnd->DoMessageLoop();

		if (p_Playback != NULL)
		{
			if (p_Playback->IsPlaying() == true)
			{
				p_Playback->Stop();
				pthread_mutex_lock(&DrawMutex);
				pthread_cancel(SoundThread);
				pthread_mutex_unlock(&DrawMutex);
			}
			delete p_Playback;
			p_Playback = NULL;
		}
		if (p_AudioFile != NULL)
		{
			Audio_CloseFileR(p_AudioFile);
			p_AudioFile = NULL;
		}

		g_object_unref(Surface);
	}
};

#ifdef __linux__
#define reallocFiles() \
	fileCount++; \
	files = (char **)realloc(files, sizeof(char *) * fileCount)

void findFiles(int argc, char **argv)
{
	fileCount = 0;
	files = NULL;
	for (int i = 1; i < argc; i++)
	{
		struct stat s;
		if (stat(argv[i], &s) == 0)
		{
			reallocFiles();
			files[fileCount - 1] = argv[i];
		}
	}
}

#undef reallocFiles
#endif

#if defined(_WINDOWS) && !defined(_CONSOLE)
#define argc __argc
#define argv __argv

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, char *CmdLine, int ShowCmd)
#else
int main(int argc, char **argv)
#endif
{
	GTKGL::GTKInit(argc, argv);

#ifdef __linux__
	findFiles(argc, argv);
	if (fileCount == 0)
		return 1;
#endif
	Interface = new Spectrometer();
	Interface->Run();
	delete Interface;
#ifdef __linux__
	free(files);
#endif

	return 0;
}
