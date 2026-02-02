#include "files.h"


static PyObject*
UringLoop_open(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int dfd = NULL;
    static char path = NULL;

    static char *kwlist[] = {"dfd", "path", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "is", kwlist, dfd, path))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_OPENAT2;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (open_file(self->ring, request_idx, dfd, path) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringLoop_read(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int fd = NULL;

    static char *kwlist[] = {"fd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, fd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        return NULL;
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
    }

    int opcode = IORING_OP_READ;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (read(self->ring, request_idx, fd, buffer) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringLoop_write(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int fd = NULL;

    static char *kwlist[] = {"fd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, fd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_WRITE;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (write(self->ring, request_idx, fd, buffer) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringLoop_close(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int fd = NULL;

    static char *kwlist[] = {"fd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, fd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_CLOSE;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (close(self->ring, request_idx, fd) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringLoop_stat(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int dfd = NULL;
    static char path = NULL;

    static char *kwlist[] = {"dfd", "path", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "is", kwlist, dfd, path))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_STATX;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (stat(self->ring, request_idx, dfd, path) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringLoop_fsync(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int fd = NULL;

    static char *kwlist[] = {"fd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, fd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_FSYNC;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (fsync(self->ring, request_idx, fd) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}
