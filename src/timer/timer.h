#pragma once

#include "liburing.h"


typedef struct TimeoutParams {
    int sec;
    int nsec;
    bool required;
} TimeoutParams;



int timeout(struct io_uring *ring, struct io_uring_sqe *sqe, TimeoutParams *timeout_params);