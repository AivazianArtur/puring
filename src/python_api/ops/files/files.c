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

    UringFile *file = PyObject_New(UringFile, &UringFileType);
    if (!file) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create file");
        return NULL;
    }
    file->loop = self;
    Py_INCREF(self);

    int dfd = AT_FDCWD;
    PyObject *py_path_obj = NULL;
    const char *path = NULL;

    static char *kwlist[] = {"dfd", "path", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iO", kwlist, &dfd, &py_path_obj)) {
        Py_DECREF(file);
        PyErr_SetString(PyExc_RuntimeError, "Invalid arguments");
        return NULL;
    }

    if (!py_path_obj || !PyUnicode_Check(py_path_obj)) {
        Py_DECREF(file);
        PyErr_SetString(PyExc_TypeError, "Path must be a str");
        return NULL;
    }

    path = PyUnicode_AsUTF8(py_path_obj);
    if (!path) {
        Py_DECREF(file);
        PyErr_SetString(PyExc_TypeError, "Failed to convert path to UTF-8");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        Py_DECREF(file);
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_OPENAT2;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(
        self->registry,
        future,
        buffer,
        opcode,
        file,
        NULL
    );

    if (request_idx < 0) {
        Py_DECREF(file);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    if (open_file(self->ring, request_idx, dfd, path) < 0) {
        Py_DECREF(file);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }

    return future;
}


void 
UringFile_dealloc(UringFile *self)
{
    self->closed = true;
    if (self->loop) {
        Py_XDECREF(self->loop);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


PyObject*
UringFile_read(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop);
    if (self->loop->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "File is closed");
        return NULL;
    }

    int fd = 0;

    static char *kwlist[] = {"fd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &fd))) {
        PyErr_SetString(PyExc_RuntimeError, "Wrong input params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
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

    int request_idx = registry_add(
        self->loop->registry,
        future,
        buffer,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    char *buf = PyBytes_AS_STRING(buffer);

    if (uring_read(self->loop->ring, request_idx, fd, buf, (unsigned) size) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringFile_write(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop);
    if (self->loop->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "File is closed");
        return NULL;
    }

    int fd = 0;
    PyObject *data = NULL;

    static char *kwlist[] = {"fd", "data", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iO", kwlist, &fd, &data))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_WRITE;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(
        self->loop->registry, 
        future,
        data,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }
    
    char *buf = PyBytes_AS_STRING(data);
    Py_ssize_t size = PyBytes_GET_SIZE(data);

    if (uring_write(self->loop->ring, request_idx, fd, buf, (unsigned) size) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringFile_close(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop);
    if (self->loop->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "File is closed");
        return NULL;
    }

    int fd = 0;

    static char *kwlist[] = {"fd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &fd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
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
    int request_idx = registry_add(
        self->loop->registry,
        future,
        buffer,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    char *buf = PyBytes_AS_STRING(buffer);

    if (uring_close_file(self->loop->ring, request_idx, fd, buf) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringFile_stat(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop);
    if (self->loop->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "File is closed");
        return NULL;
    }

    int dfd;
    static char path;

    static char *kwlist[] = {"dfd", "path", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "is", kwlist, &dfd, &path))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
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
    int request_idx = registry_add(
        self->loop->registry,
        future,
        buffer,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    char *buf = PyBytes_AS_STRING(buffer);

    if (uring_stat(self->loop->ring, request_idx, dfd, &path, buf) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringFile_fsync(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop);
    if (self->loop->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "File is closed");
        return NULL;
    }

    int fd = 0;

    static char *kwlist[] = {"fd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &fd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_FSYNC;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(
        self->loop->registry,
        future,
        buffer,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (uring_fsync(self->loop->ring, request_idx, fd) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}
