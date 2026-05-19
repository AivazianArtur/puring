#include "future.h"
#include "python_api/loop/loop.h"

// DRAFT
PyObject* create_future(PuringLoop *self) 
{
    PyObject *py_loop = (PyObject *)self;
    return PyObject_CallMethod(py_loop, "create_future", NULL);
}
