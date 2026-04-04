#pragma once

#include "Python.h"

#include "python_api/loop/loop.h"

#include "macroses.h"
#include "python_macroses.h"
#include "sqe/sqe.h"


PyObject*
UringLoop_timer(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);
