#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "string.h"

#include "liburing.h"


typedef struct 
{
    uint32_t flags;
    uint32_t sq_thread_cpu;
    uint32_t sq_thread_idle;
    uint32_t features;
    uint32_t linked_ring;
    struct io_sqring_offsets sqring_offsets;
    struct io_cqring_offsets cqring_offsets;
} ring_init_params;


typedef struct 
{
    void *user_buf;
    size_t user_buf_size;
    int numa_node;
    bool huge_pages;
    bool mlock;
    size_t alignment;
    bool shared;
} memory_params;


int ring_init(
    memory_params *memory_params,
    ring_init_params *params,
    struct io_uring *ring
);
void ring_destroy(struct io_uring* ring);
