#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file saveAudio.cpp
 * @brief The implementation of the master encoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2013
 */

/*!
 * This function opens the file given by \c FileName for writing and returns a pointer
 * to the context of the opened file which must be used only by Audio_* functions
 * @param FileName The name of the file to open
 * @param Type One of the AUDIO_* constants describing what codec to use for the file
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 * @note Currently only Ogg/Vorbis, FLAC and MP4 are supported. Other formats will be added
 * in following releases of the library.
 */
libAUDIO_API void *Audio_OpenW(const char *FileName, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_OpenW(FileName);
	else if (Type == AUDIO_FLAC)
		return FLAC_OpenW(FileName);
	else if (Type == AUDIO_MP4)
		return M4A_OpenW(FileName);
	else
		return NULL;
}

/*!
 * This function sets the \c FileInfo structure for an opened file
 * @param p_AudioPtr A pointer to a file opened with \c Audio_OpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file
 * @param Type One of the AUDIO_ constants describing what codec to use for the file
 * @warning This function must be called before using \c Audio_WriteBuffer()
 * @attention \p Type must have the same value as it had for \c Audio_OpenW()
 * @bug p_FI must not be NULL as no checking on the parameter is done. FIXME!
 */
libAUDIO_API void Audio_SetFileInfo(void *p_AudioPtr, FileInfo *p_FI, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		OggVorbis_SetFileInfo(p_AudioPtr, p_FI);
	else if (Type == AUDIO_FLAC)
		FLAC_SetFileInfo(p_AudioPtr, p_FI);
	else if (Type == AUDIO_MP4)
		M4A_SetFileInfo(p_AudioPtr, p_FI);
}

void audioFile_t::fileInfo(const FileInfo &) { }

/*!
 * This function writes a buffer of audio to an opened file
 * @param p_AudioPtr A pointer to a file opened with \c Audio_OpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @param Type One of the AUDIO_ constants describing what codec to use for the file
 * @warning May not work unless \c Audio_SetFileInfo() has been called beforehand
 * @attention \p Type must have the same value as it had for \c Audio_OpenW()
 */
libAUDIO_API long Audio_WriteBuffer(void *p_AudioPtr, uint8_t *InBuffer, int nInBufferLen, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_WriteBuffer(p_AudioPtr, InBuffer, nInBufferLen);
	else if (Type == AUDIO_FLAC)
		return FLAC_WriteBuffer(p_AudioPtr, InBuffer, nInBufferLen);
	else if (Type == AUDIO_MP4)
		return M4A_WriteBuffer(p_AudioPtr, InBuffer, nInBufferLen);
	else
		return -2;
}

long audioWriteBuffer(void *audioFile, uint8_t *buffer, int length)
{
	const auto file = static_cast<audioFile_t *>(audioFile);
	if (!file)
		return 0;
	return file->writeBuffer(static_cast<void *>(buffer), uint32_t(length));
}

int64_t audioFile_t::writeBuffer(const void *const, const uint32_t) { return 0; }

/*!
 * Closes an opened audio file
 * @param p_AudioPtr A pointer to a file opened with \c Audio_OpenW()
 * @param Type One of the AUDIO_ constants describing what codec to use for the file
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_AudioPtr after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 * @attention \p Type must have the same value as it had for \c Audio_OpenW()
 */
libAUDIO_API int Audio_CloseFileW(void *p_AudioPtr, int Type)
{
	if (Type == AUDIO_OGG_VORBIS)
		return OggVorbis_CloseFileW(p_AudioPtr);
	else if (Type == AUDIO_FLAC)
		return FLAC_CloseFileW(p_AudioPtr);
	else if (Type == AUDIO_MP4)
		return M4A_CloseFileW(p_AudioPtr);
	else
		return 0;
}
