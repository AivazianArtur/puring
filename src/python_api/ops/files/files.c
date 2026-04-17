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
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->py_loop
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

    const char *path = NULL;
    int dfd = AT_FDCWD;
    PyObject *py_path_obj = NULL;
    PyObject *timeout_params_obj = NULL;
    int flags = 0;
    int resolve = 0;
    int mode = 0644;

    static char *kwlist[] = {"path", "dfd", "timeout_params", "flags", "resolve", "mode", NULL};
    if (!PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "O|iOKKK",
        kwlist,
        &py_path_obj,
        &dfd,
        &timeout_params_obj,
        &flags,
        &resolve,
        &mode
    )) {
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

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

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
        NULL,
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

    int result = open_file(
        self->ring,
        request_idx,
        dfd,
        path,
        flags,
        resolve,
        mode,
        &timeout_params
    );
    if (result == -1) {
        Py_DECREF(file);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(file);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
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
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *timeout_params_obj = NULL;
    int offset = 0;
    static char *kwlist[] = {"offset", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iO", kwlist, &offset, &timeout_params_obj)) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);
    
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
        NULL,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    // char *buf = PyBytes_AS_STRING(buffer);
    // if (!buf) {
    //    Py_DECREF(future);
    //    registry_remove(self->loop->registry, request_idx);
    //    PyErr_SetString(PyExc_TypeError, "Data in buffer is not byte objects");
    //    return NULL;
    // }

    int result = uring_read(
        self->loop->ring,
        request_idx,
        self->fd,
        buffer,
        (unsigned) size,
        offset,
        &timeout_params
    );
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }
    return future;
}


PyObject*
UringFile_readv(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *buffers_obj;
    PyObject *timeout_params_obj = NULL;
    int flags = 0;
    int offset = 0;
    static char *kwlist[] = {"buffer", "offset", "flags", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iiO", kwlist, &buffers_obj, &offset, &flags, &timeout_params_obj)) {
        return NULL;
    }
    
    // Start Buff
    PyObject *seq = PySequence_Fast(buffers_obj, "Buffers must be a sequence");
    if (!seq) {
        return NULL;
    }
    Py_ssize_t nr_vecs = PySequence_Fast_GET_SIZE(seq);
    PyObject **items = PySequence_Fast_ITEMS(seq);

    struct iovec *iovecs = PyMem_Malloc(sizeof(struct iovec) * nr_vecs);
    if (!iovecs) {
        Py_DECREF(seq);
        return PyErr_NoMemory();
    }

    Py_buffer *iovecs_buf = PyMem_Malloc(sizeof(Py_buffer) * nr_vecs);

    for (Py_ssize_t i = 0; i < nr_vecs; i++) {
        if (PyObject_GetBuffer(items[i], &iovecs_buf[i], PyBUF_SIMPLE) < 0) {
            for (Py_ssize_t j = 0; j < i; j++) {
                PyBuffer_Release(&iovecs_buf[j]);
            }
            PyMem_Free(iovecs_buf);
            PyMem_Free(iovecs);
            Py_DECREF(seq);
            return NULL;
        }

        iovecs[i].iov_base = iovecs_buf[i].buf;
        iovecs[i].iov_len  = iovecs_buf[i].len;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);
    
    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }
    int opcode = IORING_OP_READV;

    int request_idx = registry_add(
        self->loop->registry,
        future,
        NULL,
        iovecs_buf,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_readv(
        self->loop->ring,
        request_idx,
        self->fd,
        iovecs,
        (unsigned) nr_vecs,
        offset,
        flags,
        &timeout_params
    );
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }
    return future;
}


PyObject*
UringFile_readv_raw(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    Py_buffer iovecs_buf;
    PyObject *timeout_params_obj = NULL;
    int flags = 0;
    int offset = 0;
    static char *kwlist[] = {"iovecs", "offset", "flags", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "w*|iiO", kwlist, &iovecs_buf, &offset, &flags, &timeout_params_obj)) {
        return NULL;
    }
    
    if (iovecs_buf.len % sizeof(struct iovec) != 0) {
        PyBuffer_Release(&iovecs_buf);
        PyErr_SetString(PyExc_ValueError, "iovecs buffer has invalid size");
        return NULL;
    }

    if (!PyBuffer_IsContiguous(&iovecs_buf, 'C')) {
        PyErr_SetString(PyExc_ValueError, "iovecs must be contiguous");
        return NULL;
    }

    struct iovec *iovecs = (struct iovec *)iovecs_buf.buf;
    unsigned nr_vecs = iovecs_buf.len / sizeof(struct iovec);

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);
    
    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }
    int opcode = IORING_OP_READV;

    int request_idx = registry_add(
        self->loop->registry,
        future,
        NULL,
        &iovecs_buf,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_readv(
        self->loop->ring,
        request_idx,
        self->fd,
        iovecs,
        (unsigned) nr_vecs,
        offset,
        flags,
        &timeout_params
    );
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );

        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *data = NULL;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static char *kwlist[] = {"data", "offset", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "O|iO", kwlist, &data, &offset, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_WRITE;
    int request_idx = registry_add(
        self->loop->registry, 
        future,
        data,
        NULL,
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
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_TypeError, "Data in buffer is not byte objects");
        return NULL;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(data);

    int result = uring_write(
        self->loop->ring,
        request_idx,
        self->fd,
        buf,
        (unsigned) size,
        offset,
        &timeout_params
    );
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }

    return future;
}


