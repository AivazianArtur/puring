#include "files.h"


int open_file(
    struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path,

    // Below are optional
    int flags,
    int resolve,
	mode_t mode,
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    if (!flags) {
        flags = O_RDWR | O_CREAT;
    }

    struct open_how how = {
        .flags = (uint64_t)flags,
        .mode = (mode_t)mode,
        .resolve = (uint64_t)resolve,
    };

    io_uring_prep_openat2(sqe, dfd, path, &how);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_read(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    unsigned size,
    int offset,

    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    io_uring_prep_read(sqe, fd, buf, size, offset);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_readv(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    int offset,
    int flags,

    struct TimeoutParams *timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    io_uring_prep_readv2(sqe, fd, iovecs, nr_vecs, offset, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_write(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    unsigned size,
    int offset,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    io_uring_prep_write(sqe, fd, buf, size, offset);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_writev(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    int offset,
    int flags,

    // Below are optional
    struct TimeoutParams *timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    io_uring_prep_writev2(sqe, fd, iovecs, nr_vecs, offset, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_close_file(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    __u64 offset = 0;

    io_uring_prep_close(sqe, fd);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_fsync(
    struct io_uring *ring,
    int request_idx,
    int fd,

    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    unsigned fsync_flags = 0;

    io_uring_prep_fsync(sqe, fd, fsync_flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_fdatasync(
    struct io_uring *ring,
    int request_idx,
    int fd,

    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    unsigned fsync_flags = IORING_FSYNC_DATASYNC;

    io_uring_prep_fsync(sqe, fd, fsync_flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_splice(
    struct io_uring *ring,
    int request_idx,
    int fd_in,
    int off_in,
	int fd_out, 
    int off_out,
	int nbytes,
	int flag,

    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    io_uring_prep_splice(
        sqe,
        fd_in,
        (int64_t)off_in,
        fd_out,
        (int64_t)off_out,
        (unsigned int)nbytes,
        (unsigned int)flag
    );

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


// TODO: io_uring_prep_read_fixed + io_uring_prep_write_fixed