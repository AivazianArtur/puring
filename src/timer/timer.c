#include "timer.h"

int
timer(struct io_uring *ring, int request_idx, TimerParams *timer_params) {
    if (!timer_params) {
        fprintf(stderr, "To required params\n");
        return -1;
    }

    struct io_uring_sqe *sqe = create_sqe(ring);
    if (sqe == NULL)
        return -1;

    struct __kernel_timespec ts = {
        .tv_sec = timer_params->sec,
        .tv_nsec = timer_params->nsec,
    };

    int flag = 0;
    if (timer_params->is_multishot) {
        flag = IORING_TIMEOUT_MULTISHOT;
    }

    io_uring_prep_timeout(sqe, &ts, (unsigned int)timer_params->count, (unsigned int)flag);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}

int
timeout(struct io_uring *ring, struct io_uring_sqe *sqe, const TimeoutParams *timeout_params) {
    if (timeout_params == NULL ||
        (timeout_params->sec == 0 && timeout_params->nsec == 0 && !timeout_params->is_required)) {
        return 0;
    }
    sqe->flags |= IOSQE_IO_LINK;

    struct io_uring_sqe *timeout_sqe = io_uring_get_sqe(ring);
    if (!timeout_sqe) {
        fprintf(stderr, "SQE for timeout is not available\n");

        if (timeout_params->is_required) {
            fprintf(stderr, "But timer required, cancel submitting\n");
            return -1;
        }
        return 0;
    }

    struct __kernel_timespec ts = {
        .tv_sec = timeout_params->sec,
        .tv_nsec = timeout_params->nsec,
    };

    io_uring_prep_link_timeout(timeout_sqe, &ts, 0);
    return 0;
}
