#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>


PyObject*
UringDir_stat(
    // UringDir *self,
    PyObject *args,
    PyObject *kwargs
);