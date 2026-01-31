#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <sys/types.h>
#include <stdbool.h>

#include "loop.h"


static PyObject *UringLoop_open(
    UringLoop* loop, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static int UringLoop_read(
    UringLoop* loop, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static void UringLoop_write(
    UringLoop* loop, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject *UringLoop_close(
    UringLoop* loop, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static int UringLoop_stat(
    UringLoop* loop, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static void UringLoop_fsync(
    UringLoop* loop, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);
