#pragma once

#define _GNU_SOURCE

#include <unistd.h>
#include <signal.h>
#include <sys/poll.h>


#include "Python.h"

#include "liburing.h"
#include "queue_events/sqe/sqe.h"


typedef struct SignalsData{
    int fd;
} SignalsData;


int set_signals_handler(struct io_uring *ring);


#define IS_SIGNALS_DATA(val) _Generic((val), \
    SignalsData: 1,      \
    default: 0               \
)