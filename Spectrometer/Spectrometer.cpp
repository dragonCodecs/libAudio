#include <Windows.h>
#include <GTK++.h>
#include <libAudio.h>
#include "Playback.h"
#include "AboutBox.h"
#include <pthread.h>
#include "DrawingFunctions.h"
#include <windows.h>
#include <GL/GL.h>
#include <GL/GLU.h>

class Spectrometer;

Playback *p_Playback;
BYTE *Buff;
void *p_AudioFile;
pthread_t SoundThread;
pthread_attr_t ThreadAttr;
int Type;
Spectrometer *Interface;
pthread_mutex_t DrawMutex;
pthread_mutexattr_t MutexAttr;

class Spectrometer
{
private:
	GTKWindow *hMainWnd;
	GTKButton *btnOpen, *btnPlay, *btnPause, *btnNext;
	GTKGLDrawingArea *Spectr;
	short *Data;
	int lenData, fnNo;
	GdkWindow *Surface;
	DrawFn Function;

	static void btnOpen_Click(GtkWidget *widget, void *data)
	{
		Spectrometer *self = (Spectrometer *)data;
		std::vector<const char *> FileTypes, FileTypeNames;
		FileTypes.push_back("*.*");
		FileTypeNames.push_back("Any file type");
		char *FN = self->hMainWnd->FileOpen("Please select a file to open..", FileTypes, FileTypeNames);
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
				Audio_CloseFileR(p_AudioFile, Type);
				p_AudioFile = NULL;
			}
			if (Buff != NULL)
			{
				delete Buff;
				Buff = NULL;
			}

			p_AudioFile = Audio_OpenR(FN, &Type);
			if (p_AudioFile == NULL)
			{
				GTKMessageBox *hMsgBox = new GTKMessageBox((GtkWindow *)self->hMainWnd->GetWindow(), GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					"Error, the file you requested could not be used for playback, please try again with another.", "libAudio Spectrum");
				hMsgBox->Run();
				delete hMsgBox;
				g_free(FN);
				FN = NULL;
				return;
			}
			p_FI = Audio_GetFileInfo(p_AudioFile, Type);
			Buff = new BYTE[8192];
			p_Playback = new Playback(p_FI, Callback, Buff, 8192, p_AudioFile);

