#include "files.h"


PyObject*
UringLoop_open(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self);

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

    static char *kwlist[] = {"path", "dirfd", "flags", "resolve", "mode", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "O|iKKKO",
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
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *timeout_params_obj = NULL;
    int offset = 0;
    int size_i = 1024;
    static char *kwlist[] = {"offset", "size", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iiO", kwlist, &offset, &size_i, &timeout_params_obj)) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);
    
    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_READ;

    Py_ssize_t size = (Py_ssize_t)size_i;

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

    char *buf = PyBytes_AS_STRING(buffer);
    if (!buf) {
       Py_DECREF(future);
       registry_remove(self->loop->registry, request_idx);
       PyErr_SetString(PyExc_TypeError, "Data in buffer is not byte objects");
       return NULL;
    }

    int result = uring_read(
        self->loop->ring,
        request_idx,
        self->fd,
        buf,
        (unsigned) size,
        offset,
        &timeout_params
    );

    return _check_result(result, self, request_idx, future);
}


PyObject*
UringFile_readv(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *buffers_obj;
    PyObject *timeout_params_obj = NULL;
    int flags = 0;
    int offset = 0;
    static char *kwlist[] = {"buffers", "offset", "flags", "timeout_params", NULL};
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

    return _check_result(result, self, request_idx, future);
}


PyObject*
UringFile_readv_raw(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
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

    return _check_result(result, self, request_idx, future);
}


PyObject*
UringFile_write(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
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

    return _check_result(result, self, request_idx, future);
}


PyObject*
UringFile_writev(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *buffers_obj;

    int flags = 0;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static char *kwlist[] = {"buffers", "flags", "offset", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "OO|iiO", kwlist, &buffers_obj, &flags, &offset, &timeout_params_obj))) {
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
    
    int result = uring_writev(
        self->loop->ring,
        request_idx,
        self->fd,
        iovecs,
        (unsigned) nr_vecs,
        offset,
        flags,
        &timeout_params
    );

    return _check_result(result, self, request_idx, future);
}


PyObject*
UringFile_writev_raw(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    Py_buffer iovecs_buf;

    int flags = 0;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static char *kwlist[] = {"buffers", "flags", "offset", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "Oy|iiO", kwlist, &iovecs_buf, &flags, &offset, &timeout_params_obj))) {
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
    
    int result = uring_writev(
        self->loop->ring,
        request_idx,
        self->fd,
        iovecs,
        (unsigned) nr_vecs,
        offset,
        flags,
        &timeout_params
    );

    return _check_result(result, self, request_idx, future);
}


PyObject*
UringFile_close(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
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

    return _check_result(result, self, request_idx, future);
}


PyObject*
UringFile_fsync(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
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
    return _check_result(result, self, request_idx, future);
}


PyObject*
UringFile_fdatasync(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
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
    return _check_result(result, self, request_idx, future);
}


PyObject*
UringFile_splice(
    UringFile *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    int src = 0;
    int dst = 0;
    int count = 0;
    int offset_src = 0;
    int offset_dst = 0;
    int flag = 0;

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"src", "dst", "count", "offset_src", "offset_dst", "flag", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "iii|iiiO",
        kwlist,
        &src,
        &dst,
        &count,
        &offset_src,
        &offset_dst,
        &flag,
        &timeout_params_obj
    )) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_SPLICE;
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

    int result = uring_splice(
        self->loop->ring,
        request_idx,
        src,
        offset_src,
        dst,
        offset_dst,
        count,
        flag,
        &timeout_params
    );
    return _check_result(result, self, request_idx, future);
}

static PyObject* _check_result(int result, UringFile *file, int request_idx, PyObject *future){
    if (result < 0) {
        Py_DECREF(future);
        registry_remove(file->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(file->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }
    return future;
}
