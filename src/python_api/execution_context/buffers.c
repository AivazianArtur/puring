#include "execution_context.h"

LinearBuffer *
create_linear_buffers(int len, int bufsize, BufferPayload *payload) {
    LinearBuffer *buffers = PyMem_Malloc(sizeof(LinearBuffer) * len);
    if (!buffers) {
        PyErr_NoMemory();
        return NULL;
    }
    for (int i = 0; i < len; i++) {
        void *buffer = PyMem_Malloc((size_t)bufsize);
        if (!buffer) {
            for (int j = 0; j < i; j++) {
                PyMem_Free(buffers[j].base);
            }
            PyMem_Free(buffers);
            PyErr_NoMemory();
            return NULL;
        }
        buffers[i] = (LinearBuffer){buffer, (size_t)bufsize};
    }
    payload->linear = buffers;
    payload->vector = NULL;
    payload->views = NULL;
    return buffers;
}

LinearBuffer *
serialize_linear_buffers(PyObject *buffers_obj, int len, BufferPayload *payload) {
    Py_buffer *views = PyMem_Malloc(sizeof(Py_buffer) * len);
    LinearBuffer *buffers = PyMem_Malloc(sizeof(LinearBuffer) * len);
    if (!views || !buffers) {
        PyMem_Free(views);
        PyMem_Free(buffers);
        PyErr_NoMemory();
        return NULL;
    }

    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *item = PySequence_GetItem(buffers_obj, i);
        if (!item) {
            // cleanup views[0..i-1]
            for (Py_ssize_t j = 0; j < i; j++)
                PyBuffer_Release(&views[j]);
            PyMem_Free(views);
            PyMem_Free(buffers);
            return NULL;
        }
        if (PyObject_GetBuffer(item, &views[i], PyBUF_WRITABLE) < 0) {
            Py_DECREF(item);
            for (Py_ssize_t j = 0; j < i; j++)
                PyBuffer_Release(&views[j]);
            PyMem_Free(views);
            PyMem_Free(buffers);
            return NULL;
        }
        buffers[i] = (LinearBuffer){views[i].buf, (size_t)views[i].len};
        Py_DECREF(item);
    }
    payload->linear = buffers;
    payload->vector = NULL;
    payload->views = views;
    return buffers;
}

VectoredBuffer *
create_vectored_buffers(int len, int bufsize, BufferPayload *payload) {
    VectoredBuffer *vec = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!vec) {
        PyErr_NoMemory();
        return NULL;
    }
    vec->iovecs = PyMem_Malloc(sizeof(struct iovec) * len);
    if (!vec->iovecs) {
        PyMem_Free(vec);
        PyErr_NoMemory();
        return NULL;
    }
    vec->nr_vecs = (uint32_t)len;

    for (int i = 0; i < len; i++) {
        void *buffer = PyMem_Malloc((size_t)bufsize);
        if (!buffer) {
            for (int j = 0; j < i; j++) {
                PyMem_Free(vec->iovecs[j].iov_base);
            }
            PyMem_Free(vec->iovecs);
            PyMem_Free(vec);
            PyErr_NoMemory();
            return NULL;
        }
        vec->iovecs[i] = (struct iovec){.iov_base = buffer, .iov_len = (size_t)bufsize};
    }
    payload->vector = vec;
    payload->linear = NULL;
    payload->views = NULL;
    return vec;
}

VectoredBuffer *
serialize_vectored_buffers(PyObject *buffers_obj, int len, BufferPayload *payload) {
    Py_buffer *views = PyMem_Malloc(sizeof(Py_buffer) * len);
    VectoredBuffer *vec = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!views || !vec) {
        PyMem_Free(views);
        PyMem_Free(vec);
        PyErr_NoMemory();
        return NULL;
    }
    vec->iovecs = PyMem_Malloc(sizeof(struct iovec) * len);
    if (!vec->iovecs) {
        PyMem_Free(views);
        PyMem_Free(vec);
        PyErr_NoMemory();
        return NULL;
    }
    vec->nr_vecs = (uint32_t)len;

    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *item = PySequence_GetItem(buffers_obj, i);
        if (!item) {
            for (Py_ssize_t j = 0; j < i; j++)
                PyBuffer_Release(&views[j]);
            PyMem_Free(views);
            PyMem_Free(vec->iovecs);
            PyMem_Free(vec);
            return NULL;
        }
        if (PyObject_GetBuffer(item, &views[i], PyBUF_WRITABLE) < 0) {
            Py_DECREF(item);
            for (Py_ssize_t j = 0; j < i; j++)
                PyBuffer_Release(&views[j]);
            PyMem_Free(views);
            PyMem_Free(vec->iovecs);
            PyMem_Free(vec);
            return NULL;
        }
        vec->iovecs[i] = (struct iovec){.iov_base = views[i].buf, .iov_len = (size_t)views[i].len};
        Py_DECREF(item);
    }
    payload->vector = vec;
    payload->linear = NULL;
    payload->views = views;
    return vec;
}

