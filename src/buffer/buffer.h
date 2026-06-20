#pragma once

#include <stdbool.h>
#include <fcntl.h>
#include <stddef.h>
#include <inttypes.h>

typedef enum PayloadOrigin {
    PAYLOAD_USER,
    PAYLOAD_RUNTIME,
} PayloadOrigin;

typedef enum PayloadType {
    PAYLOAD_LINEAR,
    PAYLOAD_IOVEC,
} PayloadType;

typedef struct LineaerBuffer {
    void *base;
    size_t len;
} LinearBuffer;

typedef struct VectoredBuffer {
    struct iovec *iovecs;
    uint32_t nr_vecs;
} VectoredBuffer;

typedef struct BufferPayload {
    PayloadType payload_type;
    PayloadOrigin payload_origin;
    Py_buffer *views;

    int len;

    LinearBuffer *linear;
    VectoredBuffer *vector;

} BufferPayload;
