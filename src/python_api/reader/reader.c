#include "reader.h"


static PyObject *
py_on_uring_ready(PyObject *capsule)
{
    UringLoop *loop = PyCapsule_GetPointer(capsule, "uring_loop");
    if (!loop) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get loop from capsule");
        return NULL;
    }

    on_uring_ready(loop);
    Py_RETURN_NONE;
}

static PyMethodDef py_uring_reader_cb_def = {
    "on_uring_ready",
    (PyCFunction)py_on_uring_ready,
    METH_O,
    "Called by asyncio when io_uring FD is ready"
};


void uring_loop_register_fd(UringLoop *loop)
{
    if (loop->is_reader_installed)
        return;

    loop->reader_capsule = PyCapsule_New(loop, "uring_loop", NULL);
    if (!loop->reader_capsule) return;

    loop->reader_callback = PyCFunction_New(&py_uring_reader_cb_def, loop->reader_capsule);
    if (!loop->reader_callback) {
        Py_DECREF(loop->reader_capsule);
        loop->reader_capsule = NULL;
        return;
    }

    Py_INCREF(loop->reader_capsule);
    Py_INCREF(loop->reader_callback);

    int uring_fd = loop->ring->ring_fd;

    PyObject *res = PyObject_CallMethod(
        loop->py_loop,
        "add_reader",
        "iOO",
        uring_fd,
        loop->reader_callback,
        loop->reader_capsule
    );

    if (!res) {
        PyErr_Print();
    } else {
        Py_DECREF(res);
    }

    loop->is_reader_installed = true;
}