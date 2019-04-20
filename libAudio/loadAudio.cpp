#include <map>
#include "libAudio.h"
#include "libAudio.hxx"

/*!
 * @internal
 * @file loadAudio.cpp
 * @brief The implementation of the master encoder API
 * @author Rachel Mant <dx-mon@users.sourceforge.net>
 * @date 2010-2019
 */

const std::map<fileIs_t, fileOpenR_t> loaders
{
	{Is_OggVorbis, OggVorbis_OpenR},
	{Is_FLAC, FLAC_OpenR},
	{Is_WAV, WAV_OpenR},
	{Is_M4A, M4A_OpenR},
	{Is_AAC, AAC_OpenR},
	{Is_MP3, MP3_OpenR},
	{Is_IT, IT_OpenR},
	{Is_MOD, MOD_OpenR},
	{Is_S3M, S3M_OpenR},
	{Is_STM, STM_OpenR},
#ifdef ENABLE_AON
	{Is_AON, AON_OpenR},
#endif
#ifdef ENABLE_FC1x
	{Is_FC1x, FC1x_OpenR},
#endif
#ifdef ENABLE_OptimFROG
	{Is_OptimFROG, OptimFROG_OpenR},
#endif
#ifdef ENABLE_WMA
	{Is_WMA, WMA_OpenR},
#endif
	{Is_MPC, MPC_OpenR},
	{Is_WavPack, WavPack_OpenR}
};

/*!
 * \c ExternalPlayback defaults on library initialisation to 0 and holds whether or not
 * internal playback initialisation is active or not via being a truth value of whether
 * external playback is wanted or not - 0 means internal is active.
 */
uint8_t ExternalPlayback = 0;
uint8_t ToPlayback = 1;

/*!
 * This function opens the file given by \c FileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by Audio_* functions
 * @param FileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c NULL if there was an error
 */
void *audioOpenR(const char *const fileName)
{
	for (const auto &loader : loaders)
	{
		if (loader.first(fileName))
			return loader.second(fileName);
	}
	return nullptr;
}

/*!
 * This function gets the \c FileInfo structure for an opened file
 * @param p_AudioPtr A pointer to a file opened with \c Audio_OpenR(), or \c NULL for a no-operation
 * @return A \c FileInfo pointer containing various metadata about an opened file or \c NULL
 * @warning This function must be called before using \c Audio_Play() or \c Audio_FillBuffer()
 */
const fileInfo_t *audioFileInfo(void *audioFile)
{
	const auto file = static_cast<const audioFile_t *>(audioFile);
	if (!file)
		return nullptr;
	return &file->fileInfo();
}

const fileInfo_t *audioGetFileInfo(void *audioFile) { return audioFileInfo(audioFile); }

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param p_AudioPtr A pointer to a file opened with \c Audio_OpenR(), or \c NULL for a no-operation
 * @param OutBuffer A pointer to the buffer to be filled
 * @param nOutBufferLen An integer giving how long the output buffer is as a maximum fill-length
 * @return Either a negative value when an error condition is entered,
 * or the number of bytes written to the buffer
 */
int64_t audioFillBuffer(void *audioFile, void *const buffer, const uint32_t length)
{
	const auto file = static_cast<audioFile_t *>(audioFile);
	if (!file)
		return 0;
	return file->fillBuffer(static_cast<void *>(buffer), uint32_t(length));
}

/*!
 * Closes an opened audio file
 * @param p_AudioPtr A pointer to a file opened with \c Audio_OpenR(), or \c NULL for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p p_AudioPtr after using
 * this function - please either set it to \c NULL or be extra carefull
 * to destroy it via scope
 */
int audioCloseFile(void *audioFile)
{
	const auto file = static_cast<const audioFile_t *>(audioFile);
	delete file;
	return 0;
}

/*!
 * Plays an opened audio file using OpenAL on the default audio device
 * @param p_AudioPtr A pointer to a file opened with \c Audio_OpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c Audio_OpenR() used to open the file at \p p_AudioPtr,
 * this function will do nothing.
 */
void audioPlay(void *audioFile)
{
	const auto file = static_cast<audioFile_t *>(audioFile);
	if (file)
		file->play();
}

void audioFile_t::play()
{
	if (_player)
		_player->play();
}

void audioPause(void *audioFile)
{
	const auto file = static_cast<audioFile_t *>(audioFile);
	if (file)
		file->pause();
}

void audioFile_t::pause()
{
	if (_player)
		_player->pause();
}

void audioStop(void *audioFile)
{
	const auto file = static_cast<audioFile_t *>(audioFile);
	if (file)
		file->stop();
}

void audioFile_t::stop()
{
	if (_player)
		_player->stop();
}

/*!
 * Checks the file given by \p FileName for whether it is an audio
 * file recognised by this library or not
 * @param FileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is audio or not
 */
bool Is_Audio(const char *FileName)
{
	if (Is_OggVorbis(FileName) == true)
		return true;
	else if (Is_FLAC(FileName) == true)
		return true;
	else if (Is_WAV(FileName) == true)
		return true;
	else if (Is_M4A(FileName) == true)
		return true;
	else if (Is_AAC(FileName) == true)
		return true;
	else if (Is_MP3(FileName) == true)
		return true;
	else if (Is_IT(FileName) == true)
		return true;
	else if (Is_MOD(FileName) == true)
		return true;
	else if (Is_S3M(FileName) == true)
		return true;
	else if (Is_STM(FileName) == true)
		return true;
	else if (Is_AON(FileName) == true)
		return true;
#ifdef ENABLE_FC1x
	else if (Is_FC1x(FileName) == true)
		return true;
#endif
	else if (Is_MPC(FileName) == true)
		return true;
	else if (Is_WavPack(FileName) == true)
		return true;
#ifdef ENABLE_OptimFROG
	else if (Is_OptimFROG(FileName) == true)
		return true;
#endif
#ifdef __WMA__
	// Add RealAudio call here when decoder is complete
	else if (Is_WMA(FileName) == true)
		return true;
#endif

	return false;
}
