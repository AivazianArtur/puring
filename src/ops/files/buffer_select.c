#include "files.h"

int
uringio_read_buffer_select(
    struct io_uring *ring,
    int request_idx,
    int fd,
    unsigned size,
    int offset,
    int bgid,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    io_uring_prep_read(sqe, fd, NULL, size, (uint64_t)offset);

    sqe->flags |= IOSQE_BUFFER_SELECT;
    sqe->buf_group = (__u16)bgid;

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
uringio_readv_buffer_select(
    struct io_uring *ring,
    int request_idx,
    int fd,
    unsigned size,
    int offset,
    int bgid,
    int nowait,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);
    struct iovec iov = {
        .iov_base = NULL,
        .iov_len = size,
    };

    int flags = 0;
    if (nowait) {
        flags |= RWF_NOWAIT;
    }

    io_uring_prep_readv2(sqe, fd, &iov, 1, (uint64_t)offset, flags);

    sqe->flags |= IOSQE_BUFFER_SELECT;
    sqe->buf_group = (__u16)bgid;

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }

    return 1;
}
