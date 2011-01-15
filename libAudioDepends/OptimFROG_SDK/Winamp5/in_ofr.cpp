// in_ofr.cpp contains an OptimFROG Lossless/DualStream audio
// input plug-in for Winamp 5 playback

// Copyright (C) 2005 Florin Ghido
// Version: 1.10, Date: 2005.07.17


#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "in2.h"
#include "OptimFROGDecoder.h"


BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

#define WM_WA_MPEG_EOF WM_USER + 2

OptimFROGDecoder* ofrd = C_NULL;
extern In_Module mod;
char lastfn[MAX_PATH];
int decode_pos_ms;
int paused;
int seek_needed;
char sample_buffer[576 * 2 * (32 / 8) * 2 * 2];
int killDecodeThread = 0;
HANDLE thread_handle = INVALID_HANDLE_VALUE;

DWORD WINAPI __stdcall DecodeThread(void* b);


void config(HWND hwndParent)
{
    MessageBox(hwndParent, "No configuration available.", "Configuration", MB_OK);
}

void about(HWND hwndParent)
{
    char buffer[1024];
    uInt32_t version = OptimFROGDecoder::getVersion();
    sprintf(buffer,
            "Winamp 5 OptimFROG input plug-in, version 1.10 [2005.07.17]\n"
            "OptimFROG Lossless/DualStream audio DLL library, version %u.%03u\n"
            "Copyright (C) 1996-2005 Florin Ghido, all rights reserved.\n"
            "Visit http://www.LosslessAudio.org for updates\n"
            "Free for non-commercial use. E-mail: FlorinGhido@yahoo.com",
            version / 1000, version % 1000);

    MessageBox(hwndParent, buffer, "OptimFROG Lossless/DualStream Decoder", MB_OK);
}

void init()
{
    ofrd = new OptimFROGDecoder();
}

void quit()
{
    delete ofrd;
    ofrd = C_NULL;
}

int isourfile(char* fn)
{
    return 0;
}

int play(char* fn)
{
    if (!ofrd->open(fn, C_TRUE))
        return 1;

    strcpy(lastfn, fn);
    paused = 0;
    decode_pos_ms = 0;
    seek_needed = -1;

    int maxlatency = mod.outMod->Open(ofrd->samplerate, ofrd->channels, ofrd->bitspersample, -1, -1);
    if (maxlatency < 0)
    {
        ofrd->close();
        return 1;
    }
    mod.SetInfo(ofrd->bitrate, ofrd->samplerate / 1000, ofrd->channels, 1);

    mod.SAVSAInit(maxlatency, ofrd->samplerate);
    mod.VSASetInfo(ofrd->samplerate, ofrd->channels);

    mod.outMod->SetVolume(-666);

    killDecodeThread = 0;
    unsigned long thread_id;
    thread_handle = (HANDLE) CreateThread(C_NULL, 0, (LPTHREAD_START_ROUTINE) DecodeThread,
        (void*) &killDecodeThread, 0, &thread_id);
    SetThreadPriority(thread_handle, THREAD_PRIORITY_ABOVE_NORMAL);
    return 0;
}

void pause()
{
    paused = 1;
    mod.outMod->Pause(1);
}

void unpause()
{
    paused = 0;
    mod.outMod->Pause(0);
}

int ispaused()
{
    return paused;
}

void stop()
{
    if (thread_handle != INVALID_HANDLE_VALUE)
    {
        killDecodeThread = 1;
        if (WaitForSingleObject(thread_handle, 2000) == WAIT_TIMEOUT)
        {
            TerminateThread(thread_handle, 0);
        }
        CloseHandle(thread_handle);
        thread_handle = INVALID_HANDLE_VALUE;
    }
    ofrd->close();

    mod.outMod->Close();

    mod.SAVSADeInit();
}

int getlength()
{
    return (sInt32_t) ofrd->length_ms;
}

int getoutputtime()
{
    return decode_pos_ms + (mod.outMod->GetOutputTime() - mod.outMod->GetWrittenTime());
}

void setoutputtime(int time_in_ms)
{
    seek_needed = time_in_ms;
}

void setvolume(int volume)
{
    mod.outMod->SetVolume(volume);
}

void setpan(int pan)
{
    mod.outMod->SetPan(pan);
}

int infoDlg(char* fn, HWND hwnd)
{
    OptimFROGDecoder tmp;

    if (tmp.getFileDetails(fn, C_TRUE) != OptimFROG_NoError)
    {
        MessageBox(hwnd, "Not a valid OptimFROG 4.5xx or 4.2x file",
            "OptimFROG File Information", MB_OK);
        return 0;
    }

    char msg[2048];
    int msgLen = sprintf(msg,
                         "File name: %s\n"
                         "Original file size: %10u\n"
                         "Length: %7.1f s\n"
                         "Samplerate: %6u\n"
                         "Channels: %2u\n"
                         "Bits per sample: %2u\n\n"
                         "Version: %1u.%3u\n"
                         "Mode: %s\n"
                         "Speedup: %s\n"
                         "Bitrate: %4u kb\n",
                         fn, (uInt32_t) tmp.originalSize, tmp.length_ms / 1000.0, tmp.samplerate,
                         tmp.channels, tmp.bitspersample, tmp.version / 1000, tmp.version % 1000,
                         tmp.method, tmp.speedup, tmp.bitrate);
    if (tmp.keyCount != 0)
    {
        msgLen += sprintf(msg + msgLen, "\n");
        for (uInt32_t i = 0; i < tmp.keyCount; ++i)
        {
            msgLen += sprintf(msg + msgLen, "%s: %s\n", tmp.keys[i], tmp.values[i]);
        }
    }

    MessageBox(hwnd, msg, "OptimFROG File Information", MB_OK);
    return 0;
}

