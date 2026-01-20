#pragma once  // Alternative of `include guards (IFNDEF)`
#include <stdio.h>
#include <stdint.h>
#include "registry.h"

/* Structs */
enum ring_flags
{
    /**
        io_uring_setup() flags map
    */
    IOPOLL,  // (1U << 0)
    SQPOLL,
    SQ_AFF,
    CQSIZE,
    CLAMP,
    ATTACH_MQ,
    R_DISABLED,
    SUBMIT_ALL,
    COOP_TASKRUN,
    TASKRUN_FLAG,
    SQE_128,
    CQE_32,
    SINGLE_ISSUER,
    DEFER_TASKRUN,
    NO_MMAP,
    REGISTRED_FD_ONLY,
    NO_SQARRAY,
    HYBRID_IOPOLL,
    CQE_MIXED,
    SQE_MIXED,  // (1U << 19)
}


enum ring_features
{
    /**
        io_uring_params->features flags
    */
    SINGLE_MMAP,
    NODROP,
    SUBMIT_STABLE,
    RW_CUR_POS,
    CUR_PERSONALITY,
    FAST_POLL,
    POLL_32BITS,
    SQPOLL_NONFIXED,
    EXT_ARG,
    NATIVE_WORKERS,
    RSRC_TAGS,
    CQE_SKIP,
    LINKED_FILE,
    REG_REG_RING,
    RECVSEND_BUNDLE,
    MIN_TIMEOUT,
    RW_ATTR,
    NO_IOWAIT,
}


typedef struct
{
	uint32_t head;
	uint32_t tail;
	uint32_t ring_mask;
	uint32_t ring_entries;
	uint32_t overflow;
	uint32_t cqes;
    uint32_t flags;
	uint32_t resv1;
	uint32_t user_addr;
} offset;


typedef struct 
{
    ring_flags flags;
    uint32_t sq_thread_cpu; // Only valid with: IORING_SETUP_SQPOLL | IORING_SETUP_SQ_AFF
    uint32_t sq_thread_idle; // How long (in ms) the SQ polling thread sleeps when idle, 0 = never sleep (busy spin) 

    ring_features features; // userspace capability bitmap - IORING_FEAT_SINGLE_MMAP
    uint32_t linked_ring; // wq_fd  File descriptor of another io_uring, Used with IORING_SETUP_ATTACH_WQ
    offset sqring_offsets;
    offset cqring_offsets;
} ring_initialization_params;


typedef struct {
    void *user_buf;
    size_t user_buf_size;

    int numa_node;          // -1 = default
    bool huge_pages;
    bool mlock;
    size_t alignment;       // e.g. 4K / 2M
    bool shared;
} memory_params;


/* Main Objects */

/* Ring Wrapper*/
typedef struct {
    PyObject_HEAD
    struct io_uring ring;
    RequestRegistry registry; 
    bool initialized;
    int fd;                 // File descriptor
    void *buffer;           // The buffer where Kernel puts data
} Uring;


/* Functions */
PyObject* uring_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int uring_init(Uring *self, PyObject *args, PyObject *kwargs);
void uring_close(Uring *self);

