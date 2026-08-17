#include "python_api/ops/files/files.h"

PyObject *
Uringio_open(PyObject *Py_UNUSED(module), PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_IS_URINGIO();
    ASSERT_LOOP_THREAD(running_loop);
    ASSERT_RING_LOOP_IS_CLOSING(running_loop);

    UringioFile *file = PyObject_GC_New(UringioFile, &UringioFileType);
    if (!file) {
        return PyErr_NoMemory();
    }

    file->loop = running_loop;
    file->closed = false;
    Py_INCREF(running_loop);
    PyObject_GC_Track(file);

    const char *path = NULL;
    int dfd = AT_FDCWD;
    PyObject *path_obj = NULL;
    PyObject *timeout_params_obj = NULL;
    int flags = -1;
    int resolve = 0;
    int mode = 0644;

    static const char *kwlist[] = {"path", "dirfd", "flags", "resolve", "mode", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "O|iiiiO", (char **)kwlist, &path_obj, &dfd, &flags, &resolve, &mode, &timeout_params_obj
        )) {
        Py_DECREF(file);
        return NULL;
    }

    PyObject *decoded_path = PyOS_FSPath(path_obj);
    if (decoded_path == NULL) {
        Py_DECREF(file);
        return NULL;
    }

    path = PyUnicode_AsUTF8(decoded_path);
    Py_DECREF(decoded_path);
    if (!path) {
        Py_DECREF(file);
        PyErr_SetString(PyExc_TypeError, "Failed to convert path to UTF-8");
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0) {
        Py_DECREF(file);
        return NULL;
    }

    PyObject *future = create_future(running_loop);
    if (!future) {
        Py_DECREF(file);
        return NULL;
    }

    int opcode = IORING_OP_OPENAT2;
    int request_idx = registry_add(running_loop->registry, future, NULL, ONESHOT, opcode, file, NULL, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(file);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = open_file(running_loop->ring, request_idx, dfd, path, flags, resolve, (mode_t)mode, timeout_params);
    if (result == -1) {
        Py_DECREF(file);
        Py_DECREF(future);
        registry_remove(running_loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(file);
        Py_DECREF(future);
        registry_remove(running_loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }

    return future;
}

int
UringioFile_traverse(UringioFile *self, visitproc visit, void *arg) {
    Py_VISIT(self->loop);
    return 0;
}

int
UringioFile_clear(UringioFile *self) {
    Py_CLEAR(self->loop);
    return 0;
}

PyObject *
UringioFile_aenter(UringioFile *self, PyObject *Py_UNUSED(ignored)) {
    PyObject *future = create_future(self->loop);
    if (!future)
        return NULL;

    PyObject *res_call = PyObject_CallMethod(future, "set_result", "O", self);
    Py_XDECREF(res_call);
    return future;
}

PyObject *
UringioFile_aexit(UringioFile *self, PyObject *args, PyObject *kwargs) {
    PyObject *exc_type = NULL;
    PyObject *exc_val = NULL;
    PyObject *exc_tb = NULL;

    static const char *kwlist[] = {"exc_type", "exc_val", "exc_tb", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO", (char **)kwlist, &exc_type, &exc_val, &exc_tb))
        return NULL;

    int had_exception = (exc_type != Py_None);

    TimeoutParams timeout_params = {0};
    PyObject *future = create_future(self->loop);
    if (!future) {
        if (had_exception) {
            return _raise_file_exception_group(exc_type, exc_val, exc_tb);
        }
        return NULL;
    }

    int opcode = IORING_OP_CLOSE;
    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, self, NULL, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        if (had_exception) {
            return _raise_file_exception_group(exc_type, exc_val, exc_tb);
        }
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    self->closed = 1;

    int result = uringio_close_file(self->loop->ring, request_idx, self->fd, timeout_params);
    PyObject *validated_result = _check_file_result(result, self, request_idx, future);
    if (!validated_result) {
        self->closed = 0;
        if (had_exception) {
            return _raise_file_exception_group(exc_type, exc_val, exc_tb);
        }
        return NULL;
    }
    return validated_result;
}

void
UringioFile_dealloc(UringioFile *self) {
    PyObject_GC_UnTrack(self);
    self->closed = true;
    UringioFile_clear(self);
    freefunc free_func = PyType_GetSlot(Py_TYPE(self), Py_tp_free);
    if (free_func) {
        free_func(self);
    } else {
        PyObject_GC_Del(self);
    }
}

PyObject *
UringioFile_close(UringioFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", (char **)kwlist, &timeout_params_obj)) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future)
        return NULL;

    int opcode = IORING_OP_CLOSE;

    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, self, NULL, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    int result = uringio_close_file(self->loop->ring, request_idx, self->fd, timeout_params);

    return _check_file_result(result, self, request_idx, future);
}

PyObject *
UringioFile_read(UringioFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *timeout_params_obj = NULL;
    int offset = -1;
    int size_i = 1024;
    static const char *kwlist[] = {"offset", "size", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iiO", (char **)kwlist, &offset, &size_i, &timeout_params_obj)) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_READ;

    Py_ssize_t size = (Py_ssize_t)size_i;

    BufferPayload *buffer_payload = get_or_create_linear_buffer(NULL, size_i);
    if (!buffer_payload) {
        Py_DECREF(future);
        return NULL;
    }

    StreamStrategy stream_strategy = get_stream_strategy();

    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, stream_strategy, opcode, self, NULL, NULL, NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = read_dispatcher(self, buffer_payload, stream_strategy, request_idx, (int)size, offset, timeout_params);
    return _check_file_result(result, self, request_idx, future);
}

PyObject *
UringioFile_readv(UringioFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *buffers_obj = NULL;
    PyObject *timeout_params_obj = NULL;
    int nowait = 0;
    int offset = -1;
    static const char *kwlist[] = {"buffers", "offset", "nowait", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "|OipO", (char **)kwlist, &buffers_obj, &offset, &nowait, &timeout_params_obj
        )) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    BufferPayload *buffer_payload;
    buffer_payload = _get_buffer();
    if (!(buffer_payload && (buffer_payload->mode == PROVIDED || buffer_payload->mode == BUF_RING))) {
        buffer_payload = get_or_create_vectored_buffer(buffers_obj);
    }
    if (!buffer_payload)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }
    int opcode = IORING_OP_READV;

    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, ONESHOT, opcode, self, NULL, NULL, NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    int result = readv_dispatcher(self, buffer_payload, request_idx, offset, nowait, timeout_params);
    return _check_file_result(result, self, request_idx, future);
}

PyObject *
UringioFile_readv_raw(UringioFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *iovecs_obj = NULL;
    int nowait = 0;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static const char *kwlist[] = {"iovecs", "offset", "nowait", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "O|iiO", (char **)kwlist, &iovecs_obj, &offset, &nowait, &timeout_params_obj
        )) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    Py_buffer iovecs_buf;
    if (PyObject_GetBuffer(iovecs_obj, &iovecs_buf, PyBUF_WRITABLE | PyBUF_STRIDES) < 0) {
        return NULL;
    }

    if ((unsigned int)(iovecs_buf.len) % sizeof(struct iovec) != 0) {
        PyBuffer_Release(&iovecs_buf);
        PyErr_SetString(PyExc_ValueError, "iovecs buffer has invalid size");
        return NULL;
    }

    if (!PyBuffer_IsContiguous(&iovecs_buf, 'C')) {
        PyBuffer_Release(&iovecs_buf);
        PyErr_SetString(PyExc_ValueError, "iovecs must be contiguous");
        return NULL;
    }

    BufferPayload *buffer_payload = create_buffer_payload_from_pybuffer(&iovecs_buf);
    if (!buffer_payload)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }
    int opcode = IORING_OP_READV;

    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, ONESHOT, opcode, self, NULL, NULL, NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uringio_readv(
        self->loop->ring,
        request_idx,
        self->fd,
        buffer_payload->vector->iovecs,
        (unsigned)buffer_payload->vector->nr_vecs,
        offset,
        nowait,
        timeout_params
    );

    return _check_file_result(result, self, request_idx, future);
}

