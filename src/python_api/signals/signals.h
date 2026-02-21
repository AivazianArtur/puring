#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdbool.h>

#include "core.h"


bool is_future_canceled(
    RequestSlot *slot,
    RequestRegistry *registry,
    struct io_uring ring,
    struct io_uring_cqe *cqe,
    int index
);
