#include "core.h"


int ring_init(memory_params *memory_params, ring_init_params *params)
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
    result = io_uring_queue_init(0, ring, 0);
    return 0;
}

void ring_destroy(io_uring* ring) 
{

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    // TODO: Create Retry setable loop, including accepting flags etc
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }
    io_uring_prep_cancel(sqe, NULL, IORING_ASYNC_CANCEL_ANY);
    ret = io_uring_submit(ring);
    if (ret < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-ret));
        return;
    }
    // TEMP: MAybe we'll need to catch every cqe but my thought to ignore is this:
    // kernel already cancelled everything, queue_exit will destroy ring. seems fine for now
    io_uring_queue_exit(ring);
}
