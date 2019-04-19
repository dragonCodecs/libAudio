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
	std::unique_ptr<AudioPointer> ret = makeUnique<AudioPointer>();
	if (!ret)
		return nullptr;
	else if (Type == AUDIO_OGG_VORBIS)
		ret->API = &OggVorbisDecoder;
	else if (Type == AUDIO_FLAC)
		ret->API = &FLACDecoder;
	else if (Type == AUDIO_MP4)
		ret->API = &M4ADecoder;
	else
		return nullptr;

	if (!ret->API->OpenW)
		return nullptr;
	ret->p_AudioFile = ret->API->OpenW(FileName);
	if (!ret->p_AudioFile)
		return nullptr;
	return ret.release();
}

/*!
 * This function sets the \c FileInfo structure for an opened file
 * @param p_AudioPtr A pointer to a file opened with \c Audio_OpenW()
 * @param p_FI A \c FileInfo pointer containing various metadata about an opened file
 * @warning This function must be called before using \c Audio_WriteBuffer()
 */
libAUDIO_API bool Audio_SetFileInfo(void *p_AudioPtr, const fileInfo_t *const p_FI)
{
	const auto p_AP = static_cast<AudioPointer *>(p_AudioPtr);
	if (!p_AP || !p_AP || !p_FI || !p_AP->API->SetFileInfo)
		return false;
	return p_AP->API->SetFileInfo(p_AP->p_AudioFile, p_FI);
}

bool audioFileInfo(void *audioFile, const fileInfo_t *const fileInfo)
{
	const auto file = static_cast<audioFile_t *>(audioFile);
	if (file)
		return file->fileInfo(*fileInfo);
	return false;
}

bool audioFile_t::fileInfo(const fileInfo_t &) { return false; }

/*!
 * This function writes a buffer of audio to an opened file
 * @param p_AudioPtr A pointer to a file opened with \c Audio_OpenW()
 * @param InBuffer The buffer of audio to write
 * @param nInBufferLen An integer giving how long the buffer to write is
 * @warning May not work unless \c Audio_SetFileInfo() has been called beforehand
 */
libAUDIO_API long Audio_WriteBuffer(void *p_AudioPtr, uint8_t *InBuffer, int nInBufferLen)
{
	const auto p_AP = static_cast<AudioPointer *>(p_AudioPtr);
	if (!p_AP || !InBuffer || !p_AP->p_AudioFile)
		return -3;
	else if (!p_AP->API->WriteBuffer)
		return -2;
	return p_AP->API->WriteBuffer(p_AP->p_AudioFile, InBuffer, nInBufferLen);
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
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_AudioPtr after using
 * this function - please either set it to \c nullptr or be extra carefull
 * to destroy it via scope
 */
libAUDIO_API int Audio_CloseFileW(void *p_AudioPtr)
{
	const auto p_AP = static_cast<AudioPointer *>(p_AudioPtr);
	if (!p_AP || !p_AP->p_AudioFile || !p_AP->API->CloseFileW)
		return 0;
	const int result = p_AP->API->CloseFileW(p_AP->p_AudioFile);
	delete p_AP;
	return result;
}
