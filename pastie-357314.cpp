// example to encode from .wav -> .mp4 file
//
// WARNING: this code does not do proper free/close of objects in case of
//          many error conditions; ie: it will leak on error.
//
// - uses a private-function from libfaac (input.ic and input.h) to read .wav
// - uses libfaac to encode AAC
// - uses libmp4v2 for .mp4 mux/container output

#include <iostream>
#include <faac.h>
//#include <mp4v2/mp4v2.h>

using namespace std;

#include "input.c"
#define _WAVEFORMATEX_
#include <mp4.h>

static int *mkChanMap(int channels, int center, int lf)
{
    int *map;
    int inpos;
    int outpos;

    if (!center && !lf)
        return NULL;

    if (channels < 3)
        return NULL;

    if (lf > 0)
        lf--;
    else
        lf = channels - 1; // default AAC position

    if (center > 0)
        center--;
    else
        center = 0; // default AAC position

    map = (int *)malloc(channels * sizeof(map[0]));
    memset(map, 0, channels * sizeof(map[0]));

    outpos = 0;
    if ((center >= 0) && (center < channels))
        map[outpos++] = center;

    inpos = 0;
    for (; outpos < (channels - 1); inpos++)
    {
        if (inpos == center)
            continue;
        if (inpos == lf)
            continue;

        map[outpos++] = inpos;
    }
    if (outpos < channels)
    {
        if ((lf >= 0) && (lf < channels))
            map[outpos] = lf;
        else
            map[outpos] = inpos;
    }

    return map;
}

///////////////////////////////////////////////////////////////////////////////

void
doit( const string& file_wav )
{
    string file_mp4 = file_wav;
    file_mp4.resize( file_mp4.find( ".wav" ));
    file_mp4.append( ".mp4" );

    unsigned long faacInputSamples;
    unsigned long faacMaxOutputBytes;

    faacEncHandle faac = faacEncOpen( 44100, 2, &faacInputSamples, &faacMaxOutputBytes );
    if( !faac ) {
        cout << "faacEncOpen failed" << endl;
        return;
    }

    cout << "faacInputSamples   = " << faacInputSamples << endl;
    cout << "faacMaxOutputBytes = " << faacMaxOutputBytes << endl;

    faacEncConfigurationPtr faacConfig = faacEncGetCurrentConfiguration( faac );

    faacConfig->mpegVersion   = MPEG4;
    faacConfig->aacObjectType = LOW;
//    faacConfig->aacObjectType = MAIN;
    faacConfig->allowMidside  = 1;
    faacConfig->useLfe        = 0;
    faacConfig->useTns        = 0;
    faacConfig->bitRate       = 64000; // per channel
    faacConfig->quantqual     = 100;
    faacConfig->outputFormat  = 0;
    faacConfig->inputFormat   = FAAC_INPUT_FLOAT;
//    faacConfig->bandWidth     = 16000;
    faacConfig->bandWidth     = 0;

    if( !faacEncSetConfiguration( faac, faacConfig )) {
        cout << "faacEncGetCurrentConfiguration failed" << endl;
        return;
    }

    MP4FileHandle mp4 = MP4Create( file_mp4.c_str() );
    if( mp4 == MP4_INVALID_FILE_HANDLE ) {
        cout << "MP4Create failed" << endl;
        return;
    }

    MP4SetTimeScale( mp4, 44100 );

    //MP4TrackId track = MP4AddAudioTrack( mp4, 44100, MP4_INVALID_DURATION );
    MP4TrackId track = MP4AddAudioTrack( mp4, 44100, 1024 );
    if( track == MP4_INVALID_TRACK_ID ) {
        cout << "MP4AddAudioTrack failed" << endl;
        return;
    }

    MP4SetAudioProfileLevel( mp4, 0x0f );

    unsigned char* faacDecoderInfo;
    unsigned long  faacDecoderInfoSize;
    if( faacEncGetDecoderSpecificInfo( faac, &faacDecoderInfo, &faacDecoderInfoSize )) {
        cout << "faacEncGetDecoderSpecificInfo failed" << endl;
        return;
    }

    MP4SetTrackESConfiguration( mp4, track, faacDecoderInfo, faacDecoderInfoSize );
    free( faacDecoderInfo );

    pcmfile_t* infile = wav_open_read( file_wav.c_str(), 0 );
    if( !infile ) {
        cout << "wav_open_read failed" << endl;
        return;
    }

    float* pcmbuf = (float*)malloc( faacInputSamples * sizeof(float) );
    int* chanmap = mkChanMap( infile->channels, 3, 4 );
    unsigned char* aacbuf = (unsigned char*)malloc( faacMaxOutputBytes );

    size_t tread = 0;
    uint64_t tsamples = 0;
    uint64_t tenc = 0;

    for( ;; ) {
        size_t nread = wav_read_float32( infile, pcmbuf, faacInputSamples, chanmap );
        tread += nread;
        tsamples += nread / infile->channels;

        int nenc = faacEncEncode( faac, (int32_t*)pcmbuf, nread, aacbuf, faacMaxOutputBytes );
        if( nenc < 0 ) {
            cout << "faacEncEncode failed" << endl;
            break;
        }

        if( !nread && !nenc )
            break;

        tenc += nenc;

cout << "tread=" << tread
     << "  nread=" << nread
     << "  tenc=" << tenc
     << "  nenc=" << nenc
     << endl;

        if( nenc )
            MP4WriteSample( mp4, track, aacbuf, nenc );
    }

    free( aacbuf );
    free( pcmbuf );
    wav_close( infile );
    faacEncClose( faac );
    MP4Close( mp4 );
}

///////////////////////////////////////////////////////////////////////////////

int
main( int argc, const char* argv[] )
{
    if( argc < 2 ) {
        cout << "usage: " << argv[0] << " file.wav" << endl;
        return 1;
    }

    doit( argv[1] );
    return 0;
}
