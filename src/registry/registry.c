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
    if (reg->slots) {
        for (unsigned int i = 0; i < reg->size; i++) {
            if (reg->slots[i].future) {
                Py_DECREF(reg->slots[i].future);
            }

            if (reg->slots[i].buffer_payload) {
                Py_DECREF(reg->slots[i].buffer_payload);
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
    if (buffer_payload != NULL)
        Py_INCREF(buffer_payload);

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

    RequestSlot *slot = &reg->slots[index];

    if (slot->future) {
        Py_DECREF(slot->future);
        slot->future = NULL;
    }

    if (slot->buffer_payload) {
        Py_DECREF(slot->buffer_payload);
        slot->buffer_payload = NULL;
    }

    free(slot->msghdr);
    slot->msghdr = NULL;
    if (slot->opcode) {
        slot->opcode = 0;
    }

    reg->top++;
    reg->available_indices[reg->top] = index;
}
