#include <stdint.h>
#ifdef _WINDOWS
#define _WIN32_LEAN_AND_MEAN
#include <windows.h>
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
#endif
#include <chrono>
#include <thread>
#include <mutex>
#include <memory>
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

template<typename T> struct uniquePtr_t
	{ typedef std::unique_ptr<T> typeT; };
template<typename T> struct uniquePtr_t<T []>
	{ typedef std::unique_ptr<T []> typeArrT; };

template<typename T, typename... Args>
inline typename uniquePtr_t<T>::typeT make_unique(Args &&... args)
{
	typedef typename std::remove_const<T>::type typeT;
	return std::unique_ptr<T>(new typeT(std::forward<Args>(args)...));
}

template<typename T>
inline typename uniquePtr_t<T>::typeArrT make_unique(size_t N)
{
	typedef typename std::remove_const<typename std::remove_extent<T>::type>::type typeT;
	return std::unique_ptr<T>(new typeT[N]());
}

class Spectrometer;

std::unique_ptr<Playback> p_Playback;
std::unique_ptr<uint8_t []> Buff;
void *p_AudioFile;
std::unique_ptr<Spectrometer> Interface;
std::thread playThread;
std::mutex drawMutex;
#ifdef __linux__
std::vector<char *> files;
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

	void btnOpenClick()
	{
#ifndef __linux__
		std::vector<const char *> FileTypes, FileTypeNames;
		FileTypes.push_back("*.*");
		FileTypeNames.push_back("Any file type");
		char *file = hMainWnd->FileOpen("Please select a file to open..", FileTypes, FileTypeNames);
#else
		char *file = nullptr;
		if (fileNo < files.size())
		{
			file = files[fileNo];
			++fileNo;
		}
#endif
		if (file != nullptr)
		{
			Data = nullptr;
			if (p_Playback && p_Playback->IsPlaying())
				p_Playback->Stop();
			if (playThread.joinable())
				playThread.join();
			p_Playback = nullptr;
			if (p_AudioFile != nullptr)
			{
				Audio_CloseFileR(p_AudioFile);
				p_AudioFile = nullptr;
			}

			p_AudioFile = Audio_OpenR(file);
			if (p_AudioFile == nullptr)
			{
				hMainWnd->MessageBox(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error, the file you requested could not be used for playback, please try again with another.",
					"libAudio Spectrometer");
#ifndef __linux__
				g_free(file);
				file = nullptr;
#endif
				return;
			}
			p_FI = Audio_GetFileInfo(p_AudioFile);
			p_Playback = make_unique<Playback>(p_FI, Callback, Buff.get(), 8192, p_AudioFile);
			LeftMeter->ResetMeter();
			RightMeter->ResetMeter();

			btnPause->Disable();
			btnPlay->Enable();
		}
#ifndef __linux__
		g_free(file);
		file = nullptr;
#endif
	}

	bool draw()
	{
		std::lock_guard<std::mutex> drawLock(drawMutex);
		Spectr->glBegin();

		if (Data != nullptr && Function != nullptr)
		{
			Function(Data, lenData);
			UpdateVUs();
		}

		Spectr->glSwapBuffers();
		Spectr->glEnd();

		return false;
	}

	static long Callback(void *p_AudioPtr, uint8_t *OutBuffer, int nOutBufferLen)
	{
		std::lock_guard<std::mutex> drawLock(drawMutex);
		long ret = Audio_FillBuffer(p_AudioPtr, OutBuffer, nOutBufferLen);

		if (ret > 0)
		{
			Interface->Data = (short *)OutBuffer;
			Interface->lenData = ret;
		}

		return ret;
	}

	void playback()
	{
		Spectr->AddTimeout();
		p_Playback->Play();
		Spectr->RemoveTimeout();
	}

	void btnPlayClick()
	{
		playThread = std::thread([this](){ playback(); });
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		btnPlay->Disable();
		btnPause->Enable();
	}

	void btnPauseClick()
	{
		p_Playback->Pause();
		playThread.join();
		btnPause->Disable();
		btnPlay->Enable();
	}

	void btnNextClick()
	{
		std::lock_guard<std::mutex> drawLock(drawMutex);
		fnNo = (fnNo + 1) % nFunctions;
		Function = Functions[fnNo];
	}

	void btnAboutClick()
	{
		AboutBox *about = new AboutBox(hMainWnd);
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
	Spectrometer() : Data(nullptr), FirstBuffer(true), PostValues(false), VolL(0), VolR(0), LeftMeter(new VUMeter()), RightMeter(new VUMeter())
	{
		void (*const openClicked)(GtkWidget *, Spectrometer *) = [](GtkWidget *, Spectrometer *self) { self->btnOpenClick(); };
		void (*const playClicked)(GtkWidget *, Spectrometer *) = [](GtkWidget *, Spectrometer *self) { self->btnPlayClick(); };
		void (*const pauseClicked)(GtkWidget *, Spectrometer *) = [](GtkWidget *, Spectrometer *self) { self->btnPauseClick(); };
		void (*const nextClicked)(GtkWidget *, Spectrometer *) = [](GtkWidget *, Spectrometer *self) { self->btnNextClick(); };
		void (*const aboutClicked)(GtkWidget *, Spectrometer *) = [](GtkWidget *, Spectrometer *self) { self->btnAboutClick(); };
		bool (*const doDraw)(GtkWidget *, GdkEventExpose *, Spectrometer *) = [](GtkWidget *, GdkEventExpose *, Spectrometer *self) { return self->draw(); };
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
		btnOpen->SetOnClicked((void *)openClicked, this);
		horBox->AddChild(btnOpen);
		btnPlay = new GTKButton("_Play");
		btnPlay->Disable();
		btnPlay->SetOnClicked((void *)playClicked, this);
		horBox->AddChild(btnPlay);
		btnPause = new GTKButton("_Pause");
		btnPause->Disable();
		btnPause->SetOnClicked((void *)pauseClicked, this);
		horBox->AddChild(btnPause);
		btnNext = new GTKButton("_Next Visualisation");
		btnNext->SetOnClicked((void *)nextClicked, this);
		horBox->AddChild(btnNext);
		btnAbout = new GTKButton("_About");
		btnAbout->SetOnClicked((void *)aboutClicked, this);
		horBox->AddChild(btnAbout);
		frame = new GTKFrame("Visualisation");
		verBox->AddChild(frame);
		verBox->AddChild(horBox);
		horBox = new GTKHBox(FALSE);
		horBox->SetBorder(2);
		frame->AddChild(horBox);
		Spectr = new GTKGLDrawingArea(456, 214, GLBase::MakeStandardConfig());
		Spectr->SetHandler("expose_event", (void *)doDraw, this);
		horBox->AddChild(Spectr);

		p_AudioFile = nullptr;
		Buff = nullptr;
		p_Playback = nullptr;
		ExternalPlayback = TRUE;
#ifdef __linux__
		fileNo = 0;
#endif

		fnNo = 1;
		Function = Functions[fnNo];
	}

	~Spectrometer()
	{
		//delete hMainWnd;
		Spectr = nullptr;
		Surface = nullptr;
		btnPause = nullptr;
		btnPlay = nullptr;
		btnOpen= nullptr;
		hMainWnd = nullptr;
	}

	void Run()
	{
		Buff = make_unique<uint8_t []>(8192);
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

		if (p_Playback && p_Playback->IsPlaying())
			p_Playback->Stop();
		if (playThread.joinable())
			playThread.join();
		p_Playback = nullptr;
		if (p_AudioFile != nullptr)
		{
			Audio_CloseFileR(p_AudioFile);
			p_AudioFile = nullptr;
		}

		g_object_unref(Surface);
		Buff = nullptr;
	}
};

#ifdef __linux__
void findFiles(int argc, char **argv)
{
	files.clear();
	for (int i = 1; i < argc; ++i)
	{
		struct stat s;
		if (stat(argv[i], &s) == 0)
			files.push_back(argv[i]);
	}
}
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
	if (files.size() == 0)
		return 1;
#endif
	Interface = make_unique<Spectrometer>();
	Interface->Run();
	Interface = nullptr;

	return 0;
}