PyObject*
UringFile_writev(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );

        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *data = NULL;
    PyObject *buffers_obj;

    int flags = 0;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static char *kwlist[] = {"data", "buffers", "flags", "offset", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "OO|iiO", kwlist, &data, &buffers_obj, &flags, &offset, &timeout_params_obj))) {
        return NULL;
    }

    // Start Buff
    PyObject *seq = PySequence_Fast(buffers_obj, "Buffers must be a sequence");
    if (!seq) {
        return NULL;
    }
    Py_ssize_t nr_vecs = PySequence_Fast_GET_SIZE(seq);
    PyObject **items = PySequence_Fast_ITEMS(seq);

    struct iovec *iovecs = PyMem_Malloc(sizeof(struct iovec) * nr_vecs);
    if (!iovecs) {
        Py_DECREF(seq);
        return PyErr_NoMemory();
    }

    Py_buffer *iovecs_buf = PyMem_Malloc(sizeof(Py_buffer) * nr_vecs);

    for (Py_ssize_t i = 0; i < nr_vecs; i++) {
        if (PyObject_GetBuffer(items[i], &iovecs_buf[i], PyBUF_SIMPLE) < 0) {
            for (Py_ssize_t j = 0; j < i; j++) {
                PyBuffer_Release(&iovecs_buf[j]);
            }
            PyMem_Free(iovecs_buf);
            PyMem_Free(iovecs);
            Py_DECREF(seq);
            return NULL;
        }

        iovecs[i].iov_base = iovecs_buf[i].buf;
        iovecs[i].iov_len  = iovecs_buf[i].len;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_WRITEV;
    int request_idx = registry_add(
        self->loop->registry, 
        future,
        data,
        iovecs_buf,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    
    int result = uring_writev(
        self->loop->ring,
        request_idx,
        self->fd,
        iovecs,
        (unsigned) nr_vecs,
        offset,
        &timeout_params
    );
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }

    return future;
}


PyObject*
UringFile_writev_raw(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );

        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *data = NULL;
    Py_buffer iovecs_buf;

    int flags = 0;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static char *kwlist[] = {"data", "buffers", "flags", "offset", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "Oy|iiO", kwlist, &data, &iovecs_buf, &flags, &offset, &timeout_params_obj))) {
        return NULL;
    }

    if (iovecs_buf.len % sizeof(struct iovec) != 0) {
        PyBuffer_Release(&iovecs_buf);
        PyErr_SetString(PyExc_ValueError, "iovecs buffer has invalid size");
        return NULL;
    }

    if (!PyBuffer_IsContiguous(&iovecs_buf, 'C')) {
        PyBuffer_Release(&iovecs_buf);
        PyErr_SetString(PyExc_ValueError, "iovecs must be contiguous");
        return NULL;
    } 

    struct iovec *iovecs = (struct iovec *)iovecs_buf.buf;
    unsigned nr_vecs = iovecs_buf.len / sizeof(struct iovec);

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_WRITEV;
    int request_idx = registry_add(
        self->loop->registry, 
        future,
        data,
        &iovecs_buf,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    
    int result = uring_writev(
        self->loop->ring,
        request_idx,
        self->fd,
        iovecs,
        (unsigned) nr_vecs,
        offset,
        &timeout_params
    );
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
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
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &timeout_params_obj)) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

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
        NULL,
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
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_TypeError, "Data in buffer is not byte objects");
        return NULL;
    }
    int result = uring_close_file(self->loop->ring, request_idx, self->fd, buf, &timeout_params);
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
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
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    static char path;
    int flags = 0;
    unsigned mask = 0;
    PyObject *timeout_params_obj = NULL;

    static char *kwlist[] = {"path", "timeout_params", "flags", "mask", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "s|OiI", kwlist, &path, &timeout_params_obj, &flags, &mask))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

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
        NULL,
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
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_TypeError, "Data in buffer is not byte objects");
        return NULL;
    }

    int result = uring_stat(
        self->loop->ring,
        request_idx,
        self->fd,
        &path,
        buf,
        flags,
        mask,
        &timeout_params
    );
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
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
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &timeout_params_obj)) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

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
        NULL,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_fsync(self->loop->ring, request_idx, self->fd, &timeout_params);
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }
    return future;
}


PyObject*
UringFile_fdatasync(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &timeout_params_obj)) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

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
        NULL,
        opcode,
        self,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_fdatasync(self->loop->ring, request_idx, self->fd, &timeout_params);
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }
    return future;
}


PyObject*
UringFile_splice(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{

}