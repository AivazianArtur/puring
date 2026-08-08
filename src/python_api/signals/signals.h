#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "ring/ring.h"

typedef struct SignalsData {
    int fd;
} SignalsData;

int
set_signals_controller(struct io_uring *ring);

#define IS_SIGNALS_DATA(val) _Generic((val), SignalsData: 1, default: 0)
