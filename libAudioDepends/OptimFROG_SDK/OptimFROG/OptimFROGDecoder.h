// Copyright (C) 1996-2005 Florin Ghido, all rights reserved.


// OptimFROGDecoder.h contains a thin C++ wrapper over the
// C interface contained in OptimFROG.h for OptimFROG.dll/.so
// which works with OptimFROG Lossless/DualStream files

// Version: 1.200, Date: 2005.07.17


#ifndef OPTIMFROGDECODER_H_INCLUDED
#define OPTIMFROGDECODER_H_INCLUDED


#include "OptimFROG.h"


class OptimFROGDecoder
{
public:
    OptimFROGDecoder();
    ~OptimFROGDecoder();

    condition_t openExt(ReadInterface* rInt, void* readerInstance, condition_t readTags = C_FALSE, condition_t max16bit = C_FALSE);

    condition_t open(char* fileName, condition_t readTags = C_FALSE, condition_t max16bit = C_FALSE);
    condition_t close();

    sInt32_t readHead(void* headData, uInt32_t maxSize);
    sInt32_t readTail(void* tailData, uInt32_t maxSize);

    condition_t seekable();
    condition_t seekPointPos(sInt64_t pointPos);
    condition_t seekTimeMillis(sInt64_t milliseconds);
    condition_t seekTimeDouble(Float64_t seconds);
    condition_t seekBytePos(sInt64_t bytePos);

    sInt64_t getPointPos();
    sInt64_t getTimeMillis();
    Float64_t getTimeDouble();
    sInt64_t getBytePos();

    condition_t recoverableErrors();

    sInt32_t readPoints(void* data, uInt32_t noPoints);
    sInt32_t readBytes(void* data, uInt32_t noBytes);

    static uInt32_t getVersion();
    static sInt32_t decodeFile(char* sourceFile, char* destinationFile, OptimFROG_callBack callBack = C_NULL, void* callBackParam = C_NULL);
    sInt32_t getFileDetails(char* sourceFile, condition_t readTags = C_FALSE);
    void freeFileDetails();
    const char* findTag(const char* key);


    // OptimFROG_Info
    uInt32_t channels;
    uInt32_t samplerate;
    uInt32_t bitspersample;
    uInt32_t bitrate;

    uInt32_t version;
    char* method;
    char* speedup;

    sInt64_t noPoints;
    sInt64_t originalSize;
    sInt64_t compressedSize;
    sInt64_t length_ms;

    char* sampleType;
    char* channelConfig;

    // OptimFROG_Tags
    uInt32_t keyCount;
    char* keys[64];
    char* values[64];

    // miscellaneous info
    condition_t isOpened;
    condition_t isMax16bit;

private:
    void init();
    void copyInfo(OptimFROG_Info& info);
    void copyTags(OptimFROG_Tags& tags);

    // OptimFROG decoder handle
    void* decoderInstance;
};


#endif
