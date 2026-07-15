#include "files.h"

int
puring_read_fixed(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    unsigned size,
    int offset,
    int buf_index,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    io_uring_prep_read_fixed(sqe, fd, buf, size, (uint64_t)offset, buf_index);
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
puring_write_fixed(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    unsigned size,
    int offset,
    int buf_index,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);
    io_uring_prep_write_fixed(sqe, fd, buf, size, (uint64_t)offset, buf_index);
    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);
    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}
