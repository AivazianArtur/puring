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


int uring_loop_register_fd(UringLoop *loop)
{
    loop->reader_capsule = PyCapsule_New(loop, "uring_loop", NULL);
    if (!loop->reader_capsule) {
        PyErr_SetString(PyExc_RuntimeError, "No capsule to handle CQ ring.");
        return -1;
    }
    loop->reader_callback = PyCFunction_New(&py_uring_reader_cb_def, loop->reader_capsule);
    if (!loop->reader_callback) {
        Py_DECREF(loop->reader_capsule);
        loop->reader_capsule = NULL;
        PyErr_SetString(PyExc_RuntimeError, "Can`t execute function inside capsule");
        return -1;
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
        PyErr_SetString(PyExc_RuntimeError, "Reader not added");
        return -1;
    }

    Py_DECREF(res);
    loop->is_reader_installed = true;
    return 1;
}