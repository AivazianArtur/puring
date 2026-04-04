#pragma once


#include "queue_events/sqe/sqe.h"
#include "timer/timer.h"

#include "liburing.h"


#define SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params)  \
    struct io_uring_sqe *sqe = create_sqe(ring);  \
    if (sqe < 0) {  \
        return -1;  \
    }  \
    if (timeout(ring, sqe, timeout_params) < 0)  \
        return -1;  \

