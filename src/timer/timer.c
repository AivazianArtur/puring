#include "timer.h"
#include <stdio.h>

int timer() {
;
}


int timeout(struct io_uring *ring, struct io_uring_sqe *sqe, TimeoutParams *timeout_params) {
    if (timeout_params == NULL) {
        return 0;
    }
    sqe->flags |= IOSQE_IO_LINK;

    struct io_uring_sqe *timeout_sqe = io_uring_get_sqe(ring);
    if (!timeout_sqe) {
        fprintf(stderr, "SQE for timeout is not available\n");
        if (*timeout_params)
        return -1;
    }
    struct __kernel_timespec ts = {
        .tv_sec = (*timeout_params).sec,
        .tv_nsec = (*timeout_params).nsec,
    };

    io_uring_prep_link_timeout(sqe, &ts, 0);
    return 0;
}
