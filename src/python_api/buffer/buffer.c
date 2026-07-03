#include "python_api/buffer/buffer.h"

BufferPayload *
create_buffer_payload(BufferMetadata buffer_metadata, PyObject *buffers_obj) {
    BufferPayload *buffer_payload = malloc(sizeof(BufferPayload));
    if (!buffer_payload) {
        PyErr_NoMemory();
        return NULL;
    }

    PayloadOrigin origin = buffer_metadata.payload_origin;
    PayloadType payload_type = buffer_metadata.payload_type;
    BufferMode mode = buffer_metadata.mode;
    int len = buffer_metadata.len;
    int bufsize = buffer_metadata.bufsize;

    LinearBuffer *linear_buffers;
    VectoredBuffer *vectored_buffers;

    switch (mode) {
    case NORMAL_BUF:
        switch (payload_type) {
        case PAYLOAD_LINEAR:
            if (origin == PAYLOAD_RUNTIME) {
                linear_buffers = create_linear_buffers(len, bufsize, buffer_payload);
            } else if (origin == PAYLOAD_USER) {
                linear_buffers = serialize_linear_buffers(buffers_obj, len, buffer_payload);
            } else {
                return NULL;
            }
            if (!linear_buffers)
                return NULL;
            buffer_payload->linear = linear_buffers;
            break;
        case PAYLOAD_IOVEC:
            if (origin == PAYLOAD_RUNTIME) {
                vectored_buffers = create_vectored_buffers(len, bufsize, buffer_payload);
            } else if (origin == PAYLOAD_USER) {
                vectored_buffers = serialize_vectored_buffers(buffers_obj, len, buffer_payload);
            } else {
                return NULL;
            }
            if (!vectored_buffers)
                return NULL;
            buffer_payload->vector = vectored_buffers;
            break;
        case PAYLOAD_LINEAR_AND_IOVEC:
            if (origin == PAYLOAD_RUNTIME) {
                buffer_payload = make_buffers(len, (size_t)bufsize, buffer_payload);
            } else if (origin == PAYLOAD_USER) {
                buffer_payload = serialize_buffers(buffers_obj, len, buffer_payload);
            } else {
                return NULL;
            }
            if (!buffer_payload)
                return NULL;
            break;
        case PAYLOAD_TYPE_NO_VAL:
            return NULL;
        default:
            return NULL;
        }
    case FIXED:
    case PROVIDED:
    case BUF_RING:
        if (origin == PAYLOAD_RUNTIME) {
            linear_buffers = create_linear_buffers(len, bufsize, buffer_payload);
        } else if (origin == PAYLOAD_USER) {
            linear_buffers = serialize_linear_buffers(buffers_obj, len, buffer_payload);
        } else {
            return NULL;
        }
        if (!linear_buffers)
            return NULL;
        buffer_payload->linear = linear_buffers;
        break;
    case BUF_NO_VAL:
        fprintf(stderr, "Wrong value for buffer mode");
        return NULL;
    default:
        fprintf(stderr, "Wrong value for buffer mode");
        return NULL;
    }

    buffer_payload->len = len;
    buffer_payload->payload_origin = origin;
    buffer_payload->mode = mode;
    if (!buffer_payload->payload_type) {
        buffer_payload->payload_type = payload_type;
    }
    return buffer_payload;
}

BufferPayload *
create_buffer_payload_from_pybuffer(BufferMetadata buffer_metadata, Py_buffer *iovecs_buf) {
    BufferPayload *buffer_payload = malloc(sizeof(BufferPayload));
    if (!buffer_payload) {
        PyErr_NoMemory();
        return NULL;
    }
    buffer_payload->len = (int)iovecs_buf->len;
    buffer_payload->linear = NULL;
    buffer_payload->mode = buffer_metadata.mode;
    buffer_payload->payload_origin = buffer_metadata.payload_origin;
    buffer_payload->payload_type = PAYLOAD_IOVEC;
    buffer_payload->vector->iovecs = (struct iovec *)iovecs_buf->buf;
    buffer_payload->vector->nr_vecs = (unsigned int)(iovecs_buf->len) / sizeof(struct iovec);
    return buffer_payload;
}

