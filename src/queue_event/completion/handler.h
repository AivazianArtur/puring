#include <stdio.h>
#include <stdint.h>

#include "liburing.h"

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

cqe* completed_jobs(UringRequestObject* ring);
bool is_job_completed(UringRequestObject*  ring, cqe* job);
cqe* wait(UringRequestObject* ring, cqe* job);
void ack_job(UringRequestObject* ring, cqe* job);
