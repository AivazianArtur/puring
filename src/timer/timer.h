#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "liburing.h"

#include "ring/ring.h"

typedef struct TimerParams {
    int sec;
    int nsec;
    int count;
    bool is_multishot;
} TimerParams;

typedef struct TimeoutParams {
    int sec;
    int nsec;
    bool is_required;
} TimeoutParams;

int
timer(struct io_uring *ring, int request_idx, TimerParams *timer_params);

int
timeout(struct io_uring *ring, struct io_uring_sqe *sqe, const TimeoutParams *timeout_params);
