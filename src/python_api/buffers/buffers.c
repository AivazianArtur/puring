#include "python_api/buffers/buffers.h"

BufferPayload *
create_buffer_payload(BufferMode mode, PayloadType payload_type, PyObject *buffers_obj, int amount_i, int bufsize_i) {
    BufferMetadata buffer_metadata = _get_buffer_metadata(buffers_obj, mode, payload_type, amount_i, bufsize_i);
    BufferPayload *buffer_payload = malloc(sizeof(BufferPayload));
    if (!buffer_payload) {
        PyErr_NoMemory();
        return NULL;
    }
    buffer_payload->bgid = 0;
    PayloadOrigin origin = buffer_metadata.payload_origin;
    int amount = buffer_metadata.amount;
    int bufsize = buffer_metadata.bufsize;

    LinearBuffer *linear_buffers;
    VectoredBuffer *vectored_buffers;

    switch (mode) {
    case NORMAL_BUF:
        switch (payload_type) {
        case PAYLOAD_LINEAR:
            if (origin == PAYLOAD_RUNTIME) {
                linear_buffers = create_linear_buffers(amount, bufsize, buffer_payload);
            } else if (origin == PAYLOAD_USER) {
                linear_buffers = serialize_linear_buffers(buffers_obj, amount, buffer_payload);
            } else {
                return NULL;
            }
            if (!linear_buffers)
                return NULL;
            buffer_payload->linear = linear_buffers;
            break;
        case PAYLOAD_IOVEC:
            if (origin == PAYLOAD_RUNTIME) {
                vectored_buffers = create_vectored_buffers(amount, bufsize, buffer_payload);
            } else if (origin == PAYLOAD_USER) {
                vectored_buffers = serialize_vectored_buffers(buffers_obj, buffer_payload);
            } else {
                return NULL;
            }
            if (!vectored_buffers)
                return NULL;
            buffer_payload->vector = vectored_buffers;
            break;
        case PAYLOAD_LINEAR_AND_IOVEC:
            if (origin == PAYLOAD_RUNTIME) {
                buffer_payload = make_buffers(amount, (size_t)bufsize, buffer_payload);
            } else if (origin == PAYLOAD_USER) {
                buffer_payload = serialize_buffers(buffers_obj, amount, buffer_payload);
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
        payload_type = PAYLOAD_LINEAR_AND_IOVEC;
        if (origin == PAYLOAD_RUNTIME) {
            buffer_payload = make_buffers(amount, (size_t)bufsize, buffer_payload);
        } else if (origin == PAYLOAD_USER) {
            buffer_payload = serialize_buffers(buffers_obj, amount, buffer_payload);
        } else {
            return NULL;
        }
        if (!buffer_payload)
            return NULL;
        break;
    case PROVIDED:
    case BUF_RING:
        if (mode == PROVIDED) {
            buffer_payload->bgid = rand();
        }
        payload_type = PAYLOAD_LINEAR;
        if (origin == PAYLOAD_RUNTIME) {
            linear_buffers = create_linear_buffers(amount, bufsize, buffer_payload);
        } else if (origin == PAYLOAD_USER) {
            linear_buffers = serialize_linear_buffers(buffers_obj, amount, buffer_payload);
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

    buffer_payload->amount = amount;
    buffer_payload->payload_origin = origin;
    buffer_payload->mode = mode;
    buffer_payload->payload_type = payload_type;
    return buffer_payload;
}

BufferPayload *
create_buffer_payload_from_pybuffer(Py_buffer *iovecs_buf) {
    BufferPayload *buffer_payload = malloc(sizeof(BufferPayload));
    if (!buffer_payload) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(buffer_payload, 0, sizeof(BufferPayload));

    uint32_t nr_vecs = (uint32_t)((size_t)iovecs_buf->len / sizeof(struct iovec));

    VectoredBuffer *vec = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!vec) {
        free(buffer_payload);
        PyErr_NoMemory();
        return NULL;
    }

    vec->iovecs = PyMem_Malloc(sizeof(struct iovec) * (size_t)nr_vecs);
    if (!vec->iovecs) {
        PyMem_Free(vec);
        free(buffer_payload);
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(vec->iovecs, iovecs_buf->buf, sizeof(struct iovec) * (size_t)nr_vecs);
    vec->nr_vecs = nr_vecs;

    Py_buffer *views = PyMem_Malloc(sizeof(Py_buffer));
    if (!views) {
        PyMem_Free(vec->iovecs);
        PyMem_Free(vec);
        free(buffer_payload);
        PyErr_NoMemory();
        return NULL;
    }
    views[0] = *iovecs_buf;
    vec->views = views;

    buffer_payload->payload_type = PAYLOAD_IOVEC;
    buffer_payload->amount = 1;
    buffer_payload->linear = NULL;
    buffer_payload->vector = vec;
    buffer_payload->mode = NORMAL_BUF;
    buffer_payload->payload_origin = PAYLOAD_USER;
    return buffer_payload;
}

BufferPayload *
create_buffer_payload_from_data(PyObject *data) {
    BufferPayload *buffer_payload = malloc(sizeof(BufferPayload));
    if (!buffer_payload) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(buffer_payload, 0, sizeof(BufferPayload));

    LinearBuffer *buffer = PyMem_Malloc(sizeof(LinearBuffer));
    if (!buffer) {
        free(buffer_payload);
        PyErr_NoMemory();
        return NULL;
    }

    Py_buffer *views = PyMem_Malloc(sizeof(Py_buffer));
    if (!views) {
        PyMem_Free(buffer);
        free(buffer_payload);
        PyErr_NoMemory();
        return NULL;
    }

    memset(views, 0, sizeof(Py_buffer));

    if (PyObject_GetBuffer(data, views, PyBUF_CONTIG_RO) < 0) {
        PyMem_Free(views);
        PyMem_Free(buffer);
        free(buffer_payload);
        return NULL;
    }

    buffer->views = views;
    buffer->buffer = (char *)views[0].buf;
    buffer->len = (size_t)views[0].len;

    buffer_payload->payload_type = PAYLOAD_LINEAR;
    buffer_payload->vector = NULL;
    buffer_payload->amount = 1;
    buffer_payload->linear = buffer;

    buffer_payload->payload_origin = PAYLOAD_USER;

    return buffer_payload;
}

BufferPayload *
get_or_create_linear_buffer(PyObject *buffers_obj, int bufsize) {
    PayloadOrigin payload_origin;
    BufferPayload *buffer_payload = _get_buffer();
    if (!buffers_obj) {
        if (buffer_payload && buffer_payload->payload_type == PAYLOAD_LINEAR && buffer_payload->linear) {
            return buffer_payload;
        }
    }

    buffer_payload = malloc(sizeof(BufferPayload));
    if (!buffer_payload) {
        return NULL;
    }
    if (buffers_obj) {
        payload_origin = PAYLOAD_USER;
        serialize_linear_buffers(buffers_obj, 1, buffer_payload);
    } else {
        payload_origin = PAYLOAD_RUNTIME;
        create_linear_buffers(1, bufsize, buffer_payload);
    }
    buffer_payload->amount = 1;
    buffer_payload->mode = NORMAL_BUF;
    buffer_payload->payload_origin = payload_origin;
    buffer_payload->payload_type = PAYLOAD_LINEAR;
    return buffer_payload;
}

BufferPayload *
get_or_create_vectored_buffer(PyObject *buffers_obj) {
    const VectoredBuffer *vectored_buffers;
    PayloadOrigin payload_origin;
    BufferPayload *buffer_payload = _get_buffer();
    if (!buffers_obj) {
        if (buffer_payload && buffer_payload->payload_type == PAYLOAD_IOVEC && buffer_payload->vector) {
            return buffer_payload;
        }
    }

    buffer_payload = malloc(sizeof(BufferPayload));
    if (!buffer_payload)
        return NULL;
    memset(buffer_payload, 0, sizeof(BufferPayload));

    if (buffers_obj) {
        payload_origin = PAYLOAD_USER;
        vectored_buffers = serialize_vectored_buffers(buffers_obj, buffer_payload);
    } else {
        payload_origin = PAYLOAD_RUNTIME;
        vectored_buffers = create_vectored_buffers(1, 1024, buffer_payload);
    }
    if (!vectored_buffers) {
        free(buffer_payload);
        return NULL;
    }

    buffer_payload->amount = (int)vectored_buffers->nr_vecs;
    buffer_payload->mode = NORMAL_BUF;
    buffer_payload->payload_origin = payload_origin;
    buffer_payload->payload_type = PAYLOAD_IOVEC;
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
        buffers[i] = (LinearBuffer){buffer, (size_t)bufsize, NULL};
    }
    payload->linear = buffers;
    payload->vector = NULL;
    buffers->views = NULL;
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
        if (PyObject_GetBuffer(item, &views[i], PyBUF_CONTIG_RO) < 0) {
            Py_DECREF(item);
            for (Py_ssize_t j = 0; j < i; j++)
                PyBuffer_Release(&views[j]);
            PyMem_Free(views);
            PyMem_Free(buffers);
            return NULL;
        }
        buffers[i] = (LinearBuffer){views[i].buf, (size_t)views[i].len, NULL};
        Py_DECREF(item);
    }
    payload->linear = buffers;
    payload->vector = NULL;
    buffers->views = views;
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
    vec->views = NULL;
    return vec;
}

VectoredBuffer *
serialize_vectored_buffers(PyObject *buffers_obj, BufferPayload *payload) {
    PyObject *fast = PySequence_Fast(buffers_obj, "buffers must be a sequence");
    if (!fast)
        return NULL;

    Py_ssize_t nr_vecs = PySequence_Fast_GET_SIZE(fast);
    int len = (int)nr_vecs;
    Py_buffer *views = PyMem_Malloc(sizeof(Py_buffer) * (size_t)len);
    VectoredBuffer *vec = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!views || !vec) {
        PyMem_Free(views);
        PyMem_Free(vec);
        Py_DECREF(fast);
        PyErr_NoMemory();
        return NULL;
    }
    vec->iovecs = PyMem_Malloc(sizeof(struct iovec) * (size_t)len);
    if (!vec->iovecs) {
        PyMem_Free(views);
        PyMem_Free(vec);
        Py_DECREF(fast);
        PyErr_NoMemory();
        return NULL;
    }
    vec->nr_vecs = (uint32_t)len;

    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *item = PySequence_Fast_GET_ITEM(fast, i);
        if (PyObject_GetBuffer(item, &views[i], PyBUF_CONTIG_RO) < 0) {
            for (Py_ssize_t j = 0; j < i; j++)
                PyBuffer_Release(&views[j]);
            PyMem_Free(views);
            PyMem_Free(vec->iovecs);
            PyMem_Free(vec);
            Py_DECREF(fast);
            return NULL;
        }
        vec->iovecs[i] = (struct iovec){.iov_base = views[i].buf, .iov_len = (size_t)views[i].len};
    }
    payload->vector = vec;
    payload->linear = NULL;
    vec->views = views;
    Py_DECREF(fast);
    return vec;
}

