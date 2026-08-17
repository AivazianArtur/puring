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

extern PyTypeObject *AioUringLoopType;
extern PyTypeObject AioUringFileType;

typedef struct AioUringFile {
    PyObject_HEAD

        int fd;
    AioUringLoop *loop;
    bool closed;
} AioUringFile;

PyObject *
AioUring_open(PyObject *module, PyObject *args, PyObject *kwargs);

int
AioUringFile_traverse(AioUringFile *self, visitproc visit, void *arg);

int
AioUringFile_clear(AioUringFile *self);

PyObject *
AioUringFile_aenter(AioUringFile *self, PyObject *Py_UNUSED(ignored));

PyObject *
AioUringFile_aexit(AioUringFile *self, PyObject *args, PyObject *kwargs);

void
AioUringFile_dealloc(AioUringFile *self);

PyObject *
AioUringFile_read(AioUringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringFile_readv(AioUringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringFile_readv_raw(AioUringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringFile_write(AioUringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringFile_writev(AioUringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringFile_writev_raw(AioUringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringFile_close(AioUringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringFile_fsync(AioUringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringFile_fdatasync(AioUringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringFile_splice(AioUringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
_check_file_result(int result, AioUringFile *file, int request_idx, PyObject *future);

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
    AioUringFile *self,
    BufferPayload *buffer_payload,
    StreamStrategy stream,
    int request_idx,
    int size,
    int offset,
    TimeoutParams timeout_params
);

int
readv_dispatcher(
    AioUringFile *self,
    BufferPayload *buffer_payload,
    int request_idx,
    int offset,
    int nowait,
    TimeoutParams timeout_params
);

int
write_dispatcher(
    AioUringFile *self, BufferPayload *buffer_payload, int request_idx, int offset, TimeoutParams timeout_params
);
