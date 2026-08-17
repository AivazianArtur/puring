#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <linux/openat2.h>
#ifndef RESOLVE_CACHED
#define RESOLVE_CACHED 0x20
#endif

#include <liburing.h>

#include "ops/files/files.h"
#include "registry/registry.h"
#include "timer/timer.h"
#include "buffer_controllers/buffer_controllers.h"

#include "python_api/loop/loop.h"
#include "python_api/timer/timer.h"
#include "python_macroses.h"

extern PyTypeObject *UringioLoopType;
extern PyTypeObject UringioFileType;

typedef struct UringioFile {
    PyObject_HEAD

        int fd;
    UringioLoop *loop;
    bool closed;
} UringioFile;

PyObject *
Uringio_open(PyObject *module, PyObject *args, PyObject *kwargs);

int
UringioFile_traverse(UringioFile *self, visitproc visit, void *arg);

int
UringioFile_clear(UringioFile *self);

PyObject *
UringioFile_aenter(UringioFile *self, PyObject *Py_UNUSED(ignored));

PyObject *
UringioFile_aexit(UringioFile *self, PyObject *args, PyObject *kwargs);

void
UringioFile_dealloc(UringioFile *self);

PyObject *
UringioFile_read(UringioFile *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioFile_readv(UringioFile *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioFile_readv_raw(UringioFile *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioFile_write(UringioFile *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioFile_writev(UringioFile *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioFile_writev_raw(UringioFile *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioFile_close(UringioFile *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioFile_fsync(UringioFile *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioFile_fdatasync(UringioFile *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioFile_splice(UringioFile *self, PyObject *args, PyObject *kwargs);

PyObject *
_check_file_result(int result, UringioFile *file, int request_idx, PyObject *future);

PyObject *
create_resolve_enum(void);
PyObject *
create_statx_flags_enum(void);
PyObject *
create_statx_mask_enum(void);

PyObject *
_raise_file_exception_group(PyObject *body_exc_type, PyObject *body_exc_val, PyObject *body_exc_tb);

int
read_dispatcher(
    UringioFile *self,
    BufferPayload *buffer_payload,
    StreamStrategy stream,
    int request_idx,
    int size,
    int offset,
    TimeoutParams timeout_params
);

int
readv_dispatcher(
    UringioFile *self,
    BufferPayload *buffer_payload,
    int request_idx,
    int offset,
    int nowait,
    TimeoutParams timeout_params
);

int
write_dispatcher(
    UringioFile *self, BufferPayload *buffer_payload, int request_idx, int offset, TimeoutParams timeout_params
);