BufferPayload *
serialize_buffers(PyObject *buf_obj, int len, BufferPayload *payload) {
    payload->linear = NULL;
    payload->vector = NULL;
    Py_buffer *views = PyMem_Malloc(sizeof(Py_buffer) * (size_t)len);
    if (!views) {
        PyErr_NoMemory();
        free_buffer_payload(payload, true);
        return NULL;
    }
    memset(views, 0, sizeof(Py_buffer) * (size_t)len);

    payload->linear = PyMem_Malloc(sizeof(LinearBuffer) * (size_t)len);
    if (!payload->linear) {
        PyErr_NoMemory();
        free_buffer_payload(payload, true);
        return NULL;
    }

    payload->vector = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!payload->vector) {
        PyErr_NoMemory();
        free_buffer_payload(payload, true);
        return NULL;
    }

    payload->vector->iovecs = PyMem_Malloc(sizeof(struct iovec) * (size_t)len);
    if (!payload->vector->iovecs) {
        PyErr_NoMemory();
        free_buffer_payload(payload, true);
        return NULL;
    }
    payload->vector->nr_vecs = (uint32_t)len;

    for (int i = 0; i < len; i++) {
        PyObject *item = PySequence_GetItem(buf_obj, i);
        if (!item) {
            free_buffer_payload(payload, true);
            return NULL;
        }
        if (PyObject_GetBuffer(item, &views[i], PyBUF_CONTIG_RO) < 0) {
            Py_DECREF(item);
            free_buffer_payload(payload, true);
            return NULL;
        }
        Py_DECREF(item);
        payload->linear[i] = (LinearBuffer){.buffer = views[i].buf, .len = (size_t)views[i].len};
        payload->vector->iovecs[i] = (struct iovec){.iov_base = views[i].buf, .iov_len = (size_t)views[i].len};
    }
    payload->linear->views = views;
    payload->vector->views = NULL;
    return payload;
}

