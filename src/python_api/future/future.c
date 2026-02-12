#include "future.h"

// DRAFT
PyObject* create_future(UringLoop *self) 
{
    if (!self->py_loop) {
        PyErr_SetString(PyExc_RuntimeError, "Ring loop not attached");
        return NULL;
    }

    return PyObject_CallMethod(self->py_loop, "create_future", NULL);
}
