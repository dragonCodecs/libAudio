#ifndef INTERFACE__HXX
#define INTERFACE__HXX

#define PY_SSIZE_T_CLEAN
#include <Python.h>

struct cppObjDelete_t final
{
	void operator ()(PyObject *self) noexcept
		{ ::operator delete(static_cast<void *>(self)); }
};

#endif /*INTERFACE__HXX*/
