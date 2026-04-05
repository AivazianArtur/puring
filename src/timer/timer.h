#pragma once

#include <stdio.h>
#include <stdbool.h>

#include "Python.h"
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


int timer(struct io_uring *ring, TimerParams *timer_params);
int timeout(struct io_uring *ring, struct io_uring_sqe *sqe, TimeoutParams *timeout_params);
