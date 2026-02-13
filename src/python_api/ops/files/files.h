#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <sys/types.h>
#include <stdbool.h>

#include "python_api/loop/loop.h"


typedef struct UringLoop UringLoop;


PyObject* 
UringLoop_open(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject*
UringLoop_read(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject*
UringLoop_write(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject*
UringLoop_close(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject*
UringLoop_stat(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject*
UringLoop_fsync(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);
