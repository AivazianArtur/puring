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
    const char *path
    // int flags,  TODO
	// mode_t mode
);

int uring_read(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    Py_ssize_t size 
    // __u64 offset,  TODO
);

int uring_write(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf,
    Py_ssize_t size 
);

int uring_close_file(
    struct io_uring *ring,
    int request_idx,
    int fd,
    char *buf
);

int uring_stat(
    struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path,
    char *buf
    // int flags,  TODO
	// mode_t mode
);

int uring_fsync(
    struct io_uring *ring,
    int request_idx,
    int fd
    // unsigned fsync_flags,  TODO
);
