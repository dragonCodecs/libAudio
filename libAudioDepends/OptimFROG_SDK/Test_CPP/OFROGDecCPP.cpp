// Copyright (C) 1996-2006 Florin Ghido, all rights reserved.


// OFROGDec.cpp contains an example usage for OptimFROG.dll/.so
// which works with OptimFROG Lossless/DualStream files

// Version: 1.210, Date: 2006.03.02, using the C++ interface


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "OptimFROGDecoder.h"


sInt32_t fatalError(char* message)
{
    fprintf(stderr, "\nFatal error: %s!\n\n", message);
    return 1;
}

#define READ_BUFFER_SIZE 176400
sInt32_t decodeFile(char* sourceFile, char* destinationFile)
{
    printf("\n");
    printf("sourceFile:      %s\n", sourceFile);
    printf("destinationFile: %s\n", destinationFile);
    printf("\n");

    uInt8_t* buffer = C_NULL;
    try { buffer = new uInt8_t[READ_BUFFER_SIZE]; } catch(...) {}
    if (buffer == C_NULL)
    {
        return fatalError("could not allocate decode buffer");
    }

    OptimFROGDecoder ofr_dec;

    condition_t opened = ofr_dec.open(sourceFile);
    if (!opened)
    {
        delete[] buffer;
        return fatalError("could not open source OFR/OFS file");
    }

    FILE* fDst = fopen(destinationFile, "wb");
    if (fDst == C_NULL)
    {
        ofr_dec.close();
        delete[] buffer;
        return fatalError("could not open destination file");
    }

    sInt32_t headSize = ofr_dec.readHead(buffer, READ_BUFFER_SIZE);
    if (fwrite(buffer, 1, headSize, fDst) != (uInt32_t) headSize)
    {
        ofr_dec.close();
        delete[] buffer;
        fclose(fDst);
        return fatalError("could not write to destination file");
    }

    uInt32_t pointsToRead = READ_BUFFER_SIZE / ((ofr_dec.bitspersample / 8) * ofr_dec.channels);
    sInt64_t totalPointsRead = 0;
    while (totalPointsRead < ofr_dec.noPoints)
    {
        fprintf(stderr, "\015Decompressing %3d%%", (sInt32_t) (totalPointsRead * 100.0 / ofr_dec.noPoints));
        sInt32_t pointsRead = ofr_dec.readPoints(buffer, pointsToRead);
        if (pointsRead <= 0)
        {
            ofr_dec.close();
            delete[] buffer;
            fclose(fDst);
            return fatalError("could not read source file");
        }
        uInt32_t bytesToWrite = pointsRead * ((ofr_dec.bitspersample / 8) * ofr_dec.channels);
        if (fwrite(buffer, 1, bytesToWrite, fDst) != bytesToWrite)
        {
            ofr_dec.close();
            delete[] buffer;
            fclose(fDst);
            return fatalError("could not write to destination file");
        }
        totalPointsRead += pointsRead;
    }

    sInt32_t tailSize = ofr_dec.readTail(buffer, READ_BUFFER_SIZE);
    if (tailSize < 0)
    {
        ofr_dec.close();
        delete[] buffer;
        fclose(fDst);
        return fatalError("could not read source file");
    }
    if (fwrite(buffer, 1, tailSize, fDst) != (uInt32_t) tailSize)
    {
        ofr_dec.close();
        delete[] buffer;
        fclose(fDst);
        return fatalError("could not write to destination file");
    }

    condition_t recErrors = ofr_dec.recoverableErrors();

    fprintf(stderr, "\015Decompressing done.\n");
    if (recErrors)
        fprintf(stderr, "recoverable errors occured decompressing %s\n", sourceFile);
    printf("\n");

    ofr_dec.close();
    delete[] buffer;
    if (fclose(fDst) != 0)
        return fatalError("could not write to destination file");

    if (recErrors)
        return 1;
    else
        return 0;
}

sInt32_t infoFile(char* sourceFile)
{
    OptimFROGDecoder ofr_dec;

    condition_t opened = ofr_dec.open(sourceFile, C_TRUE);
    if (!opened)
    {
        return fatalError("could not open source OFR/OFS file");
    }

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

    ofr_dec.close();

    return 0;
}

int main(int argc, char* argv[])
{
    fprintf(stderr, "\n");
    fprintf(stderr, "OptimFROG Lossless/DualStream Audio Decompressor [C++]\n");
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
