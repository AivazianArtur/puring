#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdbool.h>
#include <fcntl.h>


typedef enum PayloadOrigin {
    PAYLOAD_USER,
    PAYLOAD_RUNTIME,
} PayloadOrigin;

typedef enum PayloadType {
    PAYLOAD_LINEAR,
    PAYLOAD_IOVEC,
    PAYLOAD_PROVIDED,
} PayloadType;

typedef struct BufferPayload {
    PayloadType payload_type;
    PayloadOrigin payload_origin;

    int len;

    union {
        struct {
            void *base;
            size_t len;
        } linear;

        struct {
            struct iovec *iovecs;
            uint32_t nr_vecs;
        } vectored;

        struct {
            uint16_t bgid;
            uint16_t bid;
        } provided;
    };

} BufferPayload;


typedef enum BufferMode {
    NORMAL,
    FIXED,
    PROVIDED
} BufferMode;


typedef enum TransferMode {
    NORMAL,
    ZERO_COPY
    // POOL
} TransferMode;


typedef enum StreamStrategy {
    ONESHOT,
    MULTISHOT
} StreamStrategy;


typedef struct ExecutionContext {
    TransferMode transfer_mode;
    StreamStrategy stream;
    BufferMode buffers;
} ExecutionContext;


