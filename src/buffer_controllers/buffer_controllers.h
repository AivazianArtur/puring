#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/openat2.h>
#include <netinet/in.h>

#include "liburing.h"

#include "macroses.h"

#define DEFAULT_BUFFER_IDX_REGISTRY_SIZE 128

typedef struct BufferIdxRegistry {
    int *available_indices;
    int top;
    unsigned int size;
} BufferIdxRegistry;

int
init_fixed_mode(struct io_uring *ring, struct iovec *iovecs, unsigned nr_iovecs);

int
close_fixed_mode(struct io_uring *ring);

int
init_provided_mode(struct io_uring *ring, void *addr, int len, int nr, int bgid);

int
close_provided_mode(struct io_uring *ring, int nr, int bgid);

struct io_uring_buf_ring *
init_buf_ring_mode(struct io_uring *ring, void *addr, int len, int nentries, int bgid);

int
close_buf_ring_mode(struct io_uring *ring, struct io_uring_buf_ring *buffer_ring, int nentries, int bgid);

BufferIdxRegistry *
buffer_idx_registry_new(unsigned int size);

void
buffer_idx_registry_destroy(BufferIdxRegistry *reg);

int
get_buffer_idx(BufferIdxRegistry *reg);

int
release_buffer_idx(BufferIdxRegistry *reg, int index);
