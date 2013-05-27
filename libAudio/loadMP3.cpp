#include <stdio.h>
#include <malloc.h>

#include <mad.h>
#include <id3tag.h>

#include "libAudio.h"
#include "libAudio_Common.h"

#include <limits.h>

/*!
 * @internal
 * @file loadMP3.cpp
 * @brief The implementation of the MP3 decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2013
 */

#ifndef SHRT_MAX
/*!
 * @internal
 * Backup definition of \c SHRT_MAX, which should be defined in limit.h,
 * but which assumes that short's length is 16 bits
 */
#define SHRT_MAX 0x7FFF
#endif

/*!
 * @internal
 * Internal structure for holding the decoding context for a given MP3 file
 */
typedef struct _MP3_Intern
{
	/*!
	 * @internal
	 * The MP3 file to decode
	 */
	FILE *f_MP3;
	/*!
	 * @internal
	 * The MP3 stream handle
	 */
	mad_stream *p_Stream;
	/*!
	 * @internal
	 * The MP3 frame handle
	 */
	mad_frame *p_Frame;
	/*!
	 * @internal
	 * The MP3 audio synthesis handle
	 */
	mad_synth *p_Synth;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t buffer[8192];
	/*!
	 * @internal
	 * The internal input data buffer
	 */
	uint8_t inbuff[16392];
	/*!
	 * @internal
	 * The \c FileInfo for the MP3 file being decoded
	 */
	FileInfo *p_FI;
	/*!
	 * @internal
	 * A flag indicating if we have yet to decode this MP3's initial frame
	 */
	bool InitialFrame;
	/*!
	 * @internal
	 * A count giving the amount of decoded PCM which has been used with
	 * the current values stored in buffer
	 */
	int PCMDecoded;
	/*!
	 * @internal
	 * A string holding the name of the MP3 file relative to it's opening path
	 * which is used when reading the ID3 data
	 */
	char *f_name;
	/*!
	 * @internal
	 * The playback class instance for the MP3 file
	 */
	Playback *p_Playback;
	/*!
	 * @internal
	 * The end-of-file flag
	 */
	bool eof;
} MP3_Intern;

/*!
 * @internal
 * This function applies a simple conversion algorithm to convert the input
 * fixed-point MAD sample to a short for playback
 * @param Fixed The fixed point sample to convert
 * @return The converted fixed point sample
 * @bug This function applies no noise shaping or dithering
 *   So the output is sub-par to what it could be. FIXME!
 */
