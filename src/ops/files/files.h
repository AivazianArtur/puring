#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <linux/openat2.h>

#include "liburing.h"


/* Functions */
int open_file(int dfd, struct io_uring *ring, const char *path);

int read(int fd, struct io_uring *ring, const void *buf);
int write(int fd, struct io_uring *ring, const void *buf);
int close(int fd, struct io_uring *ring);

int stat(int dfd, struct io_uring *ring, const char *path);
int fsync(int fd, struct io_uring *ring);
