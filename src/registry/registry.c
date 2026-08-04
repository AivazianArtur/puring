#include "registry.h"

RequestRegistry *
registry_new(unsigned int size) {
    RequestRegistry *registry = malloc(sizeof(RequestRegistry));
    if (!registry) {
        perror("Cant allocate memory while creating registry");
        return NULL;
    }

    if (size == 0) {
        size = DEFAULT_REGISTRY_SIZE;
    }
    registry->slots = calloc(size, sizeof(RequestSlot));
    if (!(registry->slots)) {
        free(registry);
        perror("Cant allocate memory while creating registry");
        return NULL;
    }

    registry->available_indices = malloc(size * sizeof(int));
    if (!registry->available_indices) {
        free(registry->slots);
        free(registry);
        perror(
            "Cant allocate memory for `available_indices` while creating "
            "registry"
        );
        return NULL;
    }

    for (int i = 0; i < (int)size; i++) {
        registry->available_indices[i] = i;
    }

    registry->top = (int)size - 1;
    registry->size = size;

    return registry;
}

void
registry_destroy(RequestRegistry *reg) {
    if (!reg)
        return;

    if (reg->slots) {
        for (unsigned int i = 0; i < reg->size; i++) {
            if (reg->slots[i].future) {
                Py_DECREF(reg->slots[i].future);
            }

            if (reg->slots[i].buffer_payload) {
                free_buffer_payload(reg->slots[i].buffer_payload, true);
                reg->slots[i].buffer_payload = NULL;
            }
            if (reg->slots[i].socket) {
                Py_DECREF(reg->slots[i].socket);
            }
            if (reg->slots[i].file) {
                Py_DECREF(reg->slots[i].file);
            }
        }
        free(reg->slots);
    }

    if (reg->available_indices) {
        free(reg->available_indices);
    }

    reg->slots = NULL;
    reg->available_indices = NULL;
    reg->size = 0;
    reg->top = -1;
    free(reg);
}

int
registry_add(
    RequestRegistry *reg,
    PyObject *future,
    BufferPayload *buffer_payload,
    StreamStrategy stream_strategy,
    int opcode,
    PuringFile *file,
    PuringSocket *socket,
    struct sockaddr_storage *sockaddr,
    struct msghdr *msghdr
) {
    if (reg->top < 0) {
        return -1;
    }

    int index = reg->available_indices[reg->top];
    reg->top--;

    RequestSlot *slot = &reg->slots[index];

    slot->user_data = (uint64_t)index;
    slot->opcode = opcode;

    slot->future = future;
    Py_INCREF(future);

    slot->buffer_payload = buffer_payload;

    slot->socket = socket;
    slot->file = file;
    if (socket != NULL) {
        Py_INCREF(socket);
    }
    if (file != NULL) {
        Py_INCREF(file);
    }

    slot->addr = sockaddr;
    slot->stream_strategy = stream_strategy;
    slot->msghdr = msghdr;

    return index;
}

RequestSlot *
registry_get(RequestRegistry *reg, int index) {
    if (index < 0 || index >= (int)(reg->size)) {
        return NULL;
    }

    return &reg->slots[index];
}

void
registry_remove(RequestRegistry *reg, int index) {
    if (index < 0 || index >= (int)(reg->size))
        return;
    if (reg->top >= (int)reg->size - 1) {
        abort();
    }

    RequestSlot *slot = &reg->slots[index];

    if (slot->future) {
        Py_DECREF(slot->future);
        slot->future = NULL;
    }

    if (slot->buffer_payload) {
        free_buffer_payload(slot->buffer_payload, false);
        slot->buffer_payload = NULL;
    }

    if (slot->socket) {
        Py_DECREF(slot->socket);
        slot->socket = NULL;
    }

    if (slot->file) {
        Py_DECREF(slot->file);
        slot->file = NULL;
    }

    free(slot->msghdr);
    slot->msghdr = NULL;
    if (slot->opcode) {
        slot->opcode = 0;
    }

    reg->top++;
    reg->available_indices[reg->top] = index;
}