inline short FixedToShort(mad_fixed_t Fixed)
{
	if (Fixed >= MAD_F_ONE)
		return SHRT_MAX;
	if (Fixed <= -MAD_F_ONE)
		return -SHRT_MAX;

	Fixed = Fixed >> (MAD_F_FRACBITS - 15);
	return (signed short)Fixed;
}

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by MP3_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *MP3_OpenR(const char *FileName)
{
	MP3_Intern *ret = NULL;
	FILE *f_MP3 = NULL;

	f_MP3 = fopen(FileName, "rb");
	if (f_MP3 == NULL)
		return ret;

	ret = (MP3_Intern *)malloc(sizeof(MP3_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(MP3_Intern));

	ret->f_MP3 = f_MP3;
	ret->f_name = strdup(FileName);
	ret->p_Stream = (mad_stream *)malloc(sizeof(mad_stream));
	ret->p_Frame = (mad_frame *)malloc(sizeof(mad_frame));
	ret->p_Synth = (mad_synth *)malloc(sizeof(mad_synth));
	//ret->p_Timer = (mad_timer_t *)malloc(sizeof(mad_timer_t));

	mad_stream_init(ret->p_Stream);
	mad_frame_init(ret->p_Frame);
	mad_synth_init(ret->p_Synth);
	//mad_timer_reset(ret->p_Timer);

	return ret;
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_MP3File A pointer to a file opened with \c MP3_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c MP3_Play() or \c MP3_FillBuffer()
 * @bug \p p_MP3File must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *MP3_GetFileInfo(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	FileInfo *ret = NULL;
	int rem = 0;
	id3_file *f_id3 = NULL;
	id3_tag *id_tag = NULL;
	id3_frame *id_frame = NULL;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	f_id3 = id3_file_open(/*fileno(p_MF->f_MP3)*/p_MF->f_name, ID3_FILE_MODE_READONLY);
	id_tag = id3_file_tag(f_id3);
	id_frame = id3_tag_findframe(id_tag, "TLEN", 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = id3_frame_field(id_frame, 1);
		if (id_field != NULL)
		{
			uint32_t strings = id3_field_getnstrings(id_field);
			if (strings > 0)
			{
				const id3_ucs4_t *str = id3_field_getstrings(id_field, 0);
				if (str != NULL)
					ret->TotalTime = id3_ucs4_getnumber(str) / 1000;
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_ALBUM, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = NULL;
		uint32_t nStrings = 0;
		char *Album = NULL;

		id_field = id3_frame_field(id_frame, 1);
		nStrings = id3_field_getnstrings(id_field);

		if (nStrings > 0)
		{
			uint32_t i = 0;
			for (i = 0; i < nStrings; i++)
			{
				Album = (char *)id3_ucs4_latin1duplicate(id3_field_getstrings(id_field, 0));

				if (Album != NULL)
				{
					if (ret->Album == NULL)
					{
						ret->Album = (const char *)malloc(strlen(Album) + 1);
						memset((char *)ret->Album, 0x00, strlen(Album) + 1);
					}
					else
					{
						ret->Album = (const char *)realloc((char *)ret->Album, strlen(ret->Album) + strlen(Album) + 4);
						memcpy((char *)ret->Album + strlen(ret->Album), " / ", 4);
					}
					memcpy((char *)ret->Album + strlen(ret->Album), Album, strlen(Album) + 1);
					free(Album);
					Album = NULL;
				}
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_ARTIST, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = NULL;
		uint32_t nStrings = 0;
		char *Artist = NULL;

		id_field = id3_frame_field(id_frame, 1);
		nStrings = id3_field_getnstrings(id_field);

		if (nStrings > 0)
		{
			uint32_t i = 0;
			for (i = 0; i < nStrings; i++)
			{
				Artist = (char *)id3_ucs4_latin1duplicate(id3_field_getstrings(id_field, 0));

				if (Artist != NULL)
				{
					if (ret->Artist == NULL)
					{
						ret->Artist = (const char *)malloc(strlen(Artist) + 1);
						memset((char *)ret->Artist, 0x00, strlen(Artist) + 1);
					}
					else
					{
						ret->Artist = (const char *)realloc((char *)ret->Artist, strlen(ret->Artist) + strlen(Artist) + 4);
						memcpy((char *)ret->Artist + strlen(ret->Artist), " / ", 4);
					}
					memcpy((char *)ret->Artist + strlen(ret->Artist), Artist, strlen(Artist) + 1);
					free(Artist);
					Artist = NULL;
				}
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_TITLE, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = NULL;
		uint32_t nStrings = 0;
		char *Title = NULL;

		id_field = id3_frame_field(id_frame, 1);
		nStrings = id3_field_getnstrings(id_field);

		if (nStrings > 0)
		{
			uint32_t i = 0;
			for (i = 0; i < nStrings; i++)
			{
				Title = (char *)id3_ucs4_latin1duplicate(id3_field_getstrings(id_field, 0));

				if (Title != NULL)
				{
					if (ret->Title == NULL)
					{
						ret->Title = (const char *)malloc(strlen(Title) + 1);
						memset((char *)ret->Title, 0x00, strlen(Title) + 1);
					}
					else
					{
						ret->Title = (const char *)realloc((char *)ret->Title, strlen(ret->Title) + strlen(Title) + 4);
						memcpy((char *)ret->Title + strlen(ret->Title), " / ", 4);
					}
					memcpy((char *)ret->Title + strlen(ret->Title), Title, strlen(Title) + 1);
					free(Title);
					Title = NULL;
				}
			}
		}
	}

	id_frame = id3_tag_findframe(id_tag, ID3_FRAME_COMMENT, 0);
	if (id_frame != NULL)
	{
		id3_field *id_field = NULL;
		uint32_t nStrings = 0;
		uint8_t *Comment = NULL;

		id_field = id3_frame_field(id_frame, 1);
		nStrings = id3_field_getnstrings(id_field);

		if (nStrings > 0)
		{
			uint32_t i = 0;
			for (i = 0; i < nStrings; i++)
			{
				Comment = id3_ucs4_latin1duplicate(id3_field_getstrings(id_field, 0));

				if (Comment != NULL)
				{
					if (ret->OtherComments.size() == 0)
						ret->nOtherComments = 0;

					ret->OtherComments.push_back(strdup((char *)Comment));
					free(Comment);
					Comment = NULL;
					ret->nOtherComments++;
				}
			}
		}
	}

	fseek(p_MF->f_MP3, id_tag->paddedsize, SEEK_SET);
	id3_file_close(f_id3);

	rem = (p_MF->p_Stream->buffer == NULL ? 0 : p_MF->p_Stream->bufend - p_MF->p_Stream->next_frame);
	fread(p_MF->inbuff + rem, 2 * 8192 - rem, 1, p_MF->f_MP3);
	mad_stream_buffer(p_MF->p_Stream, (const uint8_t *)p_MF->inbuff, (2 * 8192));

	while (p_MF->p_Frame->header.bitrate == 0 || p_MF->p_Frame->header.samplerate == 0)
		mad_frame_decode(p_MF->p_Frame, p_MF->p_Stream);
	p_MF->InitialFrame = true;
	ret->BitRate = p_MF->p_Frame->header.samplerate;
	ret->BitsPerSample = 16;
	ret->Channels = (p_MF->p_Frame->header.mode == MAD_MODE_SINGLE_CHANNEL ? 1 : 2);

	if (ExternalPlayback == 0)
		p_MF->p_Playback = new Playback(ret, MP3_FillBuffer, p_MF->buffer, 8192, p_MP3File);
	p_MF->p_FI = ret;

	return ret;
}

/*!
 * Closes an opened audio file
 * @param p_MP3File A pointer to a file opened with \c MP3_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_MP3File after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_MP3File must not be NULL as no checking on the parameter is done. FIXME!
 */
int MP3_CloseFileR(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	int ret;

	delete p_MF->p_Playback;

	mad_synth_finish(p_MF->p_Synth);
	mad_frame_finish(p_MF->p_Frame);
	mad_stream_finish(p_MF->p_Stream);
	//mad_timer_reset(p_MF->p_Timer);

	free(p_MF->f_name);
	ret = fclose(p_MF->f_MP3);
	free(p_MF);
	return ret;
}

/*!
 * @internal
 * Gets the next buffer of MP3 data from the MP3 file
 * @param p_MF Our internal MP3 context
 * @param eof The pointer to the internal eof member
 *   passed to help speed this function up slightly by not having
 *   to re-dereference the pointers and memory needed to locate
 *   the member
 */
void GetData(MP3_Intern *p_MF, bool *eof)
{
	int rem = (p_MF->p_Stream->buffer == NULL ? 0 : p_MF->p_Stream->bufend - p_MF->p_Stream->next_frame);

	if (p_MF->p_Stream->next_frame != NULL)
		memmove(p_MF->inbuff, p_MF->p_Stream->next_frame, rem);

	fread(p_MF->inbuff + rem, (2 * 8192) - rem, 1, p_MF->f_MP3);
	*eof = (feof(p_MF->f_MP3) == FALSE ? false : true);

	if (*eof == true)
		memset(p_MF->inbuff + (2 * 8192), 0x00, 8);

	mad_stream_buffer(p_MF->p_Stream, (const uint8_t *)p_MF->inbuff, (2 * 8192));
}

/*!
 * @internal
 * Loads the next frame of audio from the MP3 file
 * @param p_MF Our internal MP3 context
 * @param eof A pointer to the internal eof member
 *   for passing to \c GetData()
 */
int DecodeFrame(MP3_Intern *p_MF, bool *eof)
{
	if (p_MF->InitialFrame == false)
	{
		if (mad_frame_decode(p_MF->p_Frame, p_MF->p_Stream) != 0)
		{
			if (MAD_RECOVERABLE(p_MF->p_Stream->error) == 0)
			{
				if (p_MF->p_Stream->error == MAD_ERROR_BUFLEN)
				{
					GetData(p_MF, eof);
					if (*eof == true)
						return -2;
					return DecodeFrame(p_MF, eof);
				}
				else
				{
					printf("Unrecoverable frame level error (%i).\n", p_MF->p_Stream->error);
					return -1;
				}
			}
		}
	}

	return 0;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_MP3File A pointer to a file opened with \c MP3_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_MP3File must not be NULL as no checking on the parameter is done. FIXME!
 */
long MP3_FillBuffer(void *p_MP3File, uint8_t *OutBuffer, int nOutBufferLen)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;
	uint8_t *OBuff = OutBuffer;

	while ((OBuff - OutBuffer) < nOutBufferLen && p_MF->eof == false)
	{
		short *out = (short *)(p_MF->buffer + (OBuff - OutBuffer));
		int nOut = 0;

		if (p_MF->PCMDecoded == 0)
		{
			int ret;
			// Get input if needed, get the stream buffer part of libMAD to process that input.
			if (p_MF->p_Stream->buffer == NULL || p_MF->p_Stream->error == MAD_ERROR_BUFLEN)
				GetData(p_MF, &p_MF->eof);

			// Decode a frame:
			if ((ret = DecodeFrame(p_MF, &p_MF->eof)) != 0)
				return ret;

			if (p_MF->InitialFrame == true)
				p_MF->InitialFrame = false;

			// Synth the PCM
			mad_synth_frame(p_MF->p_Synth, p_MF->p_Frame);
		}

		// copy the PCM to our output buffer
		for (int i = p_MF->PCMDecoded; i < p_MF->p_Synth->pcm.length; i++)
		{
			short Sample = 0;

			Sample = FixedToShort(p_MF->p_Synth->pcm.samples[0][i]);
			out[((i - p_MF->PCMDecoded) * p_MF->p_FI->Channels) + 0] = Sample;

			if (p_MF->p_FI->Channels == 2)
			{
				Sample = FixedToShort(p_MF->p_Synth->pcm.samples[1][i]);
				out[((i - p_MF->PCMDecoded) * p_MF->p_FI->Channels) + 1] = Sample;
			}

			nOut += sizeof(short) * p_MF->p_FI->Channels;

			if (((OBuff - OutBuffer) + nOut) >= nOutBufferLen)
			{
				p_MF->PCMDecoded = i + 1;
				i = p_MF->p_Synth->pcm.length;
			}
		}

		if (((OBuff - OutBuffer) + nOut) < nOutBufferLen)
		{
			p_MF->PCMDecoded = 0;
		}

		memcpy(OBuff, out, nOut);
		OBuff += nOut;
	}

	return OBuff - OutBuffer;
}

/*!
 * Plays an opened MP3 file using OpenAL on the default audio device
 * @param p_MP3File A pointer to a file opened with \c MP3_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c MP3_OpenR() used to open the file at \p p_MP3File,
 * this function will do nothing.
 * @bug \p p_MP3File must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_MP3File check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void MP3_Play(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;

	p_MF->p_Playback->Play();
}

void MP3_Pause(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;

	p_MF->p_Playback->Pause();
}

void MP3_Stop(void *p_MP3File)
{
	MP3_Intern *p_MF = (MP3_Intern *)p_MP3File;

	p_MF->p_Playback->Stop();
}

/*!
 * Checks the file given by \p FileName for whether it is an MP3
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an MP3 file or not
 */
bool Is_MP3(const char *FileName)
{
	FILE *f_MP3 = fopen(FileName, "rb");
	char ID3[3];
	char MP3Sig[2];

	if (f_MP3 == NULL)
		return false;

	fread(ID3, 3, 1, f_MP3);
	fseek(f_MP3, 0, SEEK_SET);
	fread(MP3Sig, 2, 1, f_MP3);
	fclose(f_MP3);

	if (strncmp(ID3, "ID3", 3) == 0)
		return true;
	if ((((((short)MP3Sig[0]) << 8) | MP3Sig[1]) & 0xFFFA) == 0xFFFA)
		return true;

	return false;
}

/*!
 * @internal
 * This structure controls decoding MP3 files when using the high-level API on them
 */
API_Functions MP3Decoder =
{
	MP3_OpenR,
	MP3_GetFileInfo,
	MP3_FillBuffer,
	MP3_CloseFileR,
	MP3_Play,
	MP3_Pause,
	MP3_Stop
};