void getfileinfo(char* filename, char* title, int* length_in_ms)
{
    if (!filename || !*filename)
    {
        if (length_in_ms)
            *length_in_ms = getlength();
        if (title)
        {
            const char* tag_artist = ofrd->findTag("Artist");
            const char* tag_title = ofrd->findTag("Title");
            if (tag_title != C_NULL)
            {
                if (tag_artist != C_NULL)
                    sprintf(title, "%s - %s", tag_artist, tag_title);
                else
                    strcpy(title, tag_title);
            }
            else
            {
                char* p = strrchr(lastfn, '\\');
                if (p == C_NULL)
                    p = lastfn;
                else
                    p++;
                strcpy(title, p);
                p = strrchr(title, '.');
                if ((p != C_NULL) && ((stricmp(p, ".ofr") == 0) || (stricmp(p, ".ofs") == 0)))
                    *p = 0;
            }
        }
    }
    else
    {
        if (title)
        {
            char* p = strrchr(filename, '\\');
            if (p == C_NULL)
                p = filename;
            else
                p++;
            strcpy(title, p);
            p = strrchr(title, '.');
            if ((p != C_NULL) && ((stricmp(p, ".ofr") == 0) || (stricmp(p, ".ofs") == 0)))
                *p = 0;
        }
        if (length_in_ms)
            *length_in_ms = -1000;
        if (title)
        {
            OptimFROGDecoder tmp;
            if (tmp.getFileDetails(filename, C_TRUE) != OptimFROG_NoError)
            {
                return;
            }
            const char* tag_artist = tmp.findTag("Artist");
            const char* tag_title = tmp.findTag("Title");
            if (tag_title != C_NULL)
            {
                if (tag_artist != C_NULL)
                    sprintf(title, "%s - %s", tag_artist, tag_title);
                else
                    strcpy(title, tag_title);
            }
            if (length_in_ms)
                *length_in_ms = (sInt32_t) tmp.length_ms;
        }
    }
}

void eq_set(int on, char data[10], int preamp)
{
}

DWORD WINAPI __stdcall DecodeThread(void* b)
{
    int done = 0;
    while (!*((int*) b))
    {
        if (seek_needed != -1)
        {
            decode_pos_ms = seek_needed - (seek_needed % 1000);
            seek_needed = -1;
            done = 0;
            mod.outMod->Flush(decode_pos_ms);
            if (!ofrd->seekTimeMillis(decode_pos_ms))
            {
                decode_pos_ms = -1;
                done = 1;
            }
        }
        if (done)
        {
            mod.outMod->CanWrite();
            if (!mod.outMod->IsPlaying())
            {
                PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
                ofrd->close();
                return 0;
            }
            Sleep(20);
        }
        else if (mod.outMod->CanWrite() >=
            (int) ((576 * ofrd->channels * (ofrd->bitspersample / 8)) << (mod.dsp_isactive() ? 1 : 0)))
        {
            sInt32_t points_read = ofrd->readPoints(sample_buffer, 576);
            if (points_read <= 0)
            {
                done = 1;
            }
            else
            {
                sInt32_t samples = points_read * ofrd->channels;
                mod.SAAddPCMData(sample_buffer, ofrd->channels, ofrd->bitspersample, decode_pos_ms);
                mod.VSAAddPCMData(sample_buffer, ofrd->channels, ofrd->bitspersample, decode_pos_ms);
                decode_pos_ms += (points_read * 1000) / ofrd->samplerate;
                if (mod.dsp_isactive())
                {
                    samples = ofrd->channels * mod.dsp_dosamples((short*) sample_buffer,
                        samples / ofrd->channels, ofrd->bitspersample, ofrd->channels, ofrd->samplerate);
                }
                mod.outMod->Write(sample_buffer, samples * (ofrd->bitspersample / 8));
            }
        }
        else
            Sleep(20);
    }
    return 0;
}

In_Module mod =
{
    IN_VER,
    "OptimFROG Lossless/DualStream Decoder v1.10",
    0,
    0,
    "OFR\0OptimFROG Lossless Audio File (*.OFR)\0OFS\0OptimFROG DualStream Audio File (*.OFS)\0",
    1,
    1,

    config,
    about,
    init,
    quit,
    getfileinfo,
    infoDlg,
    isourfile,
    play,
    pause,
    unpause,
    ispaused,
    stop,

    getlength,
    getoutputtime,
    setoutputtime,

    setvolume,
    setpan,

    0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0,

    eq_set,

    C_NULL,

    0
};


extern "C" __declspec(dllexport) In_Module* winampGetInModule2()
{
    return &mod;
}
