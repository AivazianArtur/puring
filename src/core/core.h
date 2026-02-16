#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "liburing.h"


/* Forward declarations */
typedef struct UringLoop UringLoop;
void uring_loop_register_fd(UringLoop *loop);


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

    int numa_node;          // -1 = default
    bool huge_pages;
    bool mlock;
    size_t alignment;       // e.g. 4K / 2M
    bool shared;
} memory_params;


/* Request Registry */
#define DEFAULT_REGISTRY_SIZE 128 
typedef struct {
    uint64_t user_data;     // We store the Index here for verification
    PyObject *future;       // The asyncio Future object
    PyObject *buffer;       // The Python Buffer (bytes/bytearray/memoryview)
    int opcode;             // Opcode for debugging (IORING_OP_READ, etc.)
    PyObject *socket;
} RequestSlot;


typedef struct {
    RequestSlot *slots;     // The actual array of data
    
    // Free List Implementation (O(1) allocation)
    int *free_indices;      // Stack of available indices
    int top;                // Stack pointer (index of the top element)
    
    unsigned int size;      // Total capacity (e.g., 1024)
} RequestRegistry;


/* Functions */

/* Ring */
int ring_init(
    memory_params *memory_params,
    ring_init_params *params,
    struct io_uring *ring
);
void ring_destroy(struct io_uring* ring);

RequestRegistry* registry_new(unsigned int size);
void registry_destroy(RequestRegistry *reg);

int registry_add(
    RequestRegistry *reg,
    PyObject *future,
    PyObject *buffer,
    int opcode,
    PyObject *socket
);

RequestSlot* registry_get(RequestRegistry *reg, int index);

void registry_remove(RequestRegistry *reg, int index);
