// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2023 Rachel Mant <git@dragonmux.network>
#ifndef AUDIO_FILE__HXX
#define AUDIO_FILE__HXX

#include <future>
#include <libAudio.hxx>
#include "interface.hxx"

struct pyAudioFile_t final : public PyObject
{
private:
	std::unique_ptr<audioFile_t> audioFile;
	std::future<bool> playbackFinished;

	static pyAudioFile_t *atAddress(void *self, PyObject *args, PyObject *kwargs);
	PyObject *repr() const noexcept;

	PyObject *play(PyObject *args, PyObject *kwargs) noexcept;
	PyObject *pause(PyObject *args) noexcept;
	PyObject *stop(PyObject *args) noexcept;
	PyObject *mode(PyObject *args, PyObject *kwargs) noexcept;
	PyObject *playbackVolume(PyObject *args, PyObject *kwargs) noexcept;

public:
	pyAudioFile_t() noexcept : PyObject{}, audioFile{}, playbackFinished{} { }
	pyAudioFile_t(const char *const fileName);
	~pyAudioFile_t() noexcept;

	static pyAudioFile_t *atAddress(const std::unique_ptr<PyObject, cppObjDelete_t> &self,
		PyObject *args, PyObject *kwargs) { return atAddress(self.get(), args, kwargs); }
	static PyObject *repr(PyObject *self) noexcept
		{ return static_cast<pyAudioFile_t *>(self)->repr(); }

	static PyObject *play(PyObject *self, PyObject *args, PyObject *kwargs) noexcept
		{ return static_cast<pyAudioFile_t *>(self)->play(args, kwargs); }
	static PyObject *pause(PyObject *self, PyObject *args) noexcept
		{ return static_cast<pyAudioFile_t *>(self)->pause(args); }
	static PyObject *stop(PyObject *self, PyObject *args) noexcept
		{ return static_cast<pyAudioFile_t *>(self)->stop(args); }
	static PyObject *mode(PyObject *self, PyObject *args, PyObject *kwargs) noexcept
		{ return static_cast<pyAudioFile_t *>(self)->mode(args, kwargs); }
	static PyObject *playbackVolume(PyObject *self, PyObject *args, PyObject *kwargs) noexcept
		{ return static_cast<pyAudioFile_t *>(self)->playbackVolume(args, kwargs); }

	operator const audioFile_t *() const noexcept { return audioFile.get(); }
};

#endif /*AUDIO_FILE__HXX*/
