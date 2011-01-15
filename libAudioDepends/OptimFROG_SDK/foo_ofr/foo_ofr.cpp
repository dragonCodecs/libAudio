// foo_ofr.cpp contains an OptimFROG Lossless/DualStream audio
// input plug-in for foobar2000 playback

// Copyright (C) 2005 Florin Ghido
// Version: 1.10, Date: 2005.07.17


#include "../SDK/foobar2000.h"
#include "OptimFROG.h"


// read interface functions begin

#define getReader(variable) ((reader*) variable)

condition_t f_foobar2000_close(void* instance)
{
    return C_TRUE;
}

sInt32_t f_foobar2000_read(void* instance, void* destBuffer, uInt32_t count)
{
    return getReader(instance)->read(destBuffer, count);
}

condition_t f_foobar2000_eof(void* instance)
{
    return C_FALSE; // not used internally
}

condition_t f_foobar2000_seekable(void* instance)
{
    return getReader(instance)->can_seek();
}

sInt64_t f_foobar2000_length(void* instance)
{
    return getReader(instance)->get_length(); // -1 when unknown
}

sInt64_t f_foobar2000_getPos(void* instance)
{
    return getReader(instance)->get_position(); // -1 when unknown
}

condition_t f_foobar2000_seek(void* instance, sInt64_t pos)
{
    return getReader(instance)->seek(pos);
}

static ReadInterface rInt_foobar2000 =
{
    f_foobar2000_close,
    f_foobar2000_read,
    f_foobar2000_eof,
    f_foobar2000_seekable,
    f_foobar2000_length,
    f_foobar2000_getPos,
    f_foobar2000_seek
};

// read interface functions end


class input_ofr : public input_pcm
{
private:
    mem_block_t<char> buffer;

    void* decoderInstance;
    OptimFROG_Info iInfo;

    enum { DELTA_SAMPLES = 1024 };

public:

    virtual bool test_filename(const char* fn, const char* ext)
    {
        return ((stricmp_utf8(ext, "OFR") == 0) || (stricmp_utf8(ext, "OFS") == 0));
    }

    input_ofr()
    {
        decoderInstance = C_NULL;
    }

    ~input_ofr()
    {
        if (decoderInstance != C_NULL)
            OptimFROG_destroyInstance(decoderInstance);
        decoderInstance = C_NULL;
    }

    bool open(reader* r, file_info* info, unsigned flags)
    {
        decoderInstance = OptimFROG_createInstance();
        if (decoderInstance == C_NULL)
            return 0;
        if (!OptimFROG_openExt(decoderInstance, &rInt_foobar2000, r, C_FALSE))
        {
            OptimFROG_destroyInstance(decoderInstance);
            decoderInstance = C_NULL;
            return 0;
        }

        OptimFROG_getInfo(decoderInstance, &iInfo);

        if (flags & OPEN_FLAG_GET_INFO)
        {
            tag_reader::g_run_multi(r, info, "ape|id3v1");

            info->set_length((Float64_t) iInfo.noPoints / iInfo.samplerate);
            info->info_set_int("bitrate", iInfo.bitrate);
            info->info_set_int("samplerate", iInfo.samplerate);
            info->info_set_int("channels", iInfo.channels);
            info->info_set_int("bitspersample", iInfo.bitspersample);

            const char* fileName = info->get_file_path();
            if ((fileName != C_NULL) && (stricmp_utf8(fileName + strlen(fileName) - 3, "OFR") == 0))
                info->info_set("codec", "OptimFROG");
            else
                info->info_set("codec", "DualStream");

            info->info_set("mode", iInfo.method);
            info->info_set("speedup", iInfo.speedup);
            info->info_set_int("version", iInfo.version);
        }

        return 1;
    }

    virtual set_info_t set_info(reader* r, const file_info* info)
    {
        tag_remover::g_run(r);
        return tag_writer::g_run(r, info, "ape") ? SET_INFO_SUCCESS : SET_INFO_FAILURE;
    }

    virtual int get_samples_pcm(void** out_buffer, int* out_size, int* srate, int* bps, int* nch)
    {
        char* ptr = buffer.check_size(DELTA_SAMPLES * iInfo.channels * (iInfo.bitspersample / 8));
        sInt32_t pointsRetrieved = OptimFROG_read(decoderInstance, ptr, DELTA_SAMPLES);
        if (pointsRetrieved <= 0)
            return 0;

        *out_buffer = ptr;
        *out_size = pointsRetrieved * iInfo.channels * (iInfo.bitspersample / 8);
        *srate = iInfo.samplerate;
        *bps = iInfo.bitspersample;
        *nch = iInfo.channels;
        return 1;
    }

    virtual bool seek(double seconds)
    {
        return (OptimFROG_seekPoint(decoderInstance, (sInt64_t) (seconds * iInfo.samplerate)) == C_TRUE);
    }

    virtual bool can_seek()
    {
        return (OptimFROG_seekable(decoderInstance) == C_TRUE);
    }
};

static service_factory_t<input, input_ofr> foo;

const char* ofr_get_about_message()
{
    static char buffer[1024];
    uInt32_t version = OptimFROG_getVersion();
    sprintf(buffer,
            "foobar2000 OptimFROG input plug-in, version 1.10 [2005.07.17]\n"
            "OptimFROG Lossless/DualStream audio DLL library, version %u.%03u\n"
            "Copyright (C) 1996-2005 Florin Ghido, all rights reserved.\n"
            "Visit http://www.LosslessAudio.org for updates\n"
            "Free for non-commercial use. E-mail: FlorinGhido@yahoo.com",
             version / 1000, version % 1000);
    return buffer;
}

DECLARE_COMPONENT_VERSION("OptimFROG Lossless/DualStream Decoder", "1.10", ofr_get_about_message());

DECLARE_FILE_TYPE("OptimFROG files", "*.ofr;*.ofs");
