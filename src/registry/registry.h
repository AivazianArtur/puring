#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <string.h>
#include <stdbool.h>
#include "python_api/execution_context/execution_context_enums.h"
#include "python_api/buffers/buffers.h"

#define DEFAULT_REGISTRY_SIZE 25000

typedef struct UringioSocket UringioSocket;
typedef struct UringioFile UringioFile;
typedef struct BufferPayload BufferPayload;

typedef struct RequestSlot {
    uint64_t user_data;
    PyObject *future;
    StreamStrategy stream_strategy;
    BufferPayload *buffer_payload;
    int opcode;
    UringioFile *file;
    UringioSocket *socket;
    struct sockaddr_storage *addr;
    struct msghdr *msghdr;
} RequestSlot;

typedef struct RequestRegistry {
    RequestSlot *slots;
    int *available_indices;
    int top;
    unsigned int size;
} RequestRegistry;

RequestRegistry *
registry_new(unsigned int size);

void
registry_destroy(RequestRegistry *reg);

int
registry_add(
    RequestRegistry *reg,
    PyObject *future,
    BufferPayload *buffer_payload,
    StreamStrategy stream_strategy,
    int opcode,
    UringioFile *file,
    UringioSocket *socket,
    struct sockaddr_storage *sockaddr,
    struct msghdr *msghdr
);

RequestSlot *
registry_get(RequestRegistry *reg, int index);

void
registry_remove(RequestRegistry *reg, int index);
