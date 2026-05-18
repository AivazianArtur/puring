#include "future.h"
#include "python_api/loop/loop.h"

// DRAFT
PyObject* create_future(PuringLoop *self) 
{
    PyObject *py_loop = (PyObject *)self;
    if (!py_loop) {
        PyErr_SetString(PyExc_RuntimeError, "Ring Event Loop not attached");
        return NULL;
    }

    return PyObject_CallMethod(py_loop, "create_future", NULL);
}
