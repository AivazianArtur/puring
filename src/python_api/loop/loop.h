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

extern PyTypeObject *PuringLoopType;

typedef struct PuringLoop {
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
} PuringLoop;

PyObject *
PuringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);

int
PuringLoop_init(
    PuringLoop *self
    // PyObject *args,
    // PyObject *kwargs
);


int
PuringLoop_traverse(PuringLoop *self, visitproc visit, void *arg);

int
PuringLoop_clear(PuringLoop *self);

void
PuringLoop_dealloc(PuringLoop *self);

// PyObject*
// PuringLoop_close_loop(PuringLoop *self, PyObject *args);

PyObject *
PuringLoop_close(PuringLoop *self, PyObject *Py_UNUSED(ignored));

PyObject *
PuringLoop_run_once(PuringLoop *self);

PyObject *
PuringLoop_write_to_self(PuringLoop *self);

// Helpers
void
fast_shutdown(struct io_uring *ring, RequestRegistry *reg);
void
graceful_shutdown(struct io_uring *ring, RequestRegistry *reg);

struct __kernel_timespec
compute_timeout(PuringLoop *self);
void
promote_scheduled(PuringLoop *self);
void
drain_ready(PuringLoop *self);
