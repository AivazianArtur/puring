#include "files.h"


int open_file(
    struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path,
    // int flags,  TODO
	// mode_t mode

    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    struct open_how how = {
        // TODO: O_APPEND to append, basically need to implement getting this as param
        .flags = O_RDWR | O_CREAT,
        .mode = 0644,
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
    // __u64 offset,  TODO

    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    io_uring_prep_read(sqe, fd, buf, size, 0);

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
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    __u64 offset = 0;

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


int uring_stat(
    // TEMP: Not stable

    struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path,
    char *buf,
    // int flags,  TODO

    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    int flags = 0;
    unsigned mask = STATX_ALL;

    // io_uring_prep_statx(sqe, dfd, path, flags, mask, buf);
    io_uring_prep_statx(sqe, dfd, path, flags, mask, NULL);

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
    // unsigned fsync_flags,  TODO

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

// TODO: Vector ops
// io_uring_prep_sendmsg
// io_uring_prep_recvmsg