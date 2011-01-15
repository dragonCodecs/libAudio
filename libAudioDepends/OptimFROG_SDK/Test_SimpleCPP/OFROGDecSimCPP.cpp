// Copyright (C) 1996-2005 Florin Ghido, all rights reserved.


// OFROGDecSim.cpp contains a simple example usage for OptimFROG.dll/.so
// which works with OptimFROG Lossless/DualStream files

// Version: 1.200, Date: 2005.07.17, using the C++ interface


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "OptimFROGDecoder.h"


sInt32_t fatalError(char* message)
{
    fprintf(stderr, "\nFatal error: %s!\n\n", message);
    return 1;
}

void myCallback(void*, Float64_t percentage)
{
    fprintf(stderr, "\015Decompressing %3d%%", (sInt32_t) percentage);
}

sInt32_t decodeFile(char* sourceFile, char* destinationFile)
{
    printf("\n");
    printf("sourceFile:      %s\n", sourceFile);
    printf("destinationFile: %s\n", destinationFile);
    printf("\n");

    sInt32_t result = OptimFROGDecoder::decodeFile(sourceFile, destinationFile, myCallback);

    if (result == OptimFROG_MemoryError)
        return fatalError("could not create instance");
    if (result == OptimFROG_OpenError)
        return fatalError("could not open source OFR/OFS file");
    if (result == OptimFROG_WriteError)
        return fatalError("could not open destination file");
    if (result == OptimFROG_FatalError)
        return fatalError("could not read source file");

    fprintf(stderr, "\015Decompressing done.\n");

    if (result == OptimFROG_RecoverableError)
        fprintf(stderr, "recoverable errors occured decompressing %s\n", sourceFile);
    printf("\n");

    if (result == OptimFROG_RecoverableError)
        return 1;
    else
        return 0;
}

sInt32_t infoFile(char* sourceFile)
{
    OptimFROGDecoder ofr_dec;

    sInt32_t result = ofr_dec.getFileDetails(sourceFile, C_TRUE);

    if (result == OptimFROG_MemoryError)
        return fatalError("could not create instance");
    if (result == OptimFROG_OpenError)
        return fatalError("could not open source OFR/OFS file");

    printf("\n");
    printf("fileName: %s\n", sourceFile);
    printf("\n");
    printf("channels:                    %2u\n", ofr_dec.channels);
    printf("bitspersample:               %2u\n", ofr_dec.bitspersample);
    printf("samplerate:              %6u\n", ofr_dec.samplerate);
    printf("length:              %10.3lf\n", ofr_dec.length_ms / 1000.0);
    printf("compressedSize:      %10.0lf\n", (Float64_t) ofr_dec.compressedSize);
    printf("originalSize:        %10.0lf\n", (Float64_t) ofr_dec.originalSize);
    printf("bitrate:                   %4u\n", ofr_dec.bitrate);
    printf("version:                   %4u\n", ofr_dec.version);
    printf("method:                  %s\n", ofr_dec.method);
    printf("speedup:                     %s\n", ofr_dec.speedup);
    printf("sampleType:              %s\n", ofr_dec.sampleType);
    printf("channelConfig:        %s\n", ofr_dec.channelConfig);
    printf("\n");

    for (uInt32_t i = 0; i < ofr_dec.keyCount; i++)
    {
        printf("%s: %s\n", ofr_dec.keys[i], ofr_dec.values[i]);
    }
    printf("\n");

    return 0;
}

int main(int argc, char* argv[])
{
    fprintf(stderr, "\n");
    fprintf(stderr, "OptimFROG Lossless/DualStream Audio Decompressor [Simple C++]\n");
    fprintf(stderr, "Copyright (C) 1996-2005 Florin Ghido, all rights reserved.\n");
    fprintf(stderr, "Visit http://www.LosslessAudio.org for updates\n");
    fprintf(stderr, "Free for non-commercial use. E-mail: FlorinGhido@yahoo.com\n");
    fprintf(stderr, "OptimFROG.dll version: %4u\n", OptimFROG_getVersion());
    fprintf(stderr, "\n");

    if (argc < 3)
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "OFROGDecSim {d|i} sourceFile [destinationFile]\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Commands:\n");
        fprintf(stderr, "    d            decode an OFR/OFS file to a WAV/RAW file\n");
        fprintf(stderr, "    i            print information about an OFR/OFS file\n");
        return 2;
    }

    if (strcmp(argv[1], "d") == 0)
    {
        char* destinationFile;

        char temp[300];
        if (argc == 3)
        {
            strcpy(temp, argv[2]);

            sInt32_t pos = strlen(temp) - 1;
            while ((pos >= 0) && (temp[pos] != '.') && (temp[pos] != '\\') && (temp[pos] != '/'))
                pos--;
            if ((pos >= 0) && (temp[pos] == '.'))
                temp[pos] = 0;

            strcat(temp, ".wav");
            destinationFile = temp;
        }
        else
            destinationFile = argv[3];

        return decodeFile(argv[2], destinationFile);
    }
    else if (strcmp(argv[1], "i") == 0)
    {
        return infoFile(argv[2]);
    }

    return 2;
}
