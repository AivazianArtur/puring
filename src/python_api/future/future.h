#pragma once
#define PY_SSIZE_T_CLEAN
#include <Python.h>


typedef struct UringLoop UringLoop;

PyObject* create_future(UringLoop *self);
