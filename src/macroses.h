#pragma once

#include "ring/ring.h"
#include "timer/timer.h"

#include "liburing.h"

#define SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params)                                                                \
    struct io_uring_sqe *sqe = create_sqe(ring);                                                                       \
    if (sqe == NULL)                                                                                                   \
        return -1;                                                                                                     \
    if (timeout(ring, sqe, timeout_params) < 0)                                                                        \
        return -1;
