#include "ring.h"


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
        fprintf(stderr, "Error while setuping SQ and CQ: %s\n", strerror(-result));
        return -1;
    }
    return 0;
}


void ring_destroy(struct io_uring* ring) {
    if (!ring) {
        fprintf(stderr, "Ring desctruction error: io_uring is NULL\n");
        return;
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Ring desctruction error: SQE is not available\n");
        return;
    }
    io_uring_prep_cancel(sqe, NULL, IORING_ASYNC_CANCEL_ANY);
    int result = io_uring_submit(ring);

    if (result < 0) {
        fprintf(stderr, "Ring desctruction error: io_uring_submit failed: %s\n", strerror(-result));
        return;
    }
    io_uring_queue_exit(ring);
}


int ring_prep_wakeup_read(struct io_uring *ring, int wakeup_fd, uint64_t *wakeup_buf)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) 
        return -1;

    io_uring_prep_read(sqe, wakeup_fd, wakeup_buf, sizeof(uint64_t), 0);
    io_uring_sqe_set_data64(sqe, WAKEUP_FD_TAG);
    io_uring_submit(ring);
    return 0;
}
