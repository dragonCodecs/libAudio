/* Copyright (C) 1996-2006 Florin Ghido, all rights reserved. */


/* OFRSFX.c contains the self extracting stub */
/* which works with OptimFROG Lossless/DualStream files */

/* Version: 1.000b, Date: 2006.03.02, using the C interface */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "OptimFROG.h"

#if defined(CFG_MSCVER_WIN32_MIX86) || defined(CFG_GNUC_MINGW32_I386)
#include <windows.h>
#endif


sInt32_t fatalError(char* message)
{
    fprintf(stderr, "\nFatal error: %s!\n\n", message);
    return 1;
}

void myCallback(void* callBackParam, Float64_t percentage)
{
    fprintf(stderr, "\015Decompressing %3d%%", (sInt32_t) percentage);
}

sInt32_t decodeFile(char* sourceFile, char* destinationFile)
{
    sInt32_t result;

    printf("\n");
    printf("sourceFile:      %s\n", sourceFile);
    printf("destinationFile: %s\n", destinationFile);
    printf("\n");

    result = OptimFROG_decodeFile(sourceFile, destinationFile, myCallback, C_NULL);

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
    sInt32_t result;
    OptimFROG_Info iInfo;
    OptimFROG_Tags iTags;
    uInt32_t i;

    result = OptimFROG_infoFile(sourceFile, &iInfo, &iTags);

    if (result == OptimFROG_MemoryError)
        return fatalError("could not create instance");
    if (result == OptimFROG_OpenError)
        return fatalError("could not open source OFR/OFS file");

    printf("\n");
    printf("fileName: %s\n", sourceFile);
    printf("\n");
    printf("channels:                    %2u\n", iInfo.channels);
    printf("bitspersample:               %2u\n", iInfo.bitspersample);
    printf("samplerate:              %6u\n", iInfo.samplerate);
    printf("length:              %10.3f\n", iInfo.length_ms / 1000.0);
    printf("compressedSize:      %10.0f\n", (Float64_t) iInfo.compressedSize);
    printf("originalSize:        %10.0f\n", (Float64_t) iInfo.originalSize);
    printf("bitrate:                   %4u\n", iInfo.bitrate);
    printf("version:                   %4u\n", iInfo.version);
    printf("method:                  %s\n", iInfo.method);
    printf("speedup:                     %s\n", iInfo.speedup);
    printf("sampleType:              %s\n", iInfo.sampleType);
    printf("channelConfig:        %s\n", iInfo.channelConfig);
    printf("\n");

    for (i = 0; i < iTags.keyCount; i++)
    {
        printf("%s: %s\n", iTags.keys[i], iTags.values[i]);
    }
    printf("\n");

    OptimFROG_freeTags(&iTags);

    return 0;
}

int main(int argc, char* argv[])
{
    sInt32_t result = 2;

    fprintf(stderr, "\n");
    fprintf(stderr, "OptimFROG Lossless/DualStream Audio SFX Decompressor\n");
    fprintf(stderr, "Copyright (C) 1996-2006 Florin Ghido, all rights reserved.\n");
    fprintf(stderr, "Visit http://www.LosslessAudio.org for updates\n");
    fprintf(stderr, "Free for non-commercial use. E-mail: FlorinGhido@yahoo.com\n");
    fprintf(stderr, "OptimFROG (Win32), v4.520b [2006.03.02]\n");
    fprintf(stderr, "\n");

    if ((argc == 2) && (strcmp(argv[1], "i") != 0))
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "%s [i] [d destinationFile]\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "Arguments:\n");
        fprintf(stderr, "                      no arguments, decode the contained OFR/OFS file\n");
        fprintf(stderr, "  d destinationFile   decode the contained OFR/OFS file to the specified file\n");
        fprintf(stderr, "  i                   print detailed information about contained OFR/OFS file\n");
    }

    if ((argc == 1) || ((argc == 3) && (strcmp(argv[1], "d") == 0)))
    {
        char* destinationFile;

        char temp[300];
        if (argc == 1)
        {
            sInt32_t pos;

            strcpy(temp, argv[0]);

            pos = strlen(temp) - 1;
            while ((pos >= 0) && (temp[pos] != '.') && (temp[pos] != '\\') && (temp[pos] != '/'))
                pos--;
            if ((pos >= 0) && (temp[pos] == '.'))
                temp[pos] = 0;

            strcat(temp, ".wav");
            destinationFile = temp;
        }
        else
            destinationFile = argv[2];

        result = decodeFile(argv[0], destinationFile);
    }
    else if (strcmp(argv[1], "i") == 0)
    {
        result = infoFile(argv[0]);
    }

#if defined(CFG_MSCVER_WIN32_MIX86) || defined(CFG_GNUC_MINGW32_I386)
    if (result != 0)
    {
        Sleep(INFINITE);
        return result;
    }
#endif

    return result;
}
