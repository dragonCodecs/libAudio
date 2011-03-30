#ifndef __NO_MPC__

#include <sys/stat.h>
#include <sys/types.h>
#include <malloc.h>

#ifdef _WINDOWS
#include <mpc/mpcdec.h>
#else
#include <MusePack/mpcdec.h>
#endif

#include "libAudio.h"
#include "libAudio_Common.h"

/*!
 * @internal
 * @file loadMPC.cpp
 * The implementation of the MPC decoder API
 * @author Richard Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2011
 */

#ifdef _WINDOWS
/*!
 * Definition of \c uint64_t as per inttypes.h
 */
typedef unsigned long long uint64_t;
/*!
 * Definition of \c uint32_t as per inttypes.h
 */
typedef unsigned int uint32_t;
/*!
 * Definition of \c uint16_t as per inttypes.h
 */
typedef unsigned short uint16_t;
/*!
 * Definition of \c uint8_t as per inttypes.h
 */
typedef unsigned char uint8_t;
#endif

#include <limits.h>

#ifndef SHRT_MAX
/*!
 * @internal
 * Backup definition of \c SHORT_MAX, which should be defined in limit.h,
 * but which assumes that short's length is 16 bits
 */
#define SHORT_MAX 0x7FFF
#else
/*!
 * @internal
 * Definition of \c SHORT_MAX, which takes on the value of \c SHRT_MAX
 * from limit.h
 */
#define SHORT_MAX SHRT_MAX
#endif

/*!
 * @internal
 * Internal structure for holding the decoding context for a given MPC file
 */
typedef struct _MPC_Intern
{
	/*!
	 * @internal
	 * The MPC file to decode
	 */
	FILE *f_MPC;
	/*!
	 * @internal
	 * The playback class instance for the MPC file
	 */
	Playback *p_Playback;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	BYTE buffer[8192];
	/*!
	 * @internal
	 * The MPC callbacks/reader information handle
	 */
	mpc_reader *callbacks;
	/*!
	 * @internal
	 * The MPC stream information handle which holds the metadata
	 * returned by \c mpc_demux_get_info()
	 */
	mpc_streaminfo *info;
	/*!
	 * @internal
	 * The MPC demultiplexer handle
	 */
	mpc_demux *demuxer;
	/*!
	 * @internal
	 * The \c FileInfo for the MP3 file being decoded
	 */
	FileInfo *p_FI;
	/*!
	 * @internal
	 * The MPC frame handle which holds, among other things, the decoded
	 * MPC data returned by \c mpc_demux_decode()
	 */
	mpc_frame_info *frame;
	/*!
	 * @internal
	 * The decoded MPC buffer filled by \c mpc_demux_decode()
	 */
	MPC_SAMPLE_FORMAT framebuff[MPC_DECODER_BUFFER_LENGTH];
	/*!
	 * @internal
	 * The count of how much of the current decoded data buffer has been used
	 */
	uint32_t PCMUsed;
} MPC_Intern;

/*!
 * @internal
 * This function applies a simple conversion algorithm to convert the input
 * floating point MPC sample to a short for playback
 * @param Sample The floating point sample to convert
 * @return The converted floating point sample
 * @bug This function applies no noise shaping or dithering
 *   So the output is sub-par to what it could be. FIXME!
 */
short FloatToShort(MPC_SAMPLE_FORMAT Sample)
{
	float s = (Sample * SHORT_MAX);

	// Perform cliping
	if (s < -SHORT_MAX)
		s = -SHORT_MAX;
	if (s > SHORT_MAX)
		s = SHORT_MAX;

	return (short)s;
}

/*!
 * @internal
 * \c f_fread() is the internal read callback for MPC file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_MPCFile Structure holding our own internal context pointer which
 *   holds the file to seek through
 * @param p_Buffer The buffer to read into
 * @param size The number of bytes to read into the buffer
 * @return The return result of \c fread()
 */
