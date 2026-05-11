#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <sys/types.h>
#include <stdbool.h>
#include "python_api/ops/files/files.h"
#include "python_api/ops/sockets/sockets.h"
#include "ring/ring.h"
#include "registry/registry.h"
#include "reader/reader.h"
#include "signals/signals.h"
#include "python_macroses.h"


extern PyTypeObject PuringLoopType;

typedef struct PuringLoop {
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
} PuringLoop;


PyObject*
PuringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);

int
PuringLoop_init(PuringLoop *self, PyObject *args, PyObject *kwargs);

void 
PuringLoop_dealloc(PuringLoop *self);

PyObject*
PuringLoop_close_loop(PuringLoop *self, PyObject *args);

PyObject*
PuringLoop_add_reader(PuringLoop *self, PyObject *args);

PyObject*
PuringLoop_remove_reader(PuringLoop *self, PyObject *args);

PyObject*
PuringLoop_add_writer(PuringLoop *self, PyObject *args);

PyObject*
PuringLoop_remove_writer(PuringLoop *self, PyObject *args);


PyObject*
PuringLoop_run_once(PyTypeObject *type, PyObject *args, PyObject *kwargs);

PyObject*
PuringLoop_write_to_self(PyTypeObject *type, PyObject *args, PyObject *kwargs);

PyObject*
PuringLoop_process_events(PyTypeObject *type, PyObject *args, PyObject *kwargs);

PyObject*
PuringLoop_make_socket_transport(PyTypeObject *type, PyObject *args, PyObject *kwargs);

PyObject*
PuringLoop_make_read_pipe_transport(PyTypeObject *type, PyObject *args, PyObject *kwargs);

PyObject*
PuringLoop_make_write_pipe_transport(PyTypeObject *type, PyObject *args, PyObject *kwargs);


// Helpers
PyObject* _get_loop(void);
int _parse_memory_params(PyObject *obj, memory_params *out);
void fast_shutdown(struct io_uring* ring, RequestRegistry *reg); 
void graceful_shutdown(struct io_uring* ring, RequestRegistry *reg);
