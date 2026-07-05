#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stddef.h>
#include <inttypes.h>

typedef struct ExecutionContext ExecutionContext;

typedef enum BufferMode { NORMAL_BUF, FIXED, PROVIDED, BUF_RING, BUF_NO_VAL } BufferMode;

typedef enum PayloadOrigin { PAYLOAD_USER, PAYLOAD_RUNTIME, ORIGIN_NO_VAL } PayloadOrigin;

typedef enum PayloadType { PAYLOAD_LINEAR, PAYLOAD_IOVEC, PAYLOAD_LINEAR_AND_IOVEC, PAYLOAD_TYPE_NO_VAL } PayloadType;

typedef struct LineaerBuffer {
    void *buffer;
    size_t len;
    Py_buffer *views;
} LinearBuffer;

typedef struct VectoredBuffer {
    struct iovec *iovecs;
    uint32_t nr_vecs;
    Py_buffer *views;
} VectoredBuffer;

typedef struct BufferPayload {
    PayloadType payload_type;
    PayloadOrigin payload_origin;
    BufferMode mode;

    int len;

    LinearBuffer *linear;
    VectoredBuffer *vector;
} BufferPayload;

typedef struct BufferMetadata {
    PayloadOrigin payload_origin;
    PayloadType payload_type;
    BufferMode mode;
    int len;
    int bufsize;
} BufferMetadata;

extern BufferPayload *
_get_buffer(void);

BufferPayload *
create_buffer_payload(BufferMode mode, PayloadType payload_type, PyObject *buffers_obj);

BufferPayload *
create_buffer_payload_from_pybuffer(Py_buffer *iovecs_buf);

BufferPayload *
create_buffer_payload_from_data(PyObject *data);

BufferPayload *
get_or_create_linear_buffer(PyObject *buffers_obj, int size);

BufferPayload *
get_or_create_vectored_buffer(PyObject *buffers_obj, int len, int bufsize);

LinearBuffer *
create_linear_buffers(int len, int bufsize, BufferPayload *payload);

LinearBuffer *
serialize_linear_buffers(PyObject *buffers_obj, int len, BufferPayload *payload);

VectoredBuffer *
create_vectored_buffers(int len, int bufsize, BufferPayload *payload);

VectoredBuffer *
serialize_vectored_buffers(PyObject *buffers_obj, BufferPayload *payload);

BufferPayload *
serialize_buffers(PyObject *buf_obj, int len, BufferPayload *payload);

BufferPayload *
make_buffers(int len, size_t bufsize, BufferPayload *payload);

void
free_buffer_payload(BufferPayload *payload, bool force);

BufferMetadata
_get_buffer_metadata(PyObject *buffers_obj, BufferMode mode, PayloadType payload_type);
