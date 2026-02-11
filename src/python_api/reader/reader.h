#include <Python.h>
#include "loop.h"
#include "socket.h"
#include "registry.h"
#include "core.h"

static void on_uring_ready(UringLoop *self);

PyObject* 
init_socket(int fd, PyObject *py_loop);

static PyObject *py_on_uring_ready(PyObject *self, PyObject *args);

void uring_loop_register_fd(UringLoop *self);
