#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <stdbool.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include "execution_context.h"
#include "python_api/ops/files/files.h"
#include "python_api/ops/sockets/sockets.h"
#include "python_macroses.h"
#include "reader/reader.h"
#include "registry/registry.h"
#include "ring/ring.h"
#include "signals/signals.h"

extern PyTypeObject *AioUringLoopType;

typedef struct AioUringLoop {
    PyObject_HEAD

        struct io_uring *ring;
    pid_t loop_tid;
    int wakeup_fd;
    uint64_t wakeup_buf;

    RequestRegistry *registry;
    unsigned int entries;

    PyObject *readers;
    PyObject *writers;

    PyObject *execution_context_var;

    bool initialized;
} AioUringLoop;

PyObject *
AioUringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);

int
AioUringLoop_init(
    AioUringLoop *self
    // PyObject *args,
    // PyObject *kwargs
);


int
AioUringLoop_traverse(AioUringLoop *self, visitproc visit, void *arg);

int
AioUringLoop_clear(AioUringLoop *self);

void
AioUringLoop_dealloc(AioUringLoop *self);

// PyObject*
// AioUringLoop_close_loop(AioUringLoop *self, PyObject *args);

PyObject *
AioUringLoop_close(AioUringLoop *self, PyObject *Py_UNUSED(ignored));

PyObject *
AioUringLoop_run_once(AioUringLoop *self);

PyObject *
AioUringLoop_write_to_self(AioUringLoop *self);

// Shutdown
void
fast_shutdown(struct io_uring *ring, RequestRegistry *reg);
void
graceful_shutdown(struct io_uring *ring, RequestRegistry *reg);

// Helpers
struct __kernel_timespec
compute_timeout(AioUringLoop *self);
void
promote_scheduled(AioUringLoop *self);
void
drain_ready(AioUringLoop *self);

// Future
PyObject *
create_future(AioUringLoop *self);
