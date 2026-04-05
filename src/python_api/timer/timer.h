#pragma once

#include "Python.h"

#include "python_api/loop/loop.h"

#include "macroses.h"
#include "python_macroses.h"
#include "queue_events/sqe/sqe.h"


PyObject*
UringLoop_timer(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);

int parse_timer_params(PyObject *obj, TimerParams *out);
int parse_timeout_params(PyObject *obj, TimeoutParams *out);
