#include <exception>
#include <substrate/utility>
#include <libAudio.h>
#include "audioFile.hxx"

constexpr static auto ctorKeywords{substrate::make_array<const char *>({
	"fileName", nullptr
})};

constexpr static auto playKeywords{substrate::make_array<const char *>({
	"wait", nullptr
})};

pyAudioFile_t::pyAudioFile_t(const char *const fileName) : PyObject{},
	audioFile{static_cast<audioFile_t*>(audioOpenR(fileName))}, playbackFinished{} { }

pyAudioFile_t::~pyAudioFile_t() noexcept
{
	if (audioFile)
		audioFile->stop();
}

pyAudioFile_t *pyAudioFile_t::atAddress(void *self, PyObject *args, PyObject *kwargs)
{
	const char *fileName = nullptr;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:__init__", const_cast<char **>(ctorKeywords.data()), &fileName))
		throw std::exception{};
	return new (self) pyAudioFile_t{fileName};
}

PyObject *pyAudioFile_t::repr() const noexcept
	{ return PyUnicode_FromFormat("<AudioFile: %p>", audioFile.get()); }

PyObject *pyAudioFile_t::play(PyObject *args, PyObject *kwargs) noexcept
{
	PyObject *pyWait{nullptr};
	if (!audioFile)
	{
		PyErr_SetString(PyExc_ValueError, "AudioFile in invalid state - audioFile is null");
		return nullptr;
	}
	else if (playbackFinished.valid())
	{
		PyErr_SetString(PyExc_ValueError, "AudioFile already playing");
		return nullptr;
	}
	else if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!:play", const_cast<char **>(playKeywords.data()),
		PyBool_Type, &pyWait))
		return nullptr;
	const bool wait = pyWait == Py_True;
	if (wait)
		audioFile->play();
	else
	{
		playbackFinished = std::async(std::launch::async,
			[](audioFile_t *const audioFile) noexcept -> bool
			{
				audioFile->play();
				return true;
			}, audioFile.get()
		);
	}
	return Py_None;
}

PyObject *pyAudioFile_t::pause(PyObject *) noexcept
{
	if (!audioFile)
	{
		PyErr_SetString(PyExc_ValueError, "AudioFile in invalid state - audioFile is null");
		return nullptr;
	}
	else if (!playbackFinished.valid())
	{
		PyErr_SetString(PyExc_ValueError, "AudioFile in invalid state - no playback in progress");
		return nullptr;
	}
	audioFile->pause();
	playbackFinished.get();
	return Py_None;
}

PyObject *pyAudioFile_t::stop(PyObject *) noexcept
{
	if (!audioFile)
	{
		PyErr_SetString(PyExc_ValueError, "AudioFile in invalid state - audioFile is null");
		return nullptr;
	}
	else if (!playbackFinished.valid())
	{
		PyErr_SetString(PyExc_ValueError, "AudioFile in invalid state - no playback in progress");
		return nullptr;
	}
	audioFile->stop();
	playbackFinished.get();
	return Py_None;
}