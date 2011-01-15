// xmms_ofr.cpp contains an OptimFROG Lossless/DualStream audio
// input plug-in for XMMS playback

// Copyright (C) 2005 Florin Ghido
// Version: 1.10, Date: 2005.07.17


#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <glib.h>
#include <pthread.h>
#include "xmms/plugin.h"
#include "xmms/util.h"
#include "OptimFROGDecoder.h"


extern InputPlugin ofr_ip;

static void ofr_init(void);
static int ofr_is_our_file(char* filename);
static void ofr_play_file(char* filename);
static void ofr_stop(void);
static void ofr_seek(int time);
static void ofr_pause(short p);
static int ofr_get_time(void);
static void ofr_get_song_info(char* filename, char** title, int* length);
static void ofr_about();
static void ofr_config();
static void ofr_file_info_box(char* filename);

static sInt32_t seek_to = -1;
static sInt32_t going = 0;
static sInt32_t eof = 0;
static sInt32_t position = 0;
static OptimFROGDecoder* ofrd = C_NULL;
static AFormat playFMT = FMT_S16_LE;

static pthread_t decode_thread;
static gboolean audio_error = FALSE;


InputPlugin ofr_ip =
{
    C_NULL,
    C_NULL,
    C_NULL,
    ofr_init,
    ofr_about,
    ofr_config,
    ofr_is_our_file,
    C_NULL,
    ofr_play_file,
    ofr_stop,
    ofr_pause,
    ofr_seek,
    C_NULL,
    ofr_get_time,
    C_NULL,
    C_NULL,
    C_NULL,
    C_NULL,
    C_NULL,
    C_NULL,
    C_NULL,
    ofr_get_song_info,
    ofr_file_info_box,
    C_NULL
};


extern "C" InputPlugin* get_iplugin_info(void)
{
    ofr_ip.description = strdup("OptimFROG Lossless/DualStream Decoder v1.10");
    return &ofr_ip;
}

static void ofr_init(void)
{
    ofrd = new OptimFROGDecoder();
}

static int ofr_is_our_file(char* filename)
{
    char* ext = strrchr(filename, '.');
    if ((ext != C_NULL) && ((strcasecmp(ext, ".ofr") == 0) || (strcasecmp(ext, ".ofs") == 0)))
        return TRUE;
    return FALSE;
}

