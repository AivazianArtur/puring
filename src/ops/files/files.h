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
    // int flags,  TODO
	// mode_t mode

    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_read(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    unsigned size,
    // __u64 offset,  TODO

    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_write(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    unsigned size,

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
    // int flags,  TODO
	// mode_t mode

    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_fsync(
    struct io_uring *ring,
    int request_idx,
    int fd,
    // unsigned fsync_flags,  TODO

    // Below are optional
    struct TimeoutParams *timeout_params
);