LinearBuffer *
create_linear_buffers(int len, int bufsize, BufferPayload *payload) {
    LinearBuffer *buffers = PyMem_Malloc(sizeof(LinearBuffer) * (size_t)len);
    if (!buffers) {
        PyErr_NoMemory();
        return NULL;
    }
    for (int i = 0; i < len; i++) {
        void *buffer = PyMem_Malloc((size_t)bufsize);
        if (!buffer) {
            for (int j = 0; j < i; j++) {
                PyMem_Free(buffers[j].buffer);
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
    payload->payload_type = PAYLOAD_LINEAR;
    return buffers;
}

LinearBuffer *
serialize_linear_buffers(PyObject *buffers_obj, int len, BufferPayload *payload) {
    Py_buffer *views = PyMem_Malloc(sizeof(Py_buffer) * (size_t)len);
    LinearBuffer *buffers = PyMem_Malloc(sizeof(LinearBuffer) * (size_t)len);
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
    payload->payload_type = PAYLOAD_LINEAR;
    return buffers;
}

VectoredBuffer *
create_vectored_buffers(int len, int bufsize, BufferPayload *payload) {
    VectoredBuffer *vec = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!vec) {
        PyErr_NoMemory();
        return NULL;
    }
    vec->iovecs = PyMem_Malloc(sizeof(struct iovec) * (size_t)len);
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
    Py_buffer *views = PyMem_Malloc(sizeof(Py_buffer) * (size_t)len);
    VectoredBuffer *vec = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!views || !vec) {
        PyMem_Free(views);
        PyMem_Free(vec);
        PyErr_NoMemory();
        return NULL;
    }
    vec->iovecs = PyMem_Malloc(sizeof(struct iovec) * (size_t)len);
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

BufferPayload *
serialize_buffers(PyObject *buf_obj, int len, BufferPayload *payload) {
    payload->views = NULL;
    payload->linear = NULL;
    payload->vector = NULL;

    payload->views = PyMem_Malloc(sizeof(Py_buffer) * (size_t)len);
    if (!payload->views) {
        PyErr_NoMemory();
        free_buffer_payload(payload);
        return NULL;
    }
    memset(payload->views, 0, sizeof(Py_buffer) * (size_t)len);

    payload->linear = PyMem_Malloc(sizeof(LinearBuffer) * (size_t)len);
    if (!payload->linear) {
        PyErr_NoMemory();
        free_buffer_payload(payload);
        return NULL;
    }

    payload->vector = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!payload->vector) {
        PyErr_NoMemory();
        free_buffer_payload(payload);
        return NULL;
    }

    payload->vector->iovecs = PyMem_Malloc(sizeof(struct iovec) * (size_t)len);
    if (!payload->vector->iovecs) {
        PyErr_NoMemory();
        free_buffer_payload(payload);
        return NULL;
    }
    payload->vector->nr_vecs = (uint32_t)len;

    for (int i = 0; i < len; i++) {
        PyObject *item = PySequence_GetItem(buf_obj, i);
        if (!item) {
            free_buffer_payload(payload);
            return NULL;
        }
        if (PyObject_GetBuffer(item, &payload->views[i], PyBUF_WRITABLE) < 0) {
            Py_DECREF(item);
            free_buffer_payload(payload);
            return NULL;
        }
        Py_DECREF(item);
        payload->linear[i] = (LinearBuffer){.buffer = payload->views[i].buf, .len = (size_t)payload->views[i].len};
        payload->vector->iovecs[i] = (struct iovec){.iov_base = payload->views[i].buf,
                                                    .iov_len = (size_t)payload->views[i].len};
    }
    return payload;
}

BufferPayload *
make_buffers(int len, size_t bufsize, BufferPayload *payload) {
    payload->views = NULL;
    payload->linear = NULL;
    payload->vector = NULL;

    payload->linear = PyMem_Calloc((size_t)len, sizeof(LinearBuffer));
    if (!payload->linear) {
        PyErr_NoMemory();
        free_buffer_payload(payload);
        return NULL;
    }

    payload->vector = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!payload->vector) {
        PyErr_NoMemory();
        free_buffer_payload(payload);
        return NULL;
    }

    payload->vector->iovecs = PyMem_Malloc(sizeof(struct iovec) * (size_t)len);
    if (!payload->vector->iovecs) {
        PyErr_NoMemory();
        free_buffer_payload(payload);
        return NULL;
    }
    payload->vector->nr_vecs = (uint32_t)len;

    for (int i = 0; i < len; i++) {
        void *buf = PyMem_Malloc(bufsize);
        if (!buf) {
            PyErr_NoMemory();
            free_buffer_payload(payload);
            return NULL;
        }
        payload->linear[i] = (LinearBuffer){.buffer = buf, .len = bufsize};
        payload->vector->iovecs[i] = (struct iovec){.iov_base = buf, .iov_len = bufsize};
    }

    return payload;
}

void
free_buffer_payload(BufferPayload *payload) {
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
                PyMem_Free(payload->linear[i].buffer);
            }
        }
        PyMem_Free(payload->linear);
    }

    if (payload->vector) {
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

BufferMetadata
get_buffer_metadata(PyObject *buffers_obj, BufferMode mode, PayloadType payload_type) {
    PayloadOrigin payload_origin;
    int len;
    int bufsize;
    BufferMetadata buffer_metadata = {0};

    if (buffers_obj) {
        if (!PySequence_Check(buffers_obj)) {
            PyErr_SetString(PyExc_TypeError, "Buffers must be a sequence");
            return buffer_metadata;
        }
        Py_ssize_t py_len = PySequence_Length(buffers_obj);
        if (py_len < 0) {
            return buffer_metadata;
        }
        len = (int)py_len;
        bufsize = (int)(Py_SIZE(buffers_obj) / len);
        payload_origin = PAYLOAD_USER;
    } else {
        payload_origin = PAYLOAD_RUNTIME;
        // TODO: in next version need to get values from puring parameters(puring initialization)
        len = 3;
        bufsize = 1024;
    }
    if (!mode || mode == BUF_NO_VAL) {
        mode = _get_buffer_mode();
    }
    if (!payload_type || payload_type == PAYLOAD_TYPE_NO_VAL) {
        payload_type = _get_payload_type();
    }

    buffer_metadata = (BufferMetadata){
        .payload_origin = payload_origin,
        .payload_type = payload_type,
        .mode = mode,
        .bufsize = bufsize,
        .len = len,
    };
    return buffer_metadata;
}

// Возможно переименовть, Если это только к iovecs
BufferMetadata
get_buffer_metadata_from_pybuffer(Py_buffer *buffers_obj, BufferMode mode) {
    if (!mode || mode == BUF_NO_VAL) {
        mode = _get_buffer_mode();
    }
    BufferMetadata buffer_metadata = {
        .bufsize = (int)buffers_obj->itemsize,
        .len = (int)buffers_obj->len,
        .mode = mode,
        .payload_type = PAYLOAD_IOVEC,
        .payload_origin = PAYLOAD_USER,
    };
    return buffer_metadata;
}
