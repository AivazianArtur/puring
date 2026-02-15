#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdio.h>

#include "core.h"
#include "python_api/ops/sockets/sockets.h"
#include "python_api/loop/loop.h"
#include "python_api/signals/signals.h"

void on_uring_ready(UringLoop *self);

PyObject* 
init_socket(int fd, PyObject *py_loop);

PyObject *py_on_uring_ready(PyObject *self, PyObject *args);

void uring_loop_register_fd(UringLoop *self);