BufferPayload *
make_buffers(int len, size_t bufsize, BufferPayload *payload) {
    payload->linear = NULL;
    payload->vector = NULL;

    payload->linear = PyMem_Calloc((size_t)len, sizeof(LinearBuffer));
    if (!payload->linear) {
        PyErr_NoMemory();
        free_buffer_payload(payload, true);
        return NULL;
    }

    payload->vector = PyMem_Malloc(sizeof(VectoredBuffer));
    if (!payload->vector) {
        PyErr_NoMemory();
        free_buffer_payload(payload, true);
        return NULL;
    }

    payload->vector->iovecs = PyMem_Malloc(sizeof(struct iovec) * (size_t)len);
    if (!payload->vector->iovecs) {
        PyErr_NoMemory();
        free_buffer_payload(payload, true);
        return NULL;
    }
    payload->vector->nr_vecs = (uint32_t)len;

    for (int i = 0; i < len; i++) {
        void *buf = PyMem_Malloc(bufsize);
        if (!buf) {
            PyErr_NoMemory();
            free_buffer_payload(payload, true);
            return NULL;
        }
        payload->linear[i] = (LinearBuffer){.buffer = buf, .len = bufsize};
        payload->vector->iovecs[i] = (struct iovec){.iov_base = buf, .iov_len = bufsize};
    }

    return payload;
}

