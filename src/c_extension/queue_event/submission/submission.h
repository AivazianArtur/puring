#include "liburing.h"

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
