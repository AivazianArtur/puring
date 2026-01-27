#include "future.h"

// DRAFT
static PyObject* create_future(PuringObject *self) {
    if (!self->loop) {
        PyErr_SetString(PyExc_RuntimeError, "Ring loop not attached");
        return NULL;
    }
    return PyObject_CallMethod(self->loop, "create_future", NULL);
}
