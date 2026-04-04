#include "loop.h"


PyObject* 
_get_loop(void)
{
    PyObject *asyncio = PyImport_ImportModule("asyncio");
    if (!asyncio) {
        PyObject* err_msg = PyUnicode_FromString("Can`t import asyncio");
        PyObject* module_name = PyUnicode_FromString("asyncio");
        PyErr_SetImportError(
            err_msg,
            module_name,
            NULL
        );
        Py_DECREF(err_msg);
        Py_DECREF(module_name);
        return NULL;
    }
    PyObject *get_loop_fn = PyObject_GetAttrString(asyncio, "get_event_loop");
    PyObject *loop = PyObject_CallNoArgs(get_loop_fn);

    Py_DECREF(get_loop_fn);
    Py_DECREF(asyncio);

    if (!loop) {
        PyErr_SetString(PyExc_RuntimeError, "No active event loop");
    }
    return loop;
}


void fast_shutdown(struct io_uring* ring, RequestRegistry *reg) 
{
    ring_destroy(ring);
    registry_destroy(reg);
}


void graceful_shutdown(struct io_uring* ring, RequestRegistry *reg)
{
    ring_destroy(ring);
    registry_destroy(reg);

    struct io_uring_cqe *cqe;  // TEMP
    struct __kernel_timespec ts;
    ts.tv_nsec = 0;
    ts.tv_sec = 3;

    while (io_uring_wait_cqe_timeout(ring, &cqe, &ts) == 0) {
        io_uring_cqe_seen(ring, cqe);
    }
}
