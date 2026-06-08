#pragma once

#include "queue_events/sqe/sqe.h"
#include "timer/timer.h"

#include "liburing.h"

typedef enum SOCKET_STATES { NEW, BOUND, LISTENING, CONNECTED, ACCEPTING, CLOSED } SOCKET_STATES;

#define SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params)                                            \
    struct io_uring_sqe *sqe = create_sqe(ring);                                                   \
    if (sqe == NULL) {                                                                             \
        return -1;                                                                                 \
    }                                                                                              \
    if (timeout(ring, sqe, timeout_params) < 0)                                                    \
        return -1;
