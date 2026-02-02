#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <sys/types.h>
#include <stdbool.h>

#include "loop.h"


static PyObject* 
UringLoop_open(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject*
UringLoop_read(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject*
UringLoop_write(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject*
UringLoop_close(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject*
UringLoop_stat(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject*
UringLoop_fsync(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);
