#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stddef.h>
#include <inttypes.h>

typedef struct ExecutionContext ExecutionContext;

typedef enum BufferMode { NORMAL_BUFFER, FIXED, PROVIDED, BUF_RING } BufferMode;

typedef enum PayloadOrigin { PAYLOAD_USER, PAYLOAD_RUNTIME } PayloadOrigin;

typedef enum PayloadType { PAYLOAD_LINEAR, PAYLOAD_IOVEC, PAYLOAD_LINEAR_AND_IOVEC } PayloadType;

typedef struct LineaerBuffer {
    void *buffer;
    size_t len;
} LinearBuffer;

typedef struct VectoredBuffer {
    struct iovec *iovecs;
    uint32_t nr_vecs;
} VectoredBuffer;

typedef struct BufferPayload {
    PayloadType payload_type;
    PayloadOrigin payload_origin;
    BufferMode mode;
    Py_buffer *views;

    int len;

    LinearBuffer *linear;
    VectoredBuffer *vector;
} BufferPayload;

typedef struct BufferMetadata {
    PayloadOrigin payload_origin;
    BufferMode mode;
    int len;
    int bufsize;
} BufferMetadata;

extern BufferMode
_get_buffer_mode(void);

BufferPayload *
create_buffer_payload(BufferMetadata buffer_metadata, PyObject *buffers_obj);

LinearBuffer *
create_linear_buffers(int len, int bufsize, BufferPayload *payload);

LinearBuffer *
serialize_linear_buffers(PyObject *buffers_obj, int len, BufferPayload *payload);

VectoredBuffer *
create_vectored_buffers(int len, int bufsize, BufferPayload *payload);

VectoredBuffer *
serialize_vectored_buffers(PyObject *buffers_obj, int len, BufferPayload *payload);

BufferPayload *
serialize_buffers(PyObject *buf_obj, int len, BufferPayload *payload);

BufferPayload *
make_buffers(int len, size_t bufsize, BufferPayload *payload);

void
free_buffer_payload(BufferPayload *payload);

BufferMetadata
get_buffer_metadata(PyObject *buffers_obj, BufferMode mode);
