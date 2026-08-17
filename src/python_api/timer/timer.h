#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "python_api/loop/loop.h"

#include "macroses.h"
#include "python_macroses.h"
#include "ring/ring.h"

PyObject *
UringioLoop_timer(PyObject *self, PyObject *args, PyObject *kwargs);

int
parse_timer_params(PyObject *obj, TimerParams *out, StreamStrategy stream_strategy);

int
parse_timeout_params(PyObject *obj, TimeoutParams *out);
