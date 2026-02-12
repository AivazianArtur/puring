#pragma once
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "loop/loop.h"


PyObject* create_future(UringLoop *self);
