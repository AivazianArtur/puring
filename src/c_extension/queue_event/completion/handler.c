#include "liburing.h"
#include "handler.h"
#include "uring.h"

// void cqe_usage_draft_example() {
    // API
    // Completion Queue

    // int io_uring_wait_cqe(struct io_uring *ring, struct io_uring_cqe **cqe_ptr);
    // int io_uring_peek_cqe(struct io_uring *ring, struct io_uring_cqe **cqe_ptr); - Non-blocking check for a completion.
    // int io_uring_wait_cqe_timeout(struct io_uring *ring, struct io_uring_cqe **cqe_ptr, struct __kernel_timespec *ts);  - Wait for a completion with a timeout.
    // unsigned io_uring_cq_ready(struct io_uring *ring); - Return the number of available CQEs.
    
    // Marking SQE as completed 
    // void io_uring_cqe_seen(struct io_uring *ring, struct io_uring_cqe *cqe); - Mark a CQE as consumed.
    // void io_uring_cq_advance(struct io_uring *ring, unsigned nr); - Manually advance the CQ head by nr entries.
// }

cqe* completed_jobs(Uring* ring)
/** Return the number of available CQEs. */
{
    // TODO: Adapt for python + Logic
    cqe* list_of_completed_jobs;
    list_of_completed_jobs = io_uring_cq_ready(*(ring->ring));
    return list_of_completed_jobs;
}

bool is_job_completed(Uring*  ring, cqe* job)
/** Non-blocking check for a completion. Non-blocking*/
{
    // TODO: Adapt for python + Logic
    int cqe_status;
    cqe_status = io_uring_peek_cqe(*(ring->ring), **job);
    if (cqe_status == 0) {
        return true;
    }
    return false;
}

cqe* wait(Uring* ring, cqe* job)
/** Block until a completion queue entry (CQE) is available. */
{
    if (!ring | !job) {
        printf("DRAFT");
    }
    // TODO: Adapt for Python + Logic
    io_uring_wait_cqe(&(ring->ring), &cqe)
}

void ack_job(Uring* ring, cqe* job)
/** Mark a CQE as consumed. */ 
{
    // TODO: Pyton logic
    void io_uring_cqe_seen(*(ring->ring), job);
}

/* TODO: Build in next versions*/
// void ack_job(Uring* ring, )
// /* Manually advance the CQ head by nr entries. */ 
// {
//     io_uring_cq_advance(*(ring->ring), unsigned nr);
// }

/* TODO: Build in next versions*/
// *cqe wait_with_timeout(ring* ring, cqe* job)
// {
//     if (!ring | !job) {
//         printf("DRAFT");
//     }
//     // TODO: Python Logic
//     io_uring_wait_cqe_timeout(&ring, &cqe)
// }