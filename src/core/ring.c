#include "core.h"


int ring_init(
    memory_params *memory_params,
    ring_init_params *params,
    struct io_uring *ring
)
{
    // if (memory_params) {
    //     if (!params) {
    //         params = {0}
    //     }
    //     result = io_uring_queue_init_mem(),
    // }
    // else if (params) {
    //     // TODO
    //     result = io_uring_queue_init_params(8, ring, params);
    // }
    // else {

    // Now only default initialization
    // TODO: Do full initialization
    int result = io_uring_queue_init(256, ring, 0);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return -1;
    }
    return 0;
}


void ring_destroy(struct io_uring* ring) {
    if (!ring) {
        fprintf(stderr, "Error: io_uring is NULL\n");
        return;
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Error: SQE is not available\n");
        return;
    }
    io_uring_prep_cancel(sqe, NULL, IORING_ASYNC_CANCEL_ANY);
    int result = io_uring_submit(ring);

    if (result < 0) {
        fprintf(stderr, "Error: io_uring_submit failed: %s\n", strerror(-result));
        return;
    }
    io_uring_queue_exit(ring);
}