#include "ops/dirs/dirs.h"


int uring_stat(
    // struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path,
    char *buf,
    int flags,
    unsigned mask

    // Below are optional
    // struct TimeoutParams *timeout_params
)
{
    // SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    // if (!mask) {
    //     mask = STATX_ALL;
    // }

    // // io_uring_prep_statx(sqe, dfd, path, flags, mask, buf);
    // io_uring_prep_statx(sqe, dfd, path, flags, mask, NULL);

    // void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    // io_uring_sqe_set_data(sqe, rings_data_pointer);

    // int result = io_uring_submit(ring);
    // if (result < 0) {
    //     fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
    //     return 0;
    // }
    // return 1;
}