INT f_fread(mpc_reader *p_MPCFile, void *p_Buffer, int size)
{
	MPC_Intern *p_MF = (MPC_Intern *)(p_MPCFile->data);
	return fread(p_Buffer, 1, size, p_MF->f_MPC);
}

/*!
 * @internal
 * \c f_fseek() is the internal seek callback for MPC file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_MPCFile Structure holding our own internal context pointer which
 *   holds the file to seek through
 * @param offset The offset through the file to which to seek to
 * @return A truth value giving if the seek succeeded or not
 */
UCHAR f_fseek(mpc_reader *p_MPCFile, int offset)
{
	MPC_Intern *p_MF = (MPC_Intern *)(p_MPCFile->data);
	return (fseek(p_MF->f_MPC, offset, SEEK_SET) == 0 ? TRUE : FALSE);
}

/*!
 * @internal
 * \c f_ftell() is the internal read possition callback for MPC file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_MPCFile Structure holding our own internal context pointer which
 *   holds the file to seek through
 * @return An integer giving the read possition of the file in bytes
 */
int f_ftell(mpc_reader *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)(p_MPCFile->data);
	return ftell(p_MF->f_MPC);
}

/*!
 * @internal
 * \c f_flen() is the internal file length callback for MPC file decoding.
 * This prevents nasty things from happening on Windows thanks to the run-time mess there.
 * @param p_MPCFile Structure holding our own internal context pointer which
 *   holds the file to seek through
 * @return An integer giving the length of the file in bytes
 */
int f_flen(mpc_reader *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)(p_MPCFile->data);
	struct stat stats;

	fstat(fileno(p_MF->f_MPC), &stats);
	return stats.st_size;
}

/*!
 * @internal
 * \c f_fcanseek() is the internal callback for determining if an MPC file being
 * decoded can be seeked on or not. \n This does two things: \n
 * - It prevents nasty things from happening on Windows thanks to the run-time mess there
 * - It uses \c fseek() as a no-operation to determine if we can seek or not.
 *
 * @param p_MPCFile Structure holding our own internal context pointer which
 *   holds the file to seek through
 * @return A truth value giving if seeking can work or not
 */
