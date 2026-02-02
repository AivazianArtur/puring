#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <sys/types.h>
#include <stdbool.h>

#include "future.h"
#include "ops/files/files.h"
#include "ops/sockets/sockets.h"


typedef struct RequestRegistry RequestRegistry;

/* Objects */

typedef struct {
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
static PyObject *UringLoop_new(
    PyTypeObject *type,
    PyObject *args,
    PyObject *kwargs,
);

static int UringLoop_init(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs,
);

static void UringLoop_dealloc(UringLoop *self);

PyObject *UringLoop_close_loop(UringLoop *self, PyObject *args);

// Helpers
PyObject* _set_loop(void);
int _parse_memory_params(PyObject *obj, memory_params *out);
int _parse_ring_init_params(PyObject *obj, ring_initialization_params *out);
