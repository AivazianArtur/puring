#pragma once

#define _GNU_SOURCE

#include <linux/openat2.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>


#include "liburing.h"

#include "macroses.h"


/* Functions */
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
);


int uring_read(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    unsigned size,
    int offset,

    // Below are optional
    struct TimeoutParams *timeout_params
);


int uring_readv(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    int offset,
    int flags,

    struct TimeoutParams *timeout_params
);


int uring_write(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    unsigned size,
    int offset,
    // Below are optional
    struct TimeoutParams *timeout_params
);


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
);


int uring_close_file(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    
    // Below are optional
    struct TimeoutParams *timeout_params
);


int uring_stat(
    struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path,
    char *buf,
    int flags,
    unsigned mask,

    // Below are optional
    struct TimeoutParams *timeout_params
);


int uring_fsync(
    struct io_uring *ring,
    int request_idx,
    int fd,

    // Below are optional
    struct TimeoutParams *timeout_params
);


int uring_fdatasync(
    struct io_uring *ring,
    int request_idx,
    int fd,

    // Below are optional
    struct TimeoutParams *timeout_params
);

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
);
