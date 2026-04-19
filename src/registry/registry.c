#include "registry.h"


RequestRegistry* registry_new(unsigned int size) 
{
    RequestRegistry* registry = malloc(sizeof(RequestRegistry));
    if (!registry) {
        perror("Cant allocate memory while creating registry");
        return NULL;
    }

    if (size == 0) {
        size = DEFAULT_REGISTRY_SIZE;
    }
    registry->slots = calloc(size, sizeof(RequestSlot));
    if (!(registry->slots)) {
        perror("Cant allocate memory while creating registry");
        return NULL;
    }

    registry->available_indices = malloc(size * sizeof(int));
    if (!registry->available_indices) {
        free(registry->slots);
        perror("Cant allocate memory for `available_indices` while creating registry");
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        registry->available_indices[i] = i;
    }
    
    registry->top = size - 1;
    registry->size = size;

    return registry;
}

void registry_destroy(RequestRegistry *reg)
{
    if (reg->slots) {
        for (unsigned int i = 0; i < reg->size; i++) {
            if (reg->slots[i].future) {
                Py_DECREF(reg->slots[i].future);
            }

            if (reg->slots[i].buffer) {
                Py_DECREF(reg->slots[i].buffer);
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

int registry_add(
    RequestRegistry *reg,
    PyObject *future,
    PyObject *buffer,
    Py_buffer *iovecs_buffer,
    int opcode,
    UringFile *file,
    UringSocket *socket
) 
{
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

    slot->buffer = buffer;
    if (buffer != NULL) Py_INCREF(buffer);

    slot->iovecs_buffer = iovecs_buffer;
    if (iovecs_buffer != NULL) Py_INCREF(iovecs_buffer);

    slot->socket = socket;
    slot->file = file;
    if (socket != NULL) Py_INCREF(socket);
    if (file != NULL) Py_INCREF(file);

    return index;
}

RequestSlot* registry_get(RequestRegistry *reg, int index) 
{
    if (index < 0 || index >= reg->size) {
        return NULL;
    }
    
    return &reg->slots[index];
}

void registry_remove(RequestRegistry *reg, int index) 
{
    if (index < 0 || index >= reg->size) return;

    RequestSlot *slot = &reg->slots[index];

    if (slot->future) {
        Py_DECREF(slot->future);
        slot->future = NULL;
    }
    
    if (slot->buffer) {
        Py_DECREF(slot->buffer);
        slot->buffer = NULL;
    }

    if (slot->iovecs_buffer) {
        Py_DECREF(slot->iovecs_buffer);
        slot->iovecs_buffer = NULL;
    }

    if (slot->opcode) {
        slot->opcode = 0;
    }

    reg->top++;
    reg->available_indices[reg->top] = index;
}
