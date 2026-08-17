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

extern PyTypeObject *UringioLoopType;

typedef struct UringioLoop {
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
} UringioLoop;

PyObject *
UringioLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);

int
UringioLoop_init(
    UringioLoop *self
    // PyObject *args,
    // PyObject *kwargs
);


int
UringioLoop_traverse(UringioLoop *self, visitproc visit, void *arg);

int
UringioLoop_clear(UringioLoop *self);

void
UringioLoop_dealloc(UringioLoop *self);

// PyObject*
// UringioLoop_close_loop(UringioLoop *self, PyObject *args);

PyObject *
UringioLoop_close(UringioLoop *self, PyObject *Py_UNUSED(ignored));

PyObject *
UringioLoop_run_once(UringioLoop *self);

PyObject *
UringioLoop_write_to_self(UringioLoop *self);

// Shutdown
void
fast_shutdown(struct io_uring *ring, RequestRegistry *reg);
void
graceful_shutdown(struct io_uring *ring, RequestRegistry *reg);

// Helpers
struct __kernel_timespec
compute_timeout(UringioLoop *self);
void
promote_scheduled(UringioLoop *self);
void
drain_ready(UringioLoop *self);

// Future
PyObject *
create_future(UringioLoop *self);
