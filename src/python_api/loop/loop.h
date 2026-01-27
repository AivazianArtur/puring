#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <sys/types.h>
#include <stdbool.h>

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
PyObject *UringLoop_new(PyTypeObject *, PyObject *, PyObject *);
int UringLoop_init(UringLoop *, PyObject *, PyObject *);
void UringLoop_dealloc(UringLoop *);

PyObject *UringLoop_run(UringLoop *, PyObject *);
PyObject *UringLoop_stop(UringLoop *, PyObject *);
PyObject *UringLoop_close(UringLoop *, PyObject *);

// Helpers
PyObject* _set_loop(void);
int _parse_memory_params(PyObject *obj, memory_params *out);
int _parse_ring_init_params(PyObject *obj, ring_initialization_params *out);
