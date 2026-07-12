#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

#include <liburing.h>

#include "ops/files/files.h"
#include "registry/registry.h"
#include "timer/timer.h"
#include <linux/openat2.h>

#include "python_api/future/future.h"
#include "python_api/loop/loop.h"
#include "python_api/timer/timer.h"
#include "python_macroses.h"

extern PyTypeObject *PuringLoopType;
extern PyTypeObject PuringFileType;

typedef struct PuringFile {
    PyObject_HEAD

        int fd;
    PuringLoop *loop;
    bool closed;
} PuringFile;

PyObject *
Puring_open(PyObject *module, PyObject *args, PyObject *kwargs);

void
PuringFile_dealloc(PuringFile *self);

PyObject *
PuringFile_read(PuringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringFile_readv(PuringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringFile_readv_raw(PuringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringFile_write(PuringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringFile_writev(PuringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringFile_writev_raw(PuringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringFile_close(PuringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringFile_fsync(PuringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringFile_fdatasync(PuringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringFile_splice(PuringFile *self, PyObject *args, PyObject *kwargs);

PyObject *
_check_file_result(int result, PuringFile *file, int request_idx, PyObject *future);

PyObject *
create_resolve_enum(void);
PyObject *
create_statx_flags_enum(void);
PyObject *
create_statx_mask_enum(void);
PyObject *
create_transfer_mode_enum(void);

int
read_dispatcher(
    PuringFile *self, BufferPayload *buffer_payload, int request_idx, int size, int offset, TimeoutParams timeout_params
);

int
readv_dispatcher(
    PuringFile *self,
    BufferPayload *buffer_payload,
    int request_idx,
    int offset,
    int nowait,
    TimeoutParams timeout_params
);

int
write_dispatcher(
    PuringFile *self, BufferPayload *buffer_payload, int request_idx, int offset, TimeoutParams timeout_params
);
