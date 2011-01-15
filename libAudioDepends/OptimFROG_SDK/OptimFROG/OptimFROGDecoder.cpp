// Copyright (C) 1996-2005 Florin Ghido, all rights reserved.


// OptimFROGDecoder.cpp contains a thin C++ wrapper over the
// C interface contained in OptimFROG.h for OptimFROG.dll/.so
// which works with OptimFROG Lossless/DualStream files

// Version: 1.200, Date: 2005.07.17


#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "OptimFROGDecoder.h"


#if defined(CFG_MSCVER_WIN32_MIX86) || defined(CFG_BORLANDC_WIN32_MIX86)
#define strcasecmp stricmp
#else
#include <strings.h>
// for strcasecmp, POSIX compliant
#endif


OptimFROGDecoder::OptimFROGDecoder()
{
    init();
}

OptimFROGDecoder::~OptimFROGDecoder()
{
    freeFileDetails();
    OptimFROG_destroyInstance(decoderInstance);
    decoderInstance = C_NULL;
    isOpened = C_FALSE;
}

condition_t OptimFROGDecoder::openExt(ReadInterface* rInt, void* readerInstance, condition_t readTags, condition_t max16bit)
{
    if (decoderInstance == C_NULL)
    {
        decoderInstance = OptimFROG_createInstance();
        if (decoderInstance == C_NULL)
        {
            return C_FALSE;
        }
    }

    isOpened = OptimFROG_openExt(decoderInstance, rInt, readerInstance, readTags);
    if (!isOpened)
    {
        return C_FALSE;
    }
    isMax16bit = max16bit;

    OptimFROG_Info tInfo;
    OptimFROG_getInfo(decoderInstance, &tInfo);
    copyInfo(tInfo);
    if (readTags)
    {
        OptimFROG_Tags tTags;
        OptimFROG_getTags(decoderInstance, &tTags);
        copyTags(tTags);
        OptimFROG_freeTags(&tTags);
    }

    return isOpened;
}

condition_t OptimFROGDecoder::open(char* fileName, condition_t readTags, condition_t max16bit)
{
    if (decoderInstance == C_NULL)
    {
        decoderInstance = OptimFROG_createInstance();
        if (decoderInstance == C_NULL)
        {
            return C_FALSE;
        }
    }

    isOpened = OptimFROG_open(decoderInstance, fileName, readTags);
    if (!isOpened)
    {
        return C_FALSE;
    }
    isMax16bit = max16bit;

    OptimFROG_Info tInfo;
    OptimFROG_getInfo(decoderInstance, &tInfo);
    copyInfo(tInfo);
    if (readTags)
    {
        OptimFROG_Tags tTags;
        OptimFROG_getTags(decoderInstance, &tTags);
        copyTags(tTags);
        OptimFROG_freeTags(&tTags);
    }

    return isOpened;
}

condition_t OptimFROGDecoder::close()
{
    freeFileDetails();
    condition_t result = OptimFROG_close(decoderInstance);
    isOpened = C_FALSE;
    return result;
}

sInt32_t OptimFROGDecoder::readHead(void* headData, uInt32_t maxSize)
{
    assert(isOpened);

    return OptimFROG_readHead(decoderInstance, headData, maxSize);
}

sInt32_t OptimFROGDecoder::readTail(void* tailData, uInt32_t maxSize)
{
    assert(isOpened);

    return OptimFROG_readTail(decoderInstance, tailData, maxSize);
}

condition_t OptimFROGDecoder::seekable()
{
    assert(isOpened);

    return OptimFROG_seekable(decoderInstance);
}

condition_t OptimFROGDecoder::seekPointPos(sInt64_t pointPos)
{
    assert(isOpened);

    return OptimFROG_seekPoint(decoderInstance, pointPos);
}

condition_t OptimFROGDecoder::seekTimeMillis(sInt64_t milliseconds)
{
    assert(isOpened);

    return OptimFROG_seekTime(decoderInstance, milliseconds);
}

condition_t OptimFROGDecoder::seekTimeDouble(Float64_t time)
{
    assert(isOpened);

    sInt64_t pointPos = (sInt64_t) (time * samplerate);
    return OptimFROG_seekPoint(decoderInstance, pointPos);
}

condition_t OptimFROGDecoder::seekBytePos(sInt64_t bytePos)
{
    assert(isOpened);

    sInt64_t pointPos = bytePos / ((bitspersample / 8) * channels);
    if (isMax16bit && (bitspersample > 16))
    {
        pointPos = bytePos / ((16 / 8) * channels);
    }

    return OptimFROG_seekPoint(decoderInstance, pointPos);
}

sInt64_t OptimFROGDecoder::getPointPos()
{
    assert(isOpened);

    return OptimFROG_getPos(decoderInstance);
}

sInt64_t OptimFROGDecoder::getTimeMillis()
{
    assert(isOpened);

    sInt64_t pointPos = OptimFROG_getPos(decoderInstance);
    return (sInt64_t) (((Float64_t) pointPos / samplerate) * 1000.0);
}

