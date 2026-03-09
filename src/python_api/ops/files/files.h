#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <fcntl.h>

#include <liburing.h>

#include "ops/files/files.h"
#include "registry/registry.h"
#include "python_api/loop/loop.h"
#include "python_api/future/future.h"


PyTypeObject UringFileType;

typedef struct UringFile {
    PyObject_HEAD

    int fd;
    UringLoop *loop;
    bool closed;
} UringFile;


PyObject* 
UringLoop_open(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);

void 
UringFile_dealloc(UringFile *self);

PyObject*
UringFile_read(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject*
UringFile_write(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject*
UringFile_close(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject*
UringFile_stat(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject*
UringFile_fsync(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
);
