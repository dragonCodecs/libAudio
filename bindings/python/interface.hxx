// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2023 Rachel Mant <git@dragonmux.network>
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
