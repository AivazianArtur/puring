#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <sys/types.h>
#include <stdbool.h>
#include "python_api/ops/files/files.h"
#include "python_api/ops/sockets/sockets.h"
#include "core/core.h"
#include "reader/reader.h"
#include "macroses/macroses.h"


// typedef struct RequestRegistry RequestRegistry;

/* Objects */

typedef struct UringLoop {
    // Now its basicaly a driver, but in next versions it will be loop
    PyObject_HEAD
    struct io_uring *ring;
    PyObject *py_loop;
    pid_t loop_tid;
    RequestRegistry *registry;
    unsigned int entries;

    PyObject *reader_capsule;
    PyObject *reader_callback;
    bool is_reader_installed;

    bool initialized;
    bool is_closing;
} UringLoop;

extern PyTypeObject UringLoopType;
extern PyTypeObject UringSocketType;

/* Functions */
static PyObject*
UringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);

static int
UringLoop_init(UringLoop *self, PyObject *args, PyObject *kwargs);

static void 
UringLoop_dealloc(UringLoop *self);

static PyObject*
UringLoop_close_loop(UringLoop *self, PyObject *args);

static PyObject *
py_uring_loop_register_fd(PyObject *self, PyObject *args);

// TODO in next versions
// static PyObject*
// UringLoop_get_loop(UringLoop *self, PyObject *args);

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
int _parse_ring_init_params(PyObject *obj, ring_init_params *out);
void fast_shutdown(struct io_uring* ring, RequestRegistry *reg); 
void graceful_shutdown(struct io_uring* ring, RequestRegistry *reg);
int _parse_sq_offset(PyObject *obj, struct io_sqring_offsets *out);
int _parse_cq_offset(PyObject *obj, struct io_cqring_offsets *out);
