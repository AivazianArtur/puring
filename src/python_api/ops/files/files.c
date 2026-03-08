#include "files.h"


PyObject*
UringLoop_open(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int dfd = AT_FDCWD;
    PyObject *py_path_obj = NULL;
    const char *path = NULL;

    static char *kwlist[] = {"dfd", "path", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iO", kwlist, &dfd, &py_path_obj)) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid arguments");
        return NULL;
    }

    if (!py_path_obj || !PyUnicode_Check(py_path_obj)) {
        PyErr_SetString(PyExc_TypeError, "Path must be a str");
        return NULL;
    }

    path = PyUnicode_AsUTF8(py_path_obj);
    if (!path) {
        PyErr_SetString(PyExc_TypeError, "Failed to convert path to UTF-8");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_OPENAT2;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(self->registry, future, buffer, opcode, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    if (open_file(self->ring, request_idx, dfd, path) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }

    return future;
}


PyObject*
UringLoop_read(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int fd = 0;

    static char *kwlist[] = {"fd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &fd))) {
        PyErr_SetString(PyExc_RuntimeError, "Wrong input params\n");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_READ;

    Py_ssize_t size = 1024;

    PyObject *buffer = PyBytes_FromStringAndSize(NULL, size);
    if (!buffer) {
        Py_DECREF(future);
        return PyErr_NoMemory();
    }

    int request_idx = registry_add(self->registry, future, buffer, opcode, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    char *buf = PyBytes_AS_STRING(buffer);

    if (uring_read(self->ring, request_idx, fd, buf, (unsigned) size) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringLoop_write(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int fd = 0;
    PyObject *data = NULL;

    static char *kwlist[] = {"fd", "data", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iO", kwlist, &fd, &data))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_WRITE;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(self->registry, future, data, opcode, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }
    
    char *buf = PyBytes_AS_STRING(data);
    Py_ssize_t size = PyBytes_GET_SIZE(data);

    if (uring_write(self->ring, request_idx, fd, buf, (unsigned) size) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringLoop_close(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int fd = 0;

    static char *kwlist[] = {"fd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &fd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_CLOSE;
    
    Py_ssize_t size = 1024;

    PyObject *buffer = PyBytes_FromStringAndSize(NULL, size);
    if (!buffer) {
        Py_DECREF(future);
        return PyErr_NoMemory();
    }
    int request_idx = registry_add(self->registry, future, buffer, opcode, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    char *buf = PyBytes_AS_STRING(buffer);

    if (uring_close_file(self->ring, request_idx, fd, buf) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringLoop_stat(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int dfd;
    static char path;

    static char *kwlist[] = {"dfd", "path", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "is", kwlist, &dfd, &path))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    Py_ssize_t size = 1024;

    PyObject *buffer = PyBytes_FromStringAndSize(NULL, size);
    if (!buffer) {
        Py_DECREF(future);
        return PyErr_NoMemory();
    }

    int opcode = IORING_OP_STATX;
    int request_idx = registry_add(self->registry, future, buffer, opcode, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    char *buf = PyBytes_AS_STRING(buffer);

    if (uring_stat(self->ring, request_idx, dfd, &path, buf) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringLoop_fsync(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int fd = 0;

    static char *kwlist[] = {"fd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &fd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_FSYNC;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(self->registry, future, buffer, opcode, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (uring_fsync(self->ring, request_idx, fd) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}
