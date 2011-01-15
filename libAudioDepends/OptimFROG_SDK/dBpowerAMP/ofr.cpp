// ofr.cpp contains an OptimFROG Lossless/DualStream audio
// input plug-in for dBpowerAMP playback

// Copyright (C) 2005 Florin Ghido
// Version: 1.10, Date: 2005.07.17


#include <stdio.h>
#include <windows.h>
#include "SoundInputOutput.h"
#include "OptimFROGDecoder.h"


BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

extern "C" __declspec(dllexport)
void WhatFormats(int Position, char* Format, char* Description)
{
    switch (Position)
    {
    case 0:
        strcpy(Format, ".ofr");
        strcpy(Description, "OptimFROG Lossless Audio File (*.ofr)");
        break;
    case 1:
        strcpy(Format, ".ofs");
        strcpy(Description, "OptimFROG DualStream Audio File (*.ofs)");
        break;
    case 10000:
        strcpy(Format, "");
        strcpy(Description, "OptimFROG Lossless/DualStream Decoder v1.10");
        break;
    default:
        strcpy(Format, "");
        strcpy(Description, "");
        break;
    }
}

extern "C" __declspec(dllexport)
void* CreateANewDecoderObject(char* FileToOpen, ENOpenResult& ReturnError, bool& StreamsData,
    WAVEFORMATEX** WFXOutputFormat, bool& ProtectedStream)
{
    ProtectedStream = false;
    ReturnError = OR_OK;
    StreamsData = true;

    OptimFROGDecoder* ofrd = new OptimFROGDecoder();
    if (ofrd == C_NULL)
    {
        ReturnError = OR_MemError;
        return C_NULL;
    }

    WAVEFORMATEX* OutputFormat = new WAVEFORMATEX;
    if (OutputFormat == C_NULL)
    {
        delete ofrd;
        ofrd = C_NULL;
        ReturnError = OR_MemError;
        return C_NULL;
    }

    if (!ofrd->open(FileToOpen))
    {
        delete ofrd;
        ofrd = C_NULL;
        ReturnError = OR_CODECERROR;
        return C_NULL;
    }

    OutputFormat->wFormatTag = WAVE_FORMAT_PCM;
    OutputFormat->nChannels = (uInt16_t) ofrd->channels;
    OutputFormat->nSamplesPerSec = ofrd->samplerate;
    OutputFormat->wBitsPerSample = (uInt16_t) ofrd->bitspersample;
    OutputFormat->cbSize = 0;
    OutputFormat->nBlockAlign = (uInt16_t) ((ofrd->bitspersample / 8) * ofrd->channels);
    OutputFormat->nAvgBytesPerSec = ofrd->samplerate * (ofrd->bitspersample / 8) * ofrd->channels;
    *WFXOutputFormat = OutputFormat;

    return ofrd;
}

extern "C" __declspec(dllexport)
DWORD OutputWantsABlockOfData(void* Decoder, char* SentBuffer, DWORD BufferLen, bool& IsEOF,
    ENDecodeResult& DecodeResult, DWORD& dwDataStreamPosmSec, DWORD& dwStreamTotalLenghmSec)
{
    OptimFROGDecoder* ofrd = (OptimFROGDecoder*) Decoder;
    dwDataStreamPosmSec = (DWORD) ofrd->getTimeMillis();
    dwStreamTotalLenghmSec = (DWORD) ofrd->length_ms;
    sInt32_t bytes_read = ofrd->readBytes(SentBuffer, BufferLen);
    DecodeResult = DR_OK;
    IsEOF = false;

    if (bytes_read < 0)
    {
        DecodeResult = DR_CODECERROR;
        IsEOF = true;
        return 0;
    }
    if (bytes_read == 0)
    {
        IsEOF = true;
        return 0;
    }

    return bytes_read;
}

extern "C" __declspec(dllexport)
void SkipTo(void* Decoder, DWORD SkipPos)
{
    OptimFROGDecoder* ofrd = (OptimFROGDecoder*) Decoder;
    uInt32_t target_ms = (uInt32_t) (ofrd->length_ms * (SkipPos / 100.0));
    ofrd->seekTimeMillis(target_ms);
}

