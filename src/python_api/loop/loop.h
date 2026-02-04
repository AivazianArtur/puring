#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <sys/types.h>
#include <stdbool.h>

#include "future.h"
#include "ops/files/files.h"
#include "ops/sockets/sockets.h"
#include "core/core.h"

typedef struct RequestRegistry RequestRegistry;

/* Objects */

typedef struct {
    // Now its basicaly a driver, but in next versions it will be loop
    PyObject_HEAD
    struct io_uring ring;
    PyObject *py_loop;
    pid_t loop_tid;
    RequestRegistry *registry;
    unsigned int entries;
    bool initialized;
    bool closing;
} UringLoop;


/* Functions */
static PyObject*
UringLoop_new(
    PyTypeObject *type,
    PyObject *args,
    PyObject *kwargs,
);

static int
UringLoop_init(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs,
);

static void 
UringLoop_dealloc(UringLoop *self);

// TODO in next versions
// static PyObject*
// UringLoop_close_loop(UringLoop *self, PyObject *args);

// static PyObject*
// UringLoop_run_forever(UringLoop *self, PyObject *args);

// static PyObject*
// UringLoop_stop(UringLoop *self, PyObject *args);

// static PyObject*
// UringLoop_call_soon(UringLoop *self, PyObject *args);

// static PyObject*
// UringLoop_call_later(UringLoop *self, PyObject *args);


// Helpers
PyObject* _get_loop(void);
int _parse_memory_params(PyObject *obj, memory_params *out);
int _parse_ring_init_params(PyObject *obj, ring_initialization_params *out);