PyObject *
UringioFile_write(UringioFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *data = NULL;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static const char *kwlist[] = {"data", "offset", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "O|iO", (char **)kwlist, &data, &offset, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future)
        return NULL;

    BufferPayload *buffer_payload = create_buffer_payload_from_data(data);
    if (!buffer_payload) {
        Py_DECREF(future);
        return NULL;
    }
    int opcode = IORING_OP_WRITE;
    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, ONESHOT, opcode, self, NULL, NULL, NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = write_dispatcher(self, buffer_payload, request_idx, offset, timeout_params);
    return _check_file_result(result, self, request_idx, future);
}

PyObject *
UringioFile_writev(UringioFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *buffers_obj = NULL;

    int flags = 0;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static const char *kwlist[] = {"buffers", "flags", "offset", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "O|iiO", (char **)kwlist, &buffers_obj, &flags, &offset, &timeout_params_obj
        ))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    BufferPayload *buffer_payload = get_or_create_vectored_buffer(buffers_obj);
    if (!buffer_payload)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }

    int opcode = IORING_OP_WRITEV;
    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, ONESHOT, opcode, self, NULL, NULL, NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uringio_writev(
        self->loop->ring,
        request_idx,
        self->fd,
        buffer_payload->vector->iovecs,
        (unsigned int)(buffer_payload->vector->nr_vecs),
        offset,
        flags,
        timeout_params
    );

    return _check_file_result(result, self, request_idx, future);
}

