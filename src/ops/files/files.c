#include "files.h"


int open_file(
    struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path
    // int flags,  TODO
	// mode_t mode
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }
    struct open_how how = {
        .flags = O_RDONLY,
        .mode = 0,
    };

    io_uring_prep_openat2(sqe, dfd, path, &how);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 0;
}


int uring_read(
    struct io_uring *ring,
    int request_idx,
    int fd
    // void *buf,
    // __u64 offset,  TODO
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    __u64 offset = 0;

    int size = 1024;
    char *buf = malloc(size);
    // io_uring_prep_read_fixed <- with fix buffer, but need to do io_uring_register 
    io_uring_prep_read(sqe, fd, buf, sizeof(buf), offset);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 0;
}


int uring_write(
    struct io_uring *ring,
    int request_idx,
    int fd
    // void *buf,
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    __u64 offset = 0;

    int size = 1024;
    char *buf = malloc(size);

    io_uring_prep_write(sqe, fd, buf, sizeof(buf), offset);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 0;
}


int uring_close(
    struct io_uring *ring,
    int request_idx,
    int fd
    // void *buf,
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    __u64 offset = 0;

    int size = 1024;
    char *buf = malloc(size);

    io_uring_prep_close(sqe, fd);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 0;
}


int uring_stat(
    struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path
    // int flags,  TODO
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    int flags = 0;
    struct statx *statbuf = malloc(sizeof(struct statx));
    unsigned mask = STATX_ALL;

    io_uring_prep_statx(sqe, dfd, path, flags, mask, statbuf);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 0;
}


int uring_fsync(
    struct io_uring *ring,
    int request_idx,
    int fd
    // unsigned fsync_flags,  TODO
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    unsigned fsync_flags = 0;

    io_uring_prep_fsync(sqe, fd, fsync_flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 0;
}

// TODO: Vector ops
// io_uring_prep_sendmsg
// io_uring_prep_recvmsg