static gchar* get_title(gchar* filename)
{
    char title[300];

    char* p = strrchr(filename, '/');
    if (p == C_NULL)
        p = filename;
    else
        p++;
    strcpy(title, p);
    p = strrchr(title, '.');
    if ((p != C_NULL) && ((strcasecmp(p, ".ofr") == 0) || (strcasecmp(p, ".ofs") == 0)))
        *p = 0;

    OptimFROGDecoder tmp;
    if (tmp.getFileDetails(filename, C_TRUE) != OptimFROG_NoError)
    {
        return strdup(title);
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

    return strdup(title);
}

static void* play_loop(void*)
{
    char data[512 * 2 * (32 / 8) * 2 * 2];
    int bytes;
    while (going)
    {
        if (eof && (seek_to == -1))
        {
            if (ofr_ip.output->buffer_playing())
            {
                xmms_usleep(10000);
                continue;
            }
            else
                break;
        }

        if (seek_to != -1)
        {
            position = seek_to * 1000;
            ofr_ip.output->flush(seek_to * 1000);
            if (!ofrd->seekTimeMillis(seek_to * 1000))
            {
                position = -1;
                audio_error = TRUE;
                eof = 1;
            }
            seek_to = -1;
        }

        sInt32_t points_read = ofrd->readPoints((sInt16_t*) data, 512);
        if (points_read <= 0)
        {
            eof = 1;
            ofr_ip.output->buffer_free();
            ofr_ip.output->buffer_free();
            xmms_usleep(10000);
        }
        else
        {
            sInt32_t samples = points_read * ofrd->channels;
            bytes = samples * ((ofrd->bitspersample <= 16 ? ofrd->bitspersample : 16) / 8); // max16bit
            ofr_ip.add_vis_pcm(ofr_ip.output->written_time(), playFMT, ofrd->channels, bytes, data);
            while ((ofr_ip.output->buffer_free() < bytes) && going && (seek_to == -1))
                xmms_usleep(10000);
            if (going && (seek_to == -1))
                ofr_ip.output->write_audio(data, bytes);
            position += (points_read * 1000) / ofrd->samplerate;
        }
    }
    pthread_exit(C_NULL);
}

static void ofr_play_file(char* filename)
{
    char* name;
    audio_error = FALSE;
    eof = 0;
    if (!ofrd->open(filename, C_FALSE, C_TRUE)) // max16bit
        return;
    position = 0;
    going = 1;

    if (ofrd->bitspersample == 8)
        playFMT = FMT_U8;
    else
        playFMT = FMT_S16_LE; // max16bit

    if (ofr_ip.output->open_audio(playFMT, ofrd->samplerate, ofrd->channels) == 0)
    {
        audio_error = TRUE;
        ofrd->close();
        return;
    }
    name = get_title(filename);
    ofr_ip.set_info(name, (int) ofrd->length_ms, ofrd->bitrate * 1000, ofrd->samplerate, ofrd->channels);
    seek_to = -1;
    pthread_create(&decode_thread, C_NULL, play_loop, C_NULL);
}

static void ofr_stop(void)
{
    if (going)
    {
        going = 0;
        pthread_join(decode_thread, C_NULL);
        ofr_ip.output->close_audio();
        ofrd->close();
    }
}

static void ofr_pause(short p)
{
    ofr_ip.output->pause(p);
}

static void ofr_seek(int time)
{
    seek_to = time;
    eof = FALSE;
    while (seek_to != -1)
        xmms_usleep(10000);
}

static int ofr_get_time(void)
{
    if (audio_error | !going || (eof && !ofr_ip.output->buffer_playing()))
        return -1;
    else
        return ofr_ip.output->output_time();
}

static void ofr_get_song_info(char* filename, char** title, int* length)
{
    *length = 0;
    OptimFROGDecoder tmp;
    if (tmp.getFileDetails(filename) != OptimFROG_NoError)
    {
        return;
    }

    *length = (int) tmp.length_ms;
    *title = get_title(filename);
}

static void ofr_about()
{
    char buffer[1024];
    uInt32_t version = OptimFROGDecoder::getVersion();
    sprintf(buffer,
            "XMMS OptimFROG input plug-in, version 1.10 [2005.07.17]\n"
            "OptimFROG Lossless/DualStream audio SO library, version %u.%03u\n"
            "Copyright (C) 1996-2005 Florin Ghido, all rights reserved.\n"
            "Visit http://www.LosslessAudio.org for updates\n"
            "Free for non-commercial use. E-mail: FlorinGhido@yahoo.com",
            version / 1000, version % 1000);

    xmms_show_message("OptimFROG Lossless/DualStream Decoder", buffer, "Ok", FALSE, C_NULL, C_NULL);
}

static void ofr_config()
{
    xmms_show_message("Configuration", "No configuration available.", "Ok", FALSE, C_NULL, C_NULL);
}

void ofr_file_info_box(char* fn)
{
    OptimFROGDecoder tmp;
    if (tmp.getFileDetails(fn, C_TRUE) != OptimFROG_NoError)
    {
        xmms_show_message("OptimFROG File Information", "Not a valid OptimFROG 4.5xx or 4.2x file",
            "Ok", FALSE, C_NULL, C_NULL);
        return;
    }

    char msg[2048];
    int msgLen = sprintf(msg,
                         "File name: %s\n"
                         "Original file size: %10u        \n"
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
    xmms_show_message("OptimFROG File Information", msg, "Ok", FALSE, C_NULL, C_NULL);
}