			self->btnPause->Disable();
			self->btnPlay->Enable();
		}
		g_free(FN);
		FN = NULL;
	}

	static BOOL Draw(GtkWidget *widget, GdkEventExpose *event, void *data)
	{
		Spectrometer *self = (Spectrometer *)data;
		pthread_mutex_lock(&DrawMutex);
		if (self->Data == NULL)
		{
			self->Spectr->glBegin();
			self->Spectr->glSwapBuffers();
			self->Spectr->glEnd();
			pthread_mutex_unlock(&DrawMutex);
			return TRUE;
		}

		self->Spectr->glBegin();

		if (self->Function != NULL)
			self->Function(self->Data, self->lenData);

		self->Spectr->glSwapBuffers();
		self->Spectr->glEnd();

		pthread_mutex_unlock(&DrawMutex);
		return TRUE;
	}

	static long Callback(void *p_AudioPtr, BYTE *OutBuffer, int nOutBufferLen)
	{
		long ret;
		static GdkRectangle rect = {0, 0, 456, 214};
		pthread_mutex_lock(&DrawMutex);
		if (Type == AUDIO_OGG_VORBIS)
			ret = OggVorbis_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
		else if (Type == AUDIO_AAC)
			ret = AAC_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
		else if (Type == AUDIO_FLAC)
			ret = FLAC_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
		else if (Type == AUDIO_MP3)
			ret = MP3_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
		else if (Type == AUDIO_M4A)
			ret = M4A_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
/*		else if (Type == AUDIO_MUSEPACK)
			ret = MPC_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);*/
		else if (Type == AUDIO_OPTIMFROG)
			ret = OptimFROG_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
		else if (Type == AUDIO_WAVE)
			ret = WAV_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
		else if (Type == AUDIO_WAVPACK)
			ret = WavPack_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
		else if (Type == AUDIO_WMA)
			ret = WMA_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);
		else
			ret = 0;
		if (ret <= 0)
		{
			pthread_mutex_unlock(&DrawMutex);
			return ret;
		}

		Interface->Data = (short *)OutBuffer;
		Interface->lenData = ret;
		gdk_window_invalidate_rect(Interface->Surface, &rect, FALSE);
		pthread_mutex_unlock(&DrawMutex);

		return ret;
	}

	static void *PlaybackThread(void *)
	{
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

		p_Playback->Play();
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
		Sleep(0);
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

public:
	Spectrometer()
	{
		GTKFixed *fixed;
		GTKFrame *frame;
		GTKButton *btnAbout;

		hMainWnd = new GTKWindow(GTK_WINDOW_TOPLEVEL);
		hMainWnd->SetLocation(GTK_WIN_POS_CENTER);
		hMainWnd->SetSize(450, 300);
		hMainWnd->SetTitle("libAudio Spectrometer");
		hMainWnd->SetResizable(FALSE);
		fixed = new GTKFixed(hMainWnd, 500, 300);
		btnOpen = new GTKButton("_Open file");
		btnOpen->SetSize(100, 25);
		btnOpen->SetOnClicked(btnOpen_Click, this);
		fixed->SetLocation(btnOpen, 16, 259);
		btnPlay = new GTKButton("_Play");
		btnPlay->SetSize(75, 25);
		btnPlay->Disable();
		btnPlay->SetOnClicked(btnPlay_Click, this);
		fixed->SetLocation(btnPlay, 124, 259);
		btnPause = new GTKButton("_Pause");
		btnPause->SetSize(75, 25);
		btnPause->Disable();
		btnPause->SetOnClicked(btnPause_Click, this);
		fixed->SetLocation(btnPause, 207, 259);
		btnNext = new GTKButton("_Next Visualisation");
		btnNext->SetSize(125, 25);
		btnNext->SetOnClicked(btnNext_Click, this);
		fixed->SetLocation(btnNext, 290, 259);
		btnAbout = new GTKButton("_About");
		btnAbout->SetSize(60, 25);
		btnAbout->SetOnClicked(btnAbout_Click, this);
		fixed->SetLocation(btnAbout, 423, 259);
		frame = new GTKFrame(fixed, 468, 235, 16, 16, "Visualisation");
		Spectr = new GTKGLDrawingArea(456, 214, GTKGLWidget::MakeStandardConfig());
		Spectr->glSetHandler("expose_event", Draw, this);
		frame->SetLocation((GTKWidget *)Spectr->GetGTKWidget(), 4, 2);

		p_AudioFile = NULL;
		Buff = NULL;
		p_Playback = NULL;
		ExternalPlayback = TRUE;
		Data = NULL;

		pthread_mutexattr_init(&MutexAttr);
		pthread_mutexattr_settype(&MutexAttr, PTHREAD_MUTEX_ERRORCHECK);
		pthread_mutex_init(&DrawMutex, &MutexAttr);
		pthread_mutexattr_destroy(&MutexAttr);
		fnNo = 1;
		Function = Functions[fnNo];
	}

	~Spectrometer()
	{
		delete hMainWnd;
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

		Surface = ((GTKWidget *)Interface->Spectr->GetGTKWidget())->GetWidget()->window;
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

		g_object_unref(Surface);
	}
};

#if defined(_WINDOWS) && !defined(_CONSOLE)
#define argc __argc
#define argv __argv

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, char *CmdLine, int ShowCmd)
#else
int main(int argc, char **argv)
#endif
{
	GTKGL::GTKInit(argc, argv);

	Interface = new Spectrometer();
	Interface->Run();
	delete Interface;
}