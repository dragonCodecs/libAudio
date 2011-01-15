/* Copyright (C) 1996-2006 Florin Ghido, all rights reserved. */


/* OFROGDec.cpp contains an example usage for OptimFROG.dll/.so */
/* which works with OptimFROG Lossless/DualStream files */

/* Version: 1.210, Date: 2006.03.02, using the C interface */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "OptimFROG.h"


sInt32_t fatalError(char* message)
{
    fprintf(stderr, "\nFatal error: %s!\n\n", message);
    return 1;
}

#define READ_BUFFER_SIZE 176400
sInt32_t decodeFile(char* sourceFile, char* destinationFile)
{
    void* decoderInstance;
    uInt8_t* buffer;
    condition_t opened;
    OptimFROG_Info iInfo;
    FILE* fDst;
    sInt32_t headSize;
    uInt32_t pointsToRead;
    sInt64_t totalPointsRead;
    sInt32_t tailSize;
    condition_t recErrors;

    printf("\n");
    printf("sourceFile:      %s\n", sourceFile);
    printf("destinationFile: %s\n", destinationFile);
    printf("\n");

    decoderInstance = OptimFROG_createInstance();
    if (decoderInstance == C_NULL)
        return fatalError("could not create instance");

    buffer = malloc(READ_BUFFER_SIZE);
    if (buffer == C_NULL)
    {
        OptimFROG_destroyInstance(decoderInstance);
        return fatalError("could not allocate decode buffer");
    }

    opened = OptimFROG_open(decoderInstance, sourceFile, C_FALSE);
    if (!opened)
    {
        OptimFROG_destroyInstance(decoderInstance);
        free(buffer);
        return fatalError("could not open source OFR/OFS file");
    }

    OptimFROG_getInfo(decoderInstance, &iInfo);

    fDst = fopen(destinationFile, "wb");
    if (fDst == C_NULL)
    {
        OptimFROG_close(decoderInstance);
        OptimFROG_destroyInstance(decoderInstance);
        free(buffer);
        return fatalError("could not open destination file");
    }

    headSize = OptimFROG_readHead(decoderInstance, buffer, READ_BUFFER_SIZE);
    if (fwrite(buffer, 1, headSize, fDst) != (uInt32_t) headSize)
    {
        OptimFROG_close(decoderInstance);
        OptimFROG_destroyInstance(decoderInstance);
        free(buffer);
        fclose(fDst);
        return fatalError("could not write to destination file");
    }

    pointsToRead = READ_BUFFER_SIZE / ((iInfo.bitspersample / 8) * iInfo.channels);
    totalPointsRead = 0;
    while (totalPointsRead < iInfo.noPoints)
    {
        sInt32_t pointsRead;
        uInt32_t bytesToWrite;

        fprintf(stderr, "\015Decompressing %3d%%", (sInt32_t) (totalPointsRead * 100.0 / iInfo.noPoints));
        pointsRead = OptimFROG_read(decoderInstance, buffer, pointsToRead, C_FALSE);
        if (pointsRead <= 0)
        {
            OptimFROG_close(decoderInstance);
            OptimFROG_destroyInstance(decoderInstance);
            free(buffer);
            fclose(fDst);
            return fatalError("could not read source file");
        }
        bytesToWrite = pointsRead * ((iInfo.bitspersample / 8) * iInfo.channels);
        if (fwrite(buffer, 1, bytesToWrite, fDst) != bytesToWrite)
        {
            OptimFROG_close(decoderInstance);
            OptimFROG_destroyInstance(decoderInstance);
            free(buffer);
            fclose(fDst);
            return fatalError("could not write to destination file");
        }
        totalPointsRead += pointsRead;
    }

    tailSize = OptimFROG_readTail(decoderInstance, buffer, READ_BUFFER_SIZE);
    if (tailSize < 0)
    {
        OptimFROG_close(decoderInstance);
        OptimFROG_destroyInstance(decoderInstance);
        free(buffer);
        fclose(fDst);
        return fatalError("could not read source file");
    }
    if (fwrite(buffer, 1, tailSize, fDst) != (uInt32_t) tailSize)
    {
        OptimFROG_close(decoderInstance);
        OptimFROG_destroyInstance(decoderInstance);
        free(buffer);
        fclose(fDst);
        return fatalError("could not write to destination file");
    }

    recErrors = OptimFROG_recoverableErrors(decoderInstance);

    fprintf(stderr, "\015Decompressing done.\n");
    if (recErrors)
        fprintf(stderr, "recoverable errors occured decompressing %s\n", sourceFile);
    printf("\n");

    OptimFROG_close(decoderInstance);
    OptimFROG_destroyInstance(decoderInstance);
    free(buffer);
    if (fclose(fDst) != 0)
        return fatalError("could not write to destination file");

    if (recErrors)
        return 1;
    else
        return 0;
}

sInt32_t infoFile(char* sourceFile)
{
    void* decoderInstance;
    condition_t opened;
    OptimFROG_Info iInfo;
    OptimFROG_Tags iTags;
    uInt32_t i;

    decoderInstance = OptimFROG_createInstance();
    if (decoderInstance == C_NULL)
        return fatalError("could not create instance");

    opened = OptimFROG_open(decoderInstance, sourceFile, C_TRUE);
    if (!opened)
    {
        OptimFROG_destroyInstance(decoderInstance);
        return fatalError("could not open source OFR/OFS file");
    }

    OptimFROG_getInfo(decoderInstance, &iInfo);

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

    OptimFROG_getTags(decoderInstance, &iTags);
    for (i = 0; i < iTags.keyCount; i++)
    {
        printf("%s: %s\n", iTags.keys[i], iTags.values[i]);
    }
    printf("\n");

    OptimFROG_close(decoderInstance);
    OptimFROG_destroyInstance(decoderInstance);
    OptimFROG_freeTags(&iTags);

    return 0;
}

int main(int argc, char* argv[])
{
    fprintf(stderr, "\n");
    fprintf(stderr, "OptimFROG Lossless/DualStream Audio Decompressor [C]\n");
    fprintf(stderr, "Copyright (C) 1996-2006 Florin Ghido, all rights reserved.\n");
    fprintf(stderr, "Visit http://www.LosslessAudio.org for updates\n");
    fprintf(stderr, "Free for non-commercial use. E-mail: FlorinGhido@yahoo.com\n");
    fprintf(stderr, "OptimFROG.dll version: %4u\n", OptimFROG_getVersion());
    fprintf(stderr, "\n");

    if (argc < 3)
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "OFROGDec {d|i} sourceFile [destinationFile]\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Commands:\n");
        fprintf(stderr, "    d            decode an OFR/OFS file to a WAV/RAW file\n");
        fprintf(stderr, "    i            print information about an OFR/OFS file\n");
        return 1;
    }

    if (strcmp(argv[1], "d") == 0)
    {
        char* destinationFile;

        char temp[300];
        if (argc == 3)
        {
            sInt32_t pos;
            strcpy(temp, argv[2]);

            pos = strlen(temp) - 1;
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
