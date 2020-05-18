#include <memory>
#include <substrate/utility>
#include "interface.hxx"
#include "audioFile.hxx"

#ifdef _WINDOWS
	#define EXPORT extern "C" __declspec(dllexport)
#elif __GNUC__ >= 4
	#define DEFAULT_VISIBILITY __attribute__((visibility("default")))
	#define EXPORT extern "C" DEFAULT_VISIBILITY
#else
	#error "You are using an unsupported or ancient compiler"
#endif

EXPORT PyObject *PyInit_libAudio();

static PyModuleDef libAudioPython
{
	PyModuleDef_HEAD_INIT,
	"libAudio",
	"Python bindings for libAudio",
	-1,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr
};

PyObject *cppTypeAlloc(PyTypeObject *subtype, const Py_ssize_t)
{
	const auto self = ::operator new(subtype->tp_basicsize);
	memset(self, 0, subtype->tp_basicsize);
	return static_cast<PyObject *>(self);
}

void cppTypeDealloc(PyObject *self) noexcept
	{ self->ob_type->tp_free(self); }

template<typename targetFunc_t, typename sourceFunc_t> targetFunc_t asFuncType(const sourceFunc_t func) noexcept
{
	auto *const addr = reinterpret_cast<void *>(func);
	return reinterpret_cast<targetFunc_t>(addr);
}

const static PyCFunctionWithKeywords pyAudioFilePlay = pyAudioFile_t::play;
static auto pyAudioFileFuncs{substrate::make_array<PyMethodDef>(
{
	{"play", asFuncType<PyCFunction>(pyAudioFilePlay), METH_VARARGS | METH_KEYWORDS, ""},
	{"pause", pyAudioFile_t::pause, METH_VARARGS, ""},
	{"stop", pyAudioFile_t::stop, METH_VARARGS, ""},
	{nullptr, nullptr, 0, nullptr} // Sentinel
})};

PyObject *pyAudioFileNew(PyTypeObject *subtype, PyObject *args, PyObject *kwargs) noexcept try
{
	std::unique_ptr<PyObject, cppObjDelete_t> self(subtype->tp_alloc(subtype, 0));
	auto *const stream = pyAudioFile_t::atAddress(self, args, kwargs);
	PyObject_Init(stream, subtype);
	return self.release();
}
catch (std::bad_alloc &) { return PyErr_NoMemory(); }
catch (std::system_error &error)
{
	errno = error.code().value();
	return PyErr_SetFromErrno(PyExc_IOError);
}
catch (std::exception &) { return nullptr; }

void pyAudioFileFree(void *self) noexcept
{
	[](PyObject *self) noexcept -> void { delete static_cast<pyAudioFile_t *>(self); }
		(static_cast<PyObject *>(self));
}

static PyTypeObject pyAudioFileType
{
	// Initialise a new dynamically loaded type object
	PyVarObject_HEAD_INIT(nullptr, 0)
	"libAudio.AudioFile", /* tp_name */
	sizeof(pyAudioFile_t), /* tp_basicsize */
	0, /* tp_itemsize */
	cppTypeDealloc, /*tp_dealloc*/
	0, /*tp_vectorcall_offset*/
	nullptr, /*tp_getattr*/
	nullptr, /*tp_setattr*/
	nullptr, /*tp_compare*/
	pyAudioFile_t::repr, /*tp_repr*/
	nullptr, /*tp_as_number*/
	nullptr, /*tp_as_sequence*/
	nullptr, /*tp_as_mapping*/
	nullptr, /*tp_hash */
	nullptr, /*tp_call*/
	nullptr, /*tp_str*/
	nullptr, /*tp_getattro*/
	nullptr, /*tp_setattro*/
	nullptr, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT, /*tp_flags*/
	"A file-backed stream", /*tp_doc*/
	nullptr, /*tp_traverse*/
	nullptr, /*tp_clear*/
	nullptr, /*tp_richcompare*/
	0, /*tp_weaklistoffset*/
	nullptr, /*tp_iter*/
	nullptr, /*tp_iternext*/
	pyAudioFileFuncs.data(), /*tp_methods*/
	nullptr, /* tp_members */
	nullptr, /*tp_getset*/
	nullptr, /*tp_base*/
	nullptr, /*tp_dict*/
	nullptr, /*tp_descr_get*/
	nullptr, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	nullptr, /*tp_init*/
	cppTypeAlloc, /*tp_alloc*/
	pyAudioFileNew, /*tp_new*/
	pyAudioFileFree, /*tp_free*/
	nullptr, /*tp_is_gc*/
	nullptr, /*tp_bases*/
	nullptr, /*tp_mro*/
	nullptr, /*tp_cache*/
	nullptr, /*tp_subclasses*/
	nullptr, /*tp_weaklist*/
	nullptr, /*tp_del*/
	0, /*tp_version_tag*/
	nullptr, /*tp_finalize*/
	nullptr, /*tp_vectorcall*/
	nullptr, /*tp_print*/
#ifdef COUNT_ALLOCS
	0, /*tp_allocations*/
	0, /*tp_frees*/
	0, /*tp_maxalloc*/
	nullptr, /*tp_prev*/
	nullptr  /*tp_next*/
#endif
};

bool registerType(PyObject *const module, PyTypeObject &type, const char *name)
{
	if (!module || PyType_Ready(&type))
		return false;
	Py_INCREF(&type);
	return !PyModule_AddObject(module, name, reinterpret_cast<PyObject *>(&type));
}

PyObject *PyInit_libAudio()
{
	PyObject *const module = PyModule_Create(&libAudioPython);
	if (!registerType(module, pyAudioFileType, "AudioFile"))
		return nullptr;
	return module;
}