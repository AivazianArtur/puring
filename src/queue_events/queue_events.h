#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdio.h>
#include <stdint.h>
#include "liburing.h"
#include "loop.h"

typedef struct {
    /** Wrapper of liburing's io_uring_sqe with only non-internal fields;  */

    // TODO: For every OPCODE(operation) create their own function
    // TODO: Checking on init

    // TODO:
    // PyObject_HEAD
    // PyObject *future_or_something

    signed int32_t fd; // Target file descriptor.
    uint64_t addt;  // Data pointer for Kernel

    // TEMP
    // For len every opcode should use theirs read(fd, buf, len_bytes); readv(fd, iovecs, nr_iovecs); timeout(ts, count)
    // uint32_t len_bytes; // sqe->len [Opcodes: READ/WRITE; SPLICE] 
    // uint32_t len_iovecs; // sqe->len [Opcode: READV/WRITEV]
    // uint32_t len_events; // sqe->len [Opcode: TIMEOUT]
    uint32_t len; // Len of 3 different types(see above) leaned on OPCODEs 
    uint64_t offset = 0; // For these MUST BE 0 [IORING_OP_ACCEPT, IORING_OP_CONNECT, IORING_OP_TIMEOUT, IORING_OP_POLL_ADD, IORING_OP_NOP] 
    uint64_t user_data;  // Output back to user (data pointer)
    unit64_t wrapped_sqe;
} sqe;

typedef struct {
    int ready_job_amount;
    int space_left;
} sqe_health;


// read(fd, buf, len, offset, ctx)
// write(fd, buf, len, offset, ctx)
// accept(fd, sockaddr*, socklen_t*, ctx)
// timeout(timespec*, ctx)




// Structures
enum cqe_flags {
    CQE_F_BUFFER        = 1u << 0,
    CQE_F_MORE          = 1u << 1,
    CQE_F_SOCK_NONEMPTY = 1u << 2,
    CQE_F_NOTIF         = 1u << 3,
    CQE_F_BUF_MORE      = 1u << 4,
    CQE_F_SKIP          = 1u << 5,
    CQE_F_F32           = 1u << 15,
};


// io_uring_cqe
typedef struct {
    /** Wrapper of liburing's io_uring_cqe with only non-internal fields; */

    // TODO:
    // PyObject_HEAD
    // PyObject *future_or_something;

    uint64_t user_data;  // Output back to user
    int32_t res;  // if >= 0 - succes, else - fail (should raise error)
    uint32_t flags; // bitwise OR of cqe_flags
    // uint64_t big_cqe[]; // TODO: In later versions
} cqe;

// Functions

cqe* completed_jobs(Uring* ring);
bool is_job_completed(Uring*  ring, cqe* job);
cqe* wait(Uring* ring, cqe* job);
void ack_job(Uring* ring, cqe* job);