extern "C" __declspec(dllexport)
void SetVolume(void* Decoder, DWORD LeftVolume, DWORD RightVolume)
{
    // not handled
}

extern "C" __declspec(dllexport)
void Pause(void* Decoder)
{
    // we not bothered about this
}

extern "C" __declspec(dllexport)
void UnPause(void* Decoder)
{
    // not bothered
}

extern "C" __declspec(dllexport)
void DeleteADecoderObject(void* Decoder)
{
    OptimFROGDecoder* ofrd = (OptimFROGDecoder*) Decoder;

    delete ofrd;
    ofrd = C_NULL;
}

extern "C" __declspec(dllexport)
void ShowAboutOptionsPage(void)
{
    char buffer[1024];
    uInt32_t version = OptimFROGDecoder::getVersion();
    sprintf(buffer,
            "dBpowerAMP OptimFROG input plug-in, version 1.10 [2005.07.17]\n"
            "OptimFROG Lossless/DualStream audio DLL library, version %u.%03u\n"
            "Copyright (C) 1996-2005 Florin Ghido, all rights reserved.\n"
            "Visit http://www.LosslessAudio.org for updates\n"
            "Free for non-commercial use. E-mail: FlorinGhido@yahoo.com",
            version / 1000, version % 1000);

    MessageBox(C_NULL, buffer, "OptimFROG Lossless/DualStream Decoder", MB_OK | MB_TOPMOST);
}

extern "C" __declspec(dllexport)
void GetTrackInfo(char* FileName, int& Returnkbps, int& ReturnFrequency,
    int& ReturnChannels, int& ReturnLengthmsec, int& ReturnFileSize, char* ReturnArtist,
    char* ReturnTrack, char* ReturnAlbum, int& ReturnYear, int& ReturnPreference, char* RetGenre)
{
    OptimFROGDecoder ofrd;
    if (ofrd.getFileDetails(FileName, C_TRUE) != OptimFROG_NoError)
    {
        return;
    }

    Returnkbps = ofrd.bitrate;
    ReturnFrequency = ofrd.samplerate;
    ReturnChannels = ofrd.channels;
    ReturnLengthmsec = (uInt32_t) ofrd.length_ms;
    ReturnFileSize = (uInt32_t) ofrd.compressedSize;

    if (ofrd.keyCount != 0)
    {
        const char* artist = ofrd.findTag("Artist");
        if (artist != C_NULL)
            strcpy(ReturnArtist, artist);

        const char* title = ofrd.findTag("Title");
        if (title != C_NULL)
            strcpy(ReturnTrack, title);

        const char* album = ofrd.findTag("Album");
        if (album != C_NULL)
            strcpy(ReturnAlbum, album);

        const char* year = ofrd.findTag("Year");
        if (year != C_NULL)
            sscanf(year, "%u", &ReturnYear);

        const char* genre = ofrd.findTag("Genre");
        if (genre != C_NULL)
            strcpy(RetGenre, genre);
    }
}

extern "C" __declspec(dllexport)
void GetStringInfo(char* FileName, char* RetString)
{
    // not handled
}

extern "C" __declspec(dllexport)
void SetIDTagElement(char* FileName, char* Element, char* TagVal)
{
    // not handled
}

extern "C" __declspec(dllexport)
void GetIDTagElement(char* FileName, int Idx, char* RetElement, char* RetTagVal)
{
    OptimFROGDecoder ofrd;
    if (ofrd.getFileDetails(FileName, C_TRUE) != OptimFROG_NoError)
    {
        return;
    }

    if ((uInt32_t) Idx < ofrd.keyCount)
    {
        strcpy(RetElement, ofrd.keys[Idx]);
        strcpy(RetTagVal, ofrd.values[Idx]);
    }
    else
    {
        strcpy(RetElement, "");
        strcpy(RetTagVal, "");
    }
}
