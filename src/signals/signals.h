#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <signal.h>
#include <sys/poll.h>
#include <unistd.h>

#include "liburing.h"
#include "ring/ring.h"
#include "python_api/signals/signals.h"

int
set_signals_handler(struct io_uring *ring, int pipefd);
