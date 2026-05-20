#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>


PyObject*
PuringDir_stat(
    // UringDir *self,
    PyObject *args,
    PyObject *kwargs
);