#include "files.h"

PyObject *
Puring_open(PyObject *Py_UNUSED(module), PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_IS_PURING();
    ASSERT_LOOP_THREAD(running_loop);
    ASSERT_RING_LOOP_IS_CLOSING(running_loop);

    PuringFile *file = PyObject_New(PuringFile, &PuringFileType);
    if (!file) {
        return PyErr_NoMemory();
    }

    file->loop = running_loop;
    file->closed = false;
    Py_INCREF(running_loop);

    const char *path = NULL;
    int dfd = AT_FDCWD;
    PyObject *py_path_obj = NULL;
    PyObject *timeout_params_obj = NULL;
    int flags = 0;
    int resolve = 0;
    int mode = 0644;

    static const char *kwlist[] = {"path", "dirfd", "flags", "resolve", "mode", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "O|iKKKO", (char **)kwlist, &py_path_obj, &dfd, &timeout_params_obj, &flags, &resolve, &mode
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

    PyObject *future = create_future(running_loop);
    if (!future) {
        Py_DECREF(file);
        return NULL;
    }

    int opcode = IORING_OP_OPENAT2;
    int request_idx = registry_add(running_loop->registry, future, NULL, opcode, file, NULL, NULL);
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

void
PuringFile_dealloc(PuringFile *self) {
    self->closed = true;
    if (self->loop) {
        Py_XDECREF(self->loop);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *
PuringFile_read(PuringFile *self, PyObject *args, PyObject *kwargs) {
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
    parse_timeout_params(timeout_params_obj, &timeout_params);

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

    int request_idx = registry_add(self->loop->registry, future, buffer_payload, opcode, self, NULL, NULL);
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
PuringFile_readv(PuringFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *buffers_obj;
    PyObject *timeout_params_obj = NULL;
    int nowait = 0;
    int offset = -1;
    static const char *kwlist[] = {"buffers", "offset", "nowait", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "|OipO", (char **)kwlist, &buffers_obj, &offset, &nowait, &timeout_params_obj
        )) {
        return NULL;
    }

    BufferPayload *buffer_payload;
    buffer_payload = _get_buffer();
    if (!(buffer_payload && (buffer_payload->mode == PROVIDED || buffer_payload->mode == BUF_RING))) {
        buffer_payload = get_or_create_vectored_buffer(buffers_obj, 0, 0);
    }
    if (!buffer_payload)
        return NULL;

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }
    int opcode = IORING_OP_READV;

    int request_idx = registry_add(self->loop->registry, future, buffer_payload, opcode, self, NULL, NULL);
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
PuringFile_readv_raw(PuringFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    Py_buffer iovecs_buf;
    PyObject *timeout_params_obj = NULL;
    int flags = 0;
    int offset = 0;
    static const char *kwlist[] = {"iovecs", "offset", "flags", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "w*|yiO", (char **)kwlist, &iovecs_buf, &offset, &flags, &timeout_params_obj
        )) {
        return NULL;
    }

    if ((unsigned int)(iovecs_buf.len) % sizeof(struct iovec) != 0) {
        PyBuffer_Release(&iovecs_buf);
        PyErr_SetString(PyExc_ValueError, "iovecs buffer has invalid size");
        return NULL;
    }

    if (!PyBuffer_IsContiguous(&iovecs_buf, 'C')) {
        PyErr_SetString(PyExc_ValueError, "iovecs must be contiguous");
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    BufferPayload *buffer_payload = create_buffer_payload_from_pybuffer(&iovecs_buf);
    if (!buffer_payload)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }
    int opcode = IORING_OP_READV;

    int request_idx = registry_add(self->loop->registry, future, buffer_payload, opcode, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_readv(
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
PuringFile_write(PuringFile *self, PyObject *args, PyObject *kwargs) {
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
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future)
        return NULL;

    BufferPayload *buffer_payload = create_buffer_payload_from_data(data);
    if (!buffer_payload) {
        Py_DECREF(future);
        return NULL;
    }
    int opcode = IORING_OP_WRITE;
    int request_idx = registry_add(self->loop->registry, future, buffer_payload, opcode, self, NULL, NULL);
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
PuringFile_writev(PuringFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    PyObject *buffers_obj;

    int flags = 0;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static const char *kwlist[] = {"buffers", "flags", "offset", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "O|iiiO", (char **)kwlist, &buffers_obj, &flags, &offset, &timeout_params_obj
        ))) {
        return NULL;
    }

    BufferPayload *buffer_payload = get_or_create_vectored_buffer(buffers_obj, 0, 0);
    if (!buffer_payload)
        return NULL;

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }

    int opcode = IORING_OP_WRITEV;
    int request_idx = registry_add(self->loop->registry, future, buffer_payload, opcode, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_writev(
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
PuringFile_writev_raw(PuringFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
        return NULL;
    }

    Py_buffer iovecs_buf;

    int flags = 0;
    int offset = 0;
    PyObject *timeout_params_obj = NULL;

    static const char *kwlist[] = {"buffers", "flags", "offset", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "Oy|iiO", (char **)kwlist, &iovecs_buf, &flags, &offset, &timeout_params_obj
        ))) {
        return NULL;
    }

    if ((unsigned long)(iovecs_buf.len) % sizeof(struct iovec) != 0) {
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

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }

    int opcode = IORING_OP_WRITEV;
    int request_idx = registry_add(self->loop->registry, future, buffer_payload, opcode, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_writev(
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
PuringFile_close(PuringFile *self, PyObject *args, PyObject *kwargs) {
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
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_CLOSE;

    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_close_file(self->loop->ring, request_idx, self->fd, timeout_params);

    return _check_file_result(result, self, request_idx, future);
}

PyObject *
PuringFile_fsync(PuringFile *self, PyObject *args, PyObject *kwargs) {
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
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_FSYNC;
    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_fsync(self->loop->ring, request_idx, self->fd, timeout_params);
    return _check_file_result(result, self, request_idx, future);
}

PyObject *
PuringFile_fdatasync(PuringFile *self, PyObject *args, PyObject *kwargs) {
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
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_FSYNC;
    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_fdatasync(self->loop->ring, request_idx, self->fd, timeout_params);
    return _check_file_result(result, self, request_idx, future);
}

PyObject *
PuringFile_splice(PuringFile *self, PyObject *args, PyObject *kwargs) {
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
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_SPLICE;
    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_splice(
        self->loop->ring, request_idx, src, offset_src, dst, offset_dst, count, flag, timeout_params
    );
    return _check_file_result(result, self, request_idx, future);
}

PyObject *
_check_file_result(int result, PuringFile *file, int request_idx, PyObject *future) {
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