static BufferPayload *
serialize_buffers(PyObject *buf_obj, int len, BufferPayload *payload) {
    payload->views = NULL;
    payload->linear = NULL;
    payload->vector = NULL;

    payload->views = PyMem_Malloc(sizeof(Py_buffer) * len);
    if (!payload->views) {
        PyErr_NoMemory();
        _free_buffer_payload(payload);
        return NULL;
    }
    memset(payload->views, 0, sizeof(Py_buffer) * len);

    payload->linear = PyMem_Malloc(sizeof(LinearBuffer) * len);
    if (!payload->linear) {
        PyErr_NoMemory();
        _free_buffer_payload(payload);
        return NULL;
    }

    payload->vector = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!payload->vector) {
        PyErr_NoMemory();
        _free_buffer_payload(payload);
        return NULL;
    }

    payload->vector->iovecs = PyMem_Malloc(sizeof(struct iovec) * len);
    if (!payload->vector->iovecs) {
        PyErr_NoMemory();
        _free_buffer_payload(payload);
        return NULL;
    }
    payload->vector->nr_vecs = (uint32_t)len;

    for (int i = 0; i < len; i++) {
        PyObject *item = PySequence_GetItem(buf_obj, i);
        if (!item) {
            _free_buffer_payload(payload);
            return NULL;
        }
        if (PyObject_GetBuffer(item, &payload->views[i], PyBUF_WRITABLE) < 0) {
            Py_DECREF(item);
            _free_buffer_payload(payload);
            return NULL;
        }
        Py_DECREF(item);
        payload->linear[i] = (LinearBuffer){.base = payload->views[i].buf, .len = (size_t)payload->views[i].len};
        payload->vector->iovecs[i] = (struct iovec){.iov_base = payload->views[i].buf,
                                                    .iov_len = (size_t)payload->views[i].len};
    }
    return payload;
}

static BufferPayload *
make_buffers(int len, size_t bufsize, BufferPayload *payload) {
    payload->views = NULL;
    payload->linear = NULL;
    payload->vector = NULL;

    payload->linear = PyMem_Calloc(len, sizeof(LinearBuffer));
    if (!payload->linear) {
        PyErr_NoMemory();
        _free_buffer_payload(payload);
        return NULL;
    }

    payload->vector = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!payload->vector) {
        PyErr_NoMemory();
        _free_buffer_payload(payload);
        return NULL;
    }

    payload->vector->iovecs = PyMem_Malloc(sizeof(struct iovec) * len);
    if (!payload->vector->iovecs) {
        PyErr_NoMemory();
        _free_buffer_payload(payload);
        return NULL;
    }
    payload->vector->nr_vecs = (uint32_t)len;

    for (int i = 0; i < len; i++) {
        void *buf = PyMem_Malloc(bufsize);
        if (!buf) {
            PyErr_NoMemory();
            _free_buffer_payload(payload);
            return NULL;
        }
        payload->linear[i] = (LinearBuffer){.base = buf, .len = bufsize};
        payload->vector->iovecs[i] = (struct iovec){.iov_base = buf, .iov_len = bufsize};
    }
    return payload;
}

void
_free_buffer_payload(BufferPayload *payload) {
    if (!payload)
        return;

    if (payload->views) {
        for (int i = 0; i < payload->len; i++) {
            PyBuffer_Release(&payload->views[i]);
        }
        PyMem_Free(payload->views);
    }

    if (payload->linear) {
        if (payload->payload_origin == PAYLOAD_RUNTIME) {
            for (int i = 0; i < payload->len; i++) {
                PyMem_Free(payload->linear[i].base);
            }
        }
        PyMem_Free(payload->linear);
    }

    if (payload->vector) {
        // если оба заполнены - память одна и та же, уже освободили через linear
        if (payload->payload_origin == PAYLOAD_RUNTIME && !payload->linear) {
            for (uint32_t i = 0; i < payload->vector->nr_vecs; i++) {
                PyMem_Free(payload->vector->iovecs[i].iov_base);
            }
        }
        PyMem_Free(payload->vector->iovecs);
        PyMem_Free(payload->vector);
    }

    PyMem_Free(payload);
}