UCHAR f_fcanseek(mpc_reader *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)(p_MPCFile->data);

	return (fseek(p_MF->f_MPC, 0, SEEK_CUR) == 0 ? TRUE : FALSE);
}

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by MPC_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *MPC_OpenR(const char *FileName)
{
	MPC_Intern *ret = NULL;
	FILE *f_MPC = NULL;

	f_MPC = fopen(FileName, "rb");
	if (f_MPC == NULL)
		return ret;

	ret = (MPC_Intern *)malloc(sizeof(MPC_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(MPC_Intern));

	ret->f_MPC = f_MPC;
	ret->callbacks = (mpc_reader *)malloc(sizeof(mpc_reader));
	ret->callbacks->read = f_fread;
	ret->callbacks->seek = f_fseek;
	ret->callbacks->tell = f_ftell;
	ret->callbacks->get_size = f_flen;
	ret->callbacks->canseek = f_fcanseek;
	ret->callbacks->data = ret;
	ret->demuxer = mpc_demux_init(ret->callbacks);

	ret->info = (mpc_streaminfo *)malloc(sizeof(mpc_streaminfo));
	ret->frame = (mpc_frame_info *)malloc(sizeof(mpc_frame_info));
	ret->frame->buffer = ret->framebuff;

	return ret;
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_MPCFile A pointer to a file opened with \c MPC_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c MPC_Play() or \c MPC_FillBuffer()
 * @bug \p p_MPCFile must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *MPC_GetFileInfo(void *p_MPCFile)
{
	FileInfo *ret = NULL;
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	memset(ret, 0x00, sizeof(FileInfo));

	mpc_demux_get_info(p_MF->demuxer, p_MF->info);

	p_MF->p_FI = ret;
	ret->BitsPerSample = (p_MF->info->bitrate == 0 ? 16 : p_MF->info->bitrate);
	ret->BitRate = p_MF->info->sample_freq;
	ret->Channels = p_MF->info->channels;
	ret->TotalTime = ((double)p_MF->info->samples) / ((double)ret->BitRate);

	if (ExternalPlayback == 0)
		p_MF->p_Playback = new Playback(ret, MPC_FillBuffer, p_MF->buffer, 8192, p_MPCFile);

	return ret;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_MPCFile A pointer to a file opened with \c MPC_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_MPCFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long MPC_FillBuffer(void *p_MPCFile, BYTE *OutBuffer, int nOutBufferLen)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	BYTE *OBuff = OutBuffer;

	while (OBuff - OutBuffer < nOutBufferLen)
	{
		short *out = (short *)(p_MF->buffer + (OBuff - OutBuffer));
		int nOut = 0;

		if (p_MF->PCMUsed == 0)
		{
			if (mpc_demux_decode(p_MF->demuxer, p_MF->frame) != 0 || p_MF->frame->bits == -1)
				return -2;
		}

		for (uint32_t i = p_MF->PCMUsed; i < p_MF->frame->samples; i++)
		{
			short Sample = 0;

			if (p_MF->p_FI->Channels == 1)
			{
				Sample = FloatToShort(p_MF->frame->buffer[i]);
				out[(nOut / 2)] = Sample;
				nOut += 2;
			}
			else if (p_MF->p_FI->Channels == 2)
			{
				Sample = FloatToShort(p_MF->frame->buffer[(i * 2) + 0]);
				out[(nOut / 2) + 0] = Sample;

				Sample = FloatToShort(p_MF->frame->buffer[(i * 2) + 1]);
				out[(nOut / 2) + 1] = Sample;
				nOut += 4;
			}

			if (((OBuff - OutBuffer) + nOut) >= nOutBufferLen)
			{
				p_MF->PCMUsed = ++i;
				i = p_MF->frame->samples;
			}
		}

		if (((OBuff - OutBuffer) + nOut) < nOutBufferLen)
			p_MF->PCMUsed = 0;

		memcpy(OBuff, out, nOut);
		OBuff += nOut;
	}

	return OBuff - OutBuffer;
}

/*!
 * Closes an opened audio file
 * @param p_MPCFile A pointer to a file opened with \c MPC_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_MPCFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_MPCFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int MPC_CloseFileR(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;
	int ret;

	delete p_MF->p_Playback;

	mpc_demux_exit(p_MF->demuxer);
	free(p_MF->info);
	free(p_MF->frame);
	free(p_MF->callbacks);

	ret = fclose(p_MF->f_MPC);
	free(p_MF);
	return ret;
}

/*!
 * Plays an opened MPC file using OpenAL on the default audio device
 * @param p_MPCFile A pointer to a file opened with \c MPC_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c MPC_OpenR() used to open the file at \p p_MPCFile,
 * this function will do nothing.
 * @bug \p p_MPCFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_MPCFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void MPC_Play(void *p_MPCFile)
{
	MPC_Intern *p_MF = (MPC_Intern *)p_MPCFile;

	p_MF->p_Playback->Play();
}

/*!
 * Checks the file given by \p FileName for whether it is an MPC
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an MPC file or not
 */
bool Is_MPC(const char *FileName)
{
	FILE *f_MPC = fopen(FileName, "rb");
	char MPCSig[3];

	if (f_MPC == NULL)
		return false;

	fread(MPCSig, 3, 1, f_MPC);
	fclose(f_MPC);

	if (strncmp(MPCSig, "MP+", 3) != 0 &&
		strncmp(MPCSig, "MPC", 3) != 0)
		return false;

	return true;
}

/*!
 * @internal
 * This structure controls decoding MPC files when using the high-level API on them
 */
API_Functions MPCDecoder =
{
	MPC_OpenR,
	MPC_GetFileInfo,
	MPC_FillBuffer,
	MPC_CloseFileR,
	MPC_Play
};

#endif /* __NO_MPC__ */

