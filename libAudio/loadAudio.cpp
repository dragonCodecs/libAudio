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
	{isOggVorbis, oggVorbisOpenR},
	{isFLAC, flacOpenR},
	{isWAV, wavOpenR},
	{isM4A, m4aOpenR},
	{isAAC, aacOpenR},
	{isMP3, mp3OpenR},
	{isIT, itOpenR},
	{isMOD, modOpenR},
	{isS3M, s3mOpenR},
	{isSTM, stmOpenR},
#ifdef ENABLE_AON
	{isAON, aonOpenR},
#endif
#ifdef ENABLE_FC1x
	{isFC1x, fc1xOpenR},
#endif
#ifdef ENABLE_OptimFROG
	{isOptimFROG, optimFROGOpenR},
#endif
#ifdef ENABLE_WMA
	{isWMA, wmaOpenR},
#endif
	{isMPC, mpcOpenR},
	{isWavPack, wavPackOpenR},
	{isOggOpus, oggOpusOpenR}
#ifdef ENABLE_SNDH
	,
	{isSNDH, sndhOpenR}
#endif
};

/*!
 * \c ExternalPlayback defaults on library initialisation to 0 and holds whether or not
 * internal playback initialisation is active or not via being a truth value of whether
 * external playback is wanted or not - 0 means internal is active.
 */
uint8_t ExternalPlayback = 0;
uint8_t ToPlayback = 1;

/*!
 * This function opens the file given by \c fileName for reading and playback and returns a pointer
 * to the context of the opened file which must be used only by Audio_* functions
 * @param fileName The name of the file to open
 * @return A void pointer to the context of the opened file, or \c nullptr if there was an error
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
 * This function gets the \c fileInfo_t structure for an opened file
 * @param audioFile A pointer to a file opened with \c audioOpenR(), or \c nullptr for a no-operation
 * @return A \c fileInfo_t pointer containing various metadata about an opened file or \c nullptr
 */
const fileInfo_t *audioFileInfo(void *audioFile)
{
	const auto file = static_cast<const audioFile_t *>(audioFile);
	if (!file)
		return nullptr;
	return &file->fileInfo();
}

/*!
 * This function is a synonym for \c audioFileInfo()
 * @param audioFile A pointer to a file opened with \c audioOpenR(), or \c nullptr for a no-operation
 * @return A \c fileInfo_t pointer containing various metadata about an opened file or \c nullptr
 */
const fileInfo_t *audioGetFileInfo(void *audioFile) { return audioFileInfo(audioFile); }

/*!
 * If using external playback or not using playback at all but rather wanting
 * to get PCM data, this function will do that by filling a buffer of any given length
 * with audio from an opened file.
 * @param audioFile A pointer to a file opened with \c audioOpenR(), or \c nullptr for a no-operation
 * @param buffer A pointer to the buffer to be filled
 * @param length An integer giving how long the output buffer is as a maximum fill-length
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
 * @param audioFile A pointer to a file opened with \c audioOpenR(), or \c nullptr for a no-operation
 * @return an integer indicating success or failure with the same values as \c fclose()
 * @warning Do not use the pointer given by \p audioFile after using
 * this function - please either set it to \c nullptr or be extra carefull
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
 * @param audioFile A pointer to a file opened with \c audioOpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c audioOpenR() used to open the file at \p audioFile,
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

/*!
 * Pauses playback on an audio file
 * @param audioFile A pointer to a file opened with \c audioOpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c audioOpenR() used to open the file at \p audioFile,
 * this function will do nothing.
 * @warning If \c audioPlay() has not been called, or either of
 * \c audioPause() and \c audioStop() have previously been called
 * with no interviening \c audioPlay() call, this function will do nothing.
 */
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

/*!
 * Stops playback on an audio file
 * @param audioFile A pointer to a file opened with \c audioOpenR()
 * @warning If \c ExternalPlayback was a non-zero value for
 * the call to \c audioOpenR() used to open the file at \p audioFile,
 * this function will do nothing.
 * @warning If \c audioPlay() has not been called, or either of
 * \c audioPause() and \c audioStop() have previously been called
 * with no interviening \c audioPlay() call, this function will do nothing.
 */
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
 * Checks the file given by \p fileName for whether it is an audio
 * file recognised by this library or not
 * @param fileName The name of the file to check
 * @return \c true if the file can be utilised by the library,
 * otherwise \c false
 * @note This function does not check the file extension, but rather
 * the file contents to see if it is audio or not
 */
bool isAudio(const char *fileName)
{
	for (const auto &loader : loaders)
	{
		if (loader.first(fileName))
			return true;
	}
	return false;
}
