#pragma once


int uring_stat(
    // struct io_uring *ring,
    int request_idx,
    int dfd,
    const char *path,
    char *buf,
    int flags,
    unsigned mask

    // Below are optional
    // struct TimeoutParams *timeout_params
);