Float64_t OptimFROGDecoder::getTimeDouble()
{
    assert(isOpened);

    sInt64_t pointPos = OptimFROG_getPos(decoderInstance);
    return (Float64_t) pointPos / samplerate;
}

sInt64_t OptimFROGDecoder::getBytePos()
{
    assert(isOpened);

    sInt64_t pointPos = OptimFROG_getPos(decoderInstance);
    if (isMax16bit && (bitspersample > 16))
    {
        return pointPos * ((16 / 8) * channels);
    }
    else
    {
        return pointPos * ((bitspersample / 8) * channels);
    }
}

condition_t OptimFROGDecoder::recoverableErrors()
{
    assert(isOpened);

    return OptimFROG_recoverableErrors(decoderInstance);
}

sInt32_t OptimFROGDecoder::readPoints(void* data, uInt32_t noPoints)
{
    assert(isOpened);

    return OptimFROG_read(decoderInstance, data, noPoints, isMax16bit);
}

sInt32_t OptimFROGDecoder::readBytes(void* data, uInt32_t noBytes)
{
    assert(isOpened);

    uInt32_t noPoints = noBytes / ((bitspersample / 8) * channels);
    if (isMax16bit && (bitspersample > 16))
    {
        noPoints = noBytes / ((16 / 8) * channels);
    }

    sInt32_t pointsRead = OptimFROG_read(decoderInstance, data, noPoints, isMax16bit);
    if (pointsRead < 0)
    {
        return pointsRead;
    }

    if (isMax16bit && (bitspersample > 16))
    {
        return pointsRead * ((16 / 8) * channels);
    }
    else
    {
        return pointsRead * ((bitspersample / 8) * channels);
    }
}

uInt32_t OptimFROGDecoder::getVersion()
{
    return OptimFROG_getVersion();
}

sInt32_t OptimFROGDecoder::decodeFile(char* sourceFile, char* destinationFile, OptimFROG_callBack callBack, void* callBackParam)
{
    return OptimFROG_decodeFile(sourceFile, destinationFile, callBack, callBackParam);
}

sInt32_t OptimFROGDecoder::getFileDetails(char* sourceFile, condition_t readTags)
{
    sInt32_t result = OptimFROG_NoError;
    init();

    if (readTags)
    {
        OptimFROG_Info tInfo;
        OptimFROG_Tags tTags;
        result = OptimFROG_infoFile(sourceFile, &tInfo, &tTags);
        if (result == OptimFROG_NoError)
        {
            copyInfo(tInfo);
            copyTags(tTags);
            OptimFROG_freeTags(&tTags);
        }
    }
    else
    {
        OptimFROG_Info tInfo;
        result = OptimFROG_infoFile(sourceFile, &tInfo, C_NULL);
        if (result == OptimFROG_NoError)
        {
            copyInfo(tInfo);
        }
    }

    return result;
}

void OptimFROGDecoder::freeFileDetails()
{
    for (uInt32_t i = 0; i < keyCount; ++i)
    {
        free(keys[i]);
        keys[i] = C_NULL;
        free(values[i]);
        values[i] = C_NULL;
    }
    keyCount = 0;
}

const char* OptimFROGDecoder::findTag(const char* key)
{
    for (uInt32_t i = 0; i < keyCount; ++i)
    {
        if (strcasecmp(key, keys[i]) == 0)
        {
            return values[i];
        }
    }

    return C_NULL;
}

void OptimFROGDecoder::copyInfo(OptimFROG_Info& info)
{
    channels = info.channels;
    samplerate = info.samplerate;
    bitspersample = info.bitspersample;
    bitrate = info.bitrate;

    version = info.version;
    method = info.method;
    speedup = info.speedup;

    noPoints = info.noPoints;
    originalSize = info.originalSize;
    compressedSize = info.compressedSize;
    length_ms = info.length_ms;

    sampleType = info.sampleType;
    channelConfig = info.channelConfig;
}

void OptimFROGDecoder::copyTags(OptimFROG_Tags& tags)
{
    keyCount = tags.keyCount;
    for (uInt32_t i = 0; i < keyCount; ++i)
    {
        keys[i] = strdup(tags.keys[i]);
        values[i] = strdup(tags.values[i]);
    }
}

void OptimFROGDecoder::init()
{
    // OptimFROG_Info
    channels = 0;
    samplerate = 0;
    bitspersample = 0;
    bitrate = 0;

    version = 0;
    method = C_NULL;
    speedup = C_NULL;

    noPoints = 0;
    originalSize = 0;
    compressedSize = 0;
    length_ms = 0;

    sampleType = C_NULL;
    channelConfig = C_NULL;

    // OptimFROG_Tags
    keyCount = 0;
    for (uInt32_t i = 0; i < 64; ++i)
    {
        keys[i] = C_NULL;
        values[i] = C_NULL;
    }

    // miscellaneous info
    isOpened = C_FALSE;
    isMax16bit = C_FALSE;

    // OptimFROG decoder handle
    decoderInstance = C_NULL;
}
