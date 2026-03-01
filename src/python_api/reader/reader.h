#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdio.h>

#include "core.h"
#include "python_api/ops/sockets/sockets.h"
#include "python_api/loop/loop.h"
#include "python_api/signals/signals.h"

void on_uring_ready(UringLoop *self);

static PyObject *py_on_uring_ready(PyObject *capsule);

void uring_loop_register_fd(UringLoop *self);
