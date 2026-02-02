#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <linux/openat2.h>

#include "liburing.h"


/* Functions */
int open_file(
    struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path,
    // int flags,  TODO
	// mode_t mode
);

int read(
    struct io_uring *ring,
    int request_idx,
    int fd,
    // void *buf,
    // __u64 offset,  TODO
);

int write(
    struct io_uring *ring,
    int request_idx,
    int fd,
    // void *buf,
);
int close(
    struct io_uring *ring,
    int request_idx,
    int fd,
    // void *buf,
);

int stat(
    struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path,
    // int flags,  TODO
	// mode_t mode
);

int fsync(
    struct io_uring *ring,
    int request_idx,
    int fd, 
    // unsigned fsync_flags,  TODO
);
