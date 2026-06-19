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
