#include "files.h"


int open_file(
    int dfd,
    struct io_uring *ring,
    const char *path,
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

    io_uring_prep_openat2(sqe, dfd, path, how);
    io_uring_sqe_set_data(sqe, NULL);
    io_uring_submit(ring);
    return 0;
}

int read(
    int fd,
    struct io_uring *ring,
    const void *buf,
    // __u64 offset,  TODO
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    __u64 offset = 0;
    // io_uring_prep_read_fixed <- with fix buffer, but need to do io_uring_register 
    io_uring_prep_read(sqe, fd, buf, sizeof(buf), offset);
    io_uring_sqe_set_data(sqe, buf);
    io_uring_submit(&ring);
    return 0;
}

void write(int fd, struct io_uring *ring, const void *buf)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    __u64 offset = 0;
    io_uring_prep_write(sqe, fd, buf, sizeof(buf), offset);
    io_uring_sqe_set_data(sqe, buf);
    io_uring_submit(&ring);
    return 0;
}

void close(int fd, struct io_uring *ring)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    __u64 offset = 0;

    io_uring_prep_close(sqe, fd);
    io_uring_sqe_set_data(sqe, NULL);
    io_uring_submit(&ring);
    return 0;
}

void stat(
    int dfd,
    struct io_uring *ring,
    const char *path,
    // int flags,  TODO
	// mode_t mode
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    int flags = 0;
	mode_t mode = NULL;

    io_uring_prep_statx(sqe, dfd, path, flags, mode);
    io_uring_sqe_set_data(sqe, NULL);
    io_uring_submit(&ring);
    return 0;
}

void fsync(
    int fd, 
    struct io_uring *ring,
    // unsigned fsync_flags,  TODO
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    unsigned fsync_flags = NULL;

    io_uring_prep_fsync(sqe, fd, fsync_flags);
    io_uring_sqe_set_data(sqe, NULL);
    io_uring_submit(&ring);
    return 0;
}
