#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadOggVorbis.cpp
 * @brief The implementation of the Ogg|Vorbis decoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2019
 */

/*!
 * @internal
 * Internal structure for holding the decoding context for a given Ogg|Vorbis file
 */
struct oggVorbis_t::decoderContext_t final
{
	decoderContext_t() noexcept;
	~decoderContext_t() noexcept;
};

typedef struct _OggVorbis_Intern
{
	/*!
	 * @internal
	 * The decoder context handle and handle to the Ogg|Vorbis
	 * file being decoded
	 */
	OggVorbis_File ovf;
	/*!
	 * @internal
	 * The internal decoded data buffer
	 */
	uint8_t buffer[8192];

	oggVorbis_t inner;
} OggVorbis_Intern;

oggVorbis_t::oggVorbis_t() noexcept : audioFile_t(audioType_t::oggVorbis, {}), ctx(makeUnique<decoderContext_t>()) { }
oggVorbis_t::decoderContext_t::decoderContext_t() noexcept { }

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by OggVorbis_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *OggVorbis_OpenR(const char *FileName)
{
	FILE *f_Ogg = fopen(FileName, "rb");
	if (f_Ogg == NULL)
		return nullptr;

	auto ret = makeUnique<OggVorbis_Intern>();
	if (!ret)
		return nullptr;

	ov_callbacks callbacks;
	callbacks.close_func = (int (__CDECL__ *)(void *))fclose;
	callbacks.read_func = (size_t (__CDECL__ *)(void *, size_t, size_t, void *))fread;
	callbacks.seek_func = fseek_wrapper;
	callbacks.tell_func = (long (__CDECL__ *)(void *))ftell;
	ov_open_callbacks(f_Ogg, &ret->ovf, NULL, 0, callbacks);

	return ret.release();
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenR()
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c OggVorbis_Play() or \c OggVorbis_FillBuffer()
 * @bug \p p_VorbisFile must not be NULL as no checking on the parameter is done. FIXME!
 */
FileInfo *OggVorbis_GetFileInfo(void *p_VorbisFile)
{
	OggVorbis_Intern *p_VF = (OggVorbis_Intern *)p_VorbisFile;
	FileInfo *ret = NULL;
	vorbis_comment *comments = NULL;
	char **p_comments = NULL;
	int nComment = 0;
	vorbis_info *info = NULL;
	OggVorbis_File *p_OVFile = (OggVorbis_File *)p_VorbisFile;
	if (p_OVFile == NULL)
		return ret;

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	info = ov_info(p_OVFile, -1);
	comments = ov_comment(p_OVFile, -1);
	p_comments = comments->user_comments;

	ret->BitRate = info->rate;
	ret->Channels = info->channels;
	ret->BitsPerSample = 16;

	while (p_comments[nComment] && nComment < comments->comments)
	{
		if (strncasecmp(p_comments[nComment], "title=", 6) == 0)
		{
			if (ret->Title == NULL)
				ret->Title = strdup(p_comments[nComment] + 6);
			else
			{
				int nOCText = strlen(ret->Title);
				int nCText = strlen(p_comments[nComment] + 6);
				ret->Title = (const char *)realloc((char *)ret->Title, nOCText + nCText + 4);
				memcpy((char *)ret->Title + nOCText, " / ", 3);
				memcpy((char *)ret->Title + nOCText + 3, p_comments[nComment] + 6, 	nCText + 1);
			}
		}
		else if (strncasecmp(p_comments[nComment], "artist=", 7) == 0)
		{
			if (ret->Artist == NULL)
				ret->Artist = strdup(p_comments[nComment] + 7);
			else
			{
				int nOCText = strlen(ret->Artist);
				int nCText = strlen(p_comments[nComment] + 7);
				ret->Artist = (const char *)realloc((char *)ret->Artist, nOCText + nCText + 4);
				memcpy((char *)ret->Artist + nOCText, " / ", 3);
				memcpy((char *)ret->Artist + nOCText + 3, p_comments[nComment] + 6, nCText + 1);
			}
		}
		else if (strncasecmp(p_comments[nComment], "album=", 6) == 0)
		{
			if (ret->Album == NULL)
				ret->Album = strdup(p_comments[nComment] + 6);
			else
			{
				int nOCText = strlen(ret->Album);
				int nCText = strlen(p_comments[nComment] + 6);
				ret->Album = (const char *)realloc((char *)ret->Album, nOCText + nCText + 4);
				memcpy((char *)ret->Album + nOCText, " / ", 3);
				memcpy((char *)ret->Album + nOCText + 3, p_comments[nComment] + 6, nCText + 1);
			}
		}
		else
		{
			ret->OtherComments.push_back(strdup(p_comments[nComment]));
			ret->nOtherComments++;
		}
		nComment++;
	}

	if (ExternalPlayback == 0)
		p_VF->inner.player(makeUnique<playback_t>(p_VorbisFile, OggVorbis_FillBuffer, p_VF->buffer, 8192, ret));

	return ret;
}

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenR()
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 * @bug \p p_VorbisFile must not be NULL as no checking on the parameter is done. FIXME!
 */
long OggVorbis_FillBuffer(void *p_VorbisFile, uint8_t *OutBuffer, int nOutBufferLen)
{
	int bitstream = 0;
	OggVorbis_File *p_OVFile = (OggVorbis_File *)p_VorbisFile;
	long ret = 0;
	long readtot = 1;

	while (readtot != OV_HOLE && readtot != OV_EBADLINK && ret < nOutBufferLen && readtot != 0)
	{
		readtot = ov_read(p_OVFile, (char *)OutBuffer + ret, nOutBufferLen - ret, 0, 2, 1, &bitstream);
		if (0 < readtot)
			ret += readtot;
	}

	return ret;
}

int64_t oggVorbis_t::fillBuffer(void *const buffer, const uint32_t length) { return -1; }

oggVorbis_t::decoderContext_t::~decoderContext_t() noexcept { }

/*!
 * Closes an opened audio file
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_VorbisFile after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @bug \p p_VorbisFile must not be NULL as no checking on the parameter is done. FIXME!
 */
int OggVorbis_CloseFileR(void *p_VorbisFile)
{
	int ret = 0;
	OggVorbis_Intern *p_VF = (OggVorbis_Intern *)p_VorbisFile;

	ret = ov_clear(&p_VF->ovf);
	delete p_VF;
	return ret;
}

/*!
 * Plays an opened Ogg|Vorbis file using OpenAL on the default audio device
 * @param p_VorbisFile A pointer to a file opened with \c OggVorbis_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c OggVorbis_OpenR() used to open the file at \p p_VorbisFile,
 * this function will do nothing.
 * @bug \p p_VorbisFile must not be NULL as no checking on the parameter is done. FIXME!
 *
 * @bug Futher to the \p p_VorbisFile check bug on this function, if this function is
 *   called as a no-op as given by the warning, then it will also cause the same problem. FIXME!
 */
void OggVorbis_Play(void *p_VorbisFile)
{
	OggVorbis_Intern *p_VF = (OggVorbis_Intern *)p_VorbisFile;
	p_VF->inner.play();
}

void OggVorbis_Pause(void *p_VorbisFile)
{
	OggVorbis_Intern *p_VF = (OggVorbis_Intern *)p_VorbisFile;
	p_VF->inner.pause();
}

void OggVorbis_Stop(void *p_VorbisFile)
{
	OggVorbis_Intern *p_VF = (OggVorbis_Intern *)p_VorbisFile;
	p_VF->inner.stop();
}

/*!
 * Checks the file given by \p FileName for whether it is an Ogg|Vorbis
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is an Ogg|Vorbis file or not
 */
bool Is_OggVorbis(const char *FileName) { return oggVorbis_t::isOggVorbis(FileName); }

/*!
 * Checks the file descriptor given by \p fd for whether it represents a Ogg|Vorbis
 * file recognised by this library or not
 * @param fd The descriptor of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a Ogg|Vorbis file or not
 */
bool oggVorbis_t::isOggVorbis(const int32_t fd) noexcept
{
	char oggSig[4];
	char vorbisSig[6];
	if (fd == -1 ||
		read(fd, oggSig, 4) != 4 ||
		lseek(fd, 25, SEEK_CUR) != 29 ||
		read(fd, vorbisSig, 6) != 6 ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		strncmp(oggSig, "OggS", 4) != 0 ||
		strncmp(vorbisSig, "vorbis", 6) != 0)
		return false;
	return true;
}

/*!
 * Checks the file given by \p fileName for whether it is a Ogg|Vorbis
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is a Ogg|Vorbis file or not
 */
bool oggVorbis_t::isOggVorbis(const char *const fileName) noexcept
{
	fd_t file(fileName, O_RDONLY | O_NOCTTY);
	if (!file.valid())
		return false;
	return isOggVorbis(file);
}

/*!
 * @internal
 * This structure controls decoding Ogg|Vorbis files when using the high-level API on them
 */
API_Functions OggVorbisDecoder =
{
	OggVorbis_OpenR,
	OggVorbis_GetFileInfo,
	OggVorbis_FillBuffer,
	OggVorbis_CloseFileR,
	OggVorbis_Play,
	OggVorbis_Pause,
	OggVorbis_Stop
};
