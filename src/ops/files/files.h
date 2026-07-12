#pragma once

#include <fcntl.h>
#include <linux/openat2.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "liburing.h"

#include "macroses.h"

/* Functions */
int
open_file(
    struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path,

    // Below are optional
    int flags,
    int resolve,
    mode_t mode,
    const struct TimeoutParams timeout_params
);

int
uring_read(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    unsigned size,
    int offset,
    const struct TimeoutParams timeout_params
);

int
uring_readv(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    int offset,
    int nowait,

    const struct TimeoutParams timeout_params
);

int
uring_write(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    unsigned size,
    int offset,
    const struct TimeoutParams timeout_params
);

int
uring_writev(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    int offset,
    int flags,

    const struct TimeoutParams timeout_params
);

int
uring_close_file(
    struct io_uring *ring,
    int request_idx,
    int fd,

    const struct TimeoutParams timeout_params
);

int
uring_fsync(
    struct io_uring *ring,
    int request_idx,
    int fd,

    const struct TimeoutParams timeout_params
);

int
uring_fdatasync(
    struct io_uring *ring,
    int request_idx,
    int fd,

    const struct TimeoutParams timeout_params
);

int
uring_splice(
    struct io_uring *ring,
    int request_idx,
    int fd_in,
    int off_in,
    int fd_out,
    int off_out,
    int nbytes,
    int flag,

    const struct TimeoutParams timeout_params
);

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
);

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
);

int
puring_read_buffer_select(
    struct io_uring *ring,
    int request_idx,
    int fd,
    unsigned size,
    int offset,
    int bgid,
    const struct TimeoutParams timeout_params
);

int
puring_readv_buffer_select(
    struct io_uring *ring,
    int request_idx,
    int fd,
    unsigned size,
    int offset,
    int bgid,
    int nowait,
    const struct TimeoutParams timeout_params
);
