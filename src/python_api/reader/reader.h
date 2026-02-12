#include <Python.h>
#include "loop/loop.h"
#include "sockets/sockets.h"
#include "core.h"
#include "signals/signals.h"

void on_uring_ready(UringLoop *self);

PyObject* 
init_socket(int fd, PyObject *py_loop);

PyObject *py_on_uring_ready(PyObject *self, PyObject *args);

void uring_loop_register_fd(UringLoop *self);
