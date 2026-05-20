#pragma once
#define PY_SSIZE_T_CLEAN
#include <Python.h>


typedef struct PuringLoop PuringLoop;

PyObject* create_future(PuringLoop *self);
