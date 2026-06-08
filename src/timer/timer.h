#pragma once

#include "Python.h"

#include <stdbool.h>
#include <stdio.h>

#include "liburing.h"

#include "queue_events/sqe/sqe.h"

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
timer(struct io_uring *ring, TimerParams *timer_params);

int
timeout(struct io_uring *ring, struct io_uring_sqe *sqe, const TimeoutParams *timeout_params);
