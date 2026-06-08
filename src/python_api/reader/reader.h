#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdio.h>

#include "python_api/loop/loop.h"
#include "python_api/ops/files/files.h"
#include "python_api/ops/sockets/sockets.h"
#include "registry/registry.h"
#include "ring/ring.h"
#include "signals/signals.h"

void
on_uring_ready(PuringLoop *self);
