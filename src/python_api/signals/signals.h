#pragma once

#include "Python.h"

#include <signal.h>
#include <sys/poll.h>
#include <unistd.h>

#include "liburing.h"
#include "ring/ring.h"

typedef struct SignalsData {
    int fd;
} SignalsData;

int
set_signals_handler(struct io_uring *ring);

#define IS_SIGNALS_DATA(val) _Generic((val), SignalsData: 1, default: 0)
