#pragma once
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "loop.h"


static PyObject* create_future(UringLoop *self);