PyObject *
UringioFile_writev_raw(UringioFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *buffers_obj = NULL;
    Py_buffer iovecs_buf;

    int flags = 0;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static const char *kwlist[] = {"buffers", "flags", "offset", "timeout_params", NULL};

    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "O|iiO", (char **)kwlist, &buffers_obj, &flags, &offset, &timeout_params_obj
        )) {
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    if (PyObject_GetBuffer(buffers_obj, &iovecs_buf, PyBUF_SIMPLE) < 0)
        return NULL;

    if ((size_t)iovecs_buf.len % sizeof(struct iovec) != 0) {
        PyBuffer_Release(&iovecs_buf);
        PyErr_SetString(PyExc_ValueError, "iovecs buffer has invalid size");
        return NULL;
    }

    if (!PyBuffer_IsContiguous(&iovecs_buf, 'C')) {
        PyBuffer_Release(&iovecs_buf);
        PyErr_SetString(PyExc_ValueError, "iovecs must be contiguous");
        return NULL;
    }

    BufferPayload *buffer_payload = create_buffer_payload_from_pybuffer(&iovecs_buf);
    if (!buffer_payload) {
        PyBuffer_Release(&iovecs_buf);
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }

    int opcode = IORING_OP_WRITEV;
    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, ONESHOT, opcode, self, NULL, NULL, NULL
    );

    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uringio_writev(
        self->loop->ring,
        request_idx,
        self->fd,
        buffer_payload->vector->iovecs,
        (unsigned)buffer_payload->vector->nr_vecs,
        offset,
        flags,
        timeout_params
    );

    return _check_file_result(result, self, request_idx, future);
}

PyObject *
UringioFile_fsync(UringioFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", (char **)kwlist, &timeout_params_obj)) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_FSYNC;
    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, self, NULL, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uringio_fsync(self->loop->ring, request_idx, self->fd, timeout_params);
    return _check_file_result(result, self, request_idx, future);
}

PyObject *
UringioFile_fdatasync(UringioFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", (char **)kwlist, &timeout_params_obj)) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_FSYNC;
    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, self, NULL, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uringio_fdatasync(self->loop->ring, request_idx, self->fd, timeout_params);
    return _check_file_result(result, self, request_idx, future);
}

PyObject *
UringioFile_splice(UringioFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
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
    static const char *kwlist[] = {"src", "dst", "count", "offset_src", "offset_dst", "flag", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "iii|iiiO",
            (char **)kwlist,
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
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_SPLICE;
    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, self, NULL, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uringio_splice(
        self->loop->ring, request_idx, src, offset_src, dst, offset_dst, count, flag, timeout_params
    );
    return _check_file_result(result, self, request_idx, future);
}

PyObject *
_check_file_result(int result, UringioFile *file, int request_idx, PyObject *future) {
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(file->loop->registry, request_idx);
        return NULL;
    }
    return future;
}
