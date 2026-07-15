#include "buffer_controllers/buffer_controllers.h"

BufferIdxRegistry *
buffer_idx_registry_new(unsigned int size) {
    BufferIdxRegistry *registry = malloc(sizeof(BufferIdxRegistry));
    if (!registry) {
        perror("Cant allocate memory while creating buffer index registry for fixed mode");
        return NULL;
    }

    if (size == 0) {
        size = DEFAULT_BUFFER_IDX_REGISTRY_SIZE;
    }

    registry->available_indices = malloc(size * sizeof(int));
    if (!registry->available_indices) {
        free(registry);
        perror("Cant allocate memory for `available_indices` while creating registry");
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
buffer_idx_registry_destroy(BufferIdxRegistry *reg) {
    if (reg->available_indices) {
        free(reg->available_indices);
    }

    reg->available_indices = NULL;
    reg->size = 0;
    reg->top = -1;
}

int
get_buffer_idx(BufferIdxRegistry *reg) {
    if (reg->top < 0) {
        return -1;
    }

    int index = reg->available_indices[reg->top];
    reg->top--;

    return index;
}

int
release_buffer_idx(BufferIdxRegistry *reg, int index) {
    fprintf(stderr, "SHALOM \n");
    fprintf(stderr, "III %d", reg->top);
    // fprintf(stderr, "IV %d", reg->size);
    if (reg->top + 1 >= (int)reg->size) {
        fprintf(stderr, "SHALOM1 \n");
        return -1;
    }
    fprintf(stderr, "SHALOM2 \n");

    reg->available_indices[++reg->top] = index;
    fprintf(stderr, "SHALOM3 \n");
    return 0;
}
