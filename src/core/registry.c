#include "registry.h"


int registry_init(RequestRegistry *reg, unsigned int size) {
    // 1. Allocate the slots (The data)
    reg->slots = calloc(size, sizeof(RequestSlot));
    if (!reg->slots) return -1;

    // 2. Allocate the Free List (The stack of available IDs)
    reg->free_indices = malloc(size * sizeof(int));
    if (!reg->free_indices) {
        free(reg->slots);
        return -1;
    }

    // 3. Fill the Free List
    // Initially, ALL indices (0 to size-1) are free.
    for (int i = 0; i < size; i++) {
        reg->free_indices[i] = i;
    }
    
    reg->top = size - 1; // Point to the last element
    reg->size = size;

    return 0;
}

void registry_destroy(RequestRegistry *reg) {
    // 1. Clean up any lingering Python objects
    if (reg->slots) {
        for (unsigned int i = 0; i < reg->size; i++) {
            // Decref Future
            if (reg->slots[i].future) {
                Py_DECREF(reg->slots[i].future);
            }
            // Decref Buffer
            if (reg->slots[i].buffer) {
                Py_DECREF(reg->slots[i].buffer);
            }
        }
        free(reg->slots);
    }

    // 2. Free the stack
    if (reg->free_indices) {
        free(reg->free_indices);
    }
    
    // Reset struct to safe state
    reg->slots = NULL;
    reg->free_indices = NULL;
    reg->size = 0;
    reg->top = -1;
}

int registry_add(RequestRegistry *reg, PyObject *future, PyObject *buffer, int opcode) {
    // 1. Check if we have space
    if (reg->top < 0) {
        return -1; // Registry is full!
    }

    // 2. Pop a free index from the stack
    int index = reg->free_indices[reg->top];
    reg->top--;

    // 3. Populate the slot
    RequestSlot *slot = &reg->slots[index];
    
    slot->user_data = (uint64_t)index;
    slot->opcode = opcode;

    // 4. Handle Python Objects (Reference Counting is CRITICAL)
    
    // Future: We MUST own it so it doesn't disappear
    slot->future = future;
    Py_INCREF(future); 

    // Buffer: Optional (some ops like Accept don't have a Python buffer yet)
    if (buffer != NULL) {
        slot->buffer = buffer;
        Py_INCREF(buffer);
    } else {
        slot->buffer = NULL;
    }

    return index; // This index goes into sqe->user_data
}

RequestSlot* registry_get(RequestRegistry *reg, int index) {
    if (index < 0 || index >= reg->size) {
        return NULL;
    }
    
    // Return pointer to the slot so the caller can read future/buffer
    return &reg->slots[index];
}

void registry_remove(RequestRegistry *reg, int index) {
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

    // 2. Push index back onto the Free List Stack
    // (We know top < size because we just freed one)
    reg->top++;
    reg->free_indices[reg->top] = index;
}