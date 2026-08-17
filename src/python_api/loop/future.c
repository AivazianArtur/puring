#include "loop.h"

PyObject *
create_future(UringioLoop *self) {
    PyObject *py_loop = (PyObject *)self;
    return PyObject_CallMethod(py_loop, "create_future", NULL);
}
