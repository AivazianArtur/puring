#include "files.h"


PyObject*
UringLoop_open(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->py_loop);
    if (self->is_closing) {
        PyErr_SetString(
            PyExc_RuntimeError,
            PyUnicode_FromFormat("Ring Event Loop is closing - %S", self->py_loop)
        );
        return NULL;
    }

    UringFile *file = PyObject_New(UringFile, &UringFileType);
    if (!file) {
        return PyErr_NoMemory();
    }

    file->loop = self;
    file->closed = false;
    Py_INCREF(self);

    int dfd = AT_FDCWD;
    PyObject *py_path_obj = NULL;
    const char *path = NULL;

    static char *kwlist[] = {"path", "dfd", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i", kwlist, &py_path_obj, &dfd)) {
        Py_DECREF(file);
        return NULL;
    }

    if (!PyUnicode_Check(py_path_obj)) {
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

    int result = open_file(self->ring, request_idx, dfd, path);
    if (result == -1) {
        Py_DECREF(file);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_SetString(
            PyExc_RuntimeError,
            PyUnicode_FromFormat("Ring Event Loop is closing - %S", self->loop->py_loop)
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_READ;

    Py_ssize_t size = 1024;

    PyObject *buffer = PyBytes_FromStringAndSize(NULL, size);
    if (!buffer) {
        Py_DECREF(future);
        return NULL;
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
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    char *buf = PyBytes_AS_STRING(buffer);
    if (!buf) {
        Py_DECREF(future);
        PyErr_SetString(PyErr_BadArgument, "Data in buffer is not byte objects");
        return NULL;
    }

    int result = uring_read(self->loop->ring, request_idx, self->fd, buf, (unsigned) size);
    if (result < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_SetString(
            PyExc_RuntimeError,
            PyUnicode_FromFormat("Ring Event Loop is closing - %S", self->loop->py_loop)
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *data = NULL;

    static char *kwlist[] = {"data", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &data))) {
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
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
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    
    char *buf = PyBytes_AS_STRING(data);
    if (!buf) {
        Py_DECREF(future);
        PyErr_SetString(PyErr_BadArgument, "Data in buffer is not byte objects");
        return NULL;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(data);

    int result = uring_write(self->loop->ring, request_idx, self->fd, buf, (unsigned) size);
    if (result < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_SetString(
            PyExc_RuntimeError,
            PyUnicode_FromFormat("Ring Event Loop is closing - %S", self->loop->py_loop)
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
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
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    char *buf = PyBytes_AS_STRING(buffer);
    if (!buf) {
        Py_DECREF(future);
        PyErr_SetString(PyErr_BadArgument, "Data in buffer is not byte objects");
        return NULL;
    }
    int result = uring_close_file(self->loop->ring, request_idx, self->fd, buf);
    if (result < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_SetString(
            PyExc_RuntimeError,
            PyUnicode_FromFormat("Ring Event Loop is closing - %S", self->loop->py_loop)
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    static char path;

    static char *kwlist[] = {"path", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &path))) {
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
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
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    char *buf = PyBytes_AS_STRING(buffer);
    if (!buf) {
        Py_DECREF(future);
        PyErr_SetString(PyErr_BadArgument, "Data in buffer is not byte objects");
        return NULL;
    }

    int result = uring_stat(self->loop->ring, request_idx, self->fd, &path, buf);
    if (result < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_SetString(
            PyExc_RuntimeError,
            PyUnicode_FromFormat("Ring Event Loop is closing - %S", self->loop->py_loop)
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
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
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_fsync(self->loop->ring, request_idx, self->fd);
    if (result < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }
    return future;
}