void
free_buffer_payload(BufferPayload *payload, bool force) {
    if (!payload)
        return;
    if (!(payload->mode == NORMAL_BUF) && !force)
        return;

    if (payload->linear) {
        if (payload->payload_origin == PAYLOAD_RUNTIME) {
            for (int i = 0; i < payload->amount; i++) {
                PyMem_Free(payload->linear[i].buffer);
            }
        }
        if (payload->linear->views) {
            for (int i = 0; i < payload->amount; i++) {
                PyBuffer_Release(&payload->linear->views[i]);
            }
            PyMem_Free(payload->linear->views);
        }
        PyMem_Free(payload->linear);
    }

    if (payload->vector) {
        if (payload->payload_origin == PAYLOAD_RUNTIME) {
            for (uint32_t i = 0; i < payload->vector->nr_vecs; i++) {
                PyMem_Free(payload->vector->iovecs[i].iov_base);
            }
        }

        if (payload->vector->views) {
            for (int i = 0; i < payload->amount; i++) {
                PyBuffer_Release(&payload->vector->views[i]);
            }
            PyMem_Free(payload->vector->views);
        }
        PyMem_Free(payload->vector->iovecs);
        PyMem_Free(payload->vector);
    }

    free(payload);
}

BufferMetadata
_get_buffer_metadata(
    PyObject *buffers_obj,
    BufferMode mode,
    PayloadType payload_type,
    // TODO: in next version need to get values from uringio parameters(uringio initialization)
    int amount,
    int bufsize
) {
    PayloadOrigin payload_origin;
    BufferMetadata buffer_metadata = {0};

    if (mode == BUF_NO_VAL)
        mode = NORMAL_BUF;

    if (payload_type == PAYLOAD_TYPE_NO_VAL)
        payload_type = PAYLOAD_LINEAR;

    if (buffers_obj == NULL) {
        payload_origin = PAYLOAD_RUNTIME;
    } else {
        if (!PySequence_Check(buffers_obj)) {
            PyErr_SetString(PyExc_TypeError, "Buffers must be a sequence");
            return buffer_metadata;
        }
        Py_ssize_t py_len = PySequence_Length(buffers_obj);
        if (py_len < 0) {
            return buffer_metadata;
        }
        amount = (int)py_len;
        bufsize = (int)(Py_SIZE(buffers_obj) / amount);
        payload_origin = PAYLOAD_USER;
    }

    buffer_metadata = (BufferMetadata){
        .payload_origin = payload_origin,
        .payload_type = payload_type,
        .mode = mode,
        .bufsize = bufsize,
        .amount = amount,
    };
    return buffer_metadata